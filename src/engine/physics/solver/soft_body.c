#include "physics/soft_body.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global soft body world instance
static SoftBodyWorld g_soft_body_world = {0};

bool soft_body_world_init(SoftBodyWorld *world, uint32_t max_bodies) {
  if (!world || max_bodies == 0)
    return false;

  memset(world, 0, sizeof(SoftBodyWorld));

  // Allocate bodies array
  world->body_capacity = max_bodies;
  world->bodies =
      (SoftBody *)core_alloc(world->body_capacity * sizeof(SoftBody));
  if (!world->bodies)
    return false;
  memset(world->bodies, 0, world->body_capacity * sizeof(SoftBody));

  // Set default world parameters
  world->gravity = (Vec3){0.0f, -9.81f, 0.0f};
  world->wind_velocity = vec3_zero();
  world->air_density = 1.2f;
  world->time_scale = 1.0f;
  world->enable_sleeping = true;

  // Set default collision parameters
  world->collision.enabled = true;
  world->collision.margin = 0.1f;
  world->collision.max_contacts_per_body = 16;

  LOG_INFO("Soft body world initialized with capacity for %u bodies",
           max_bodies);
  return true;
}

void soft_body_world_cleanup(SoftBodyWorld *world) {
  if (!world)
    return;

  // Cleanup all bodies
  for (uint32_t i = 0; i < world->body_count; i++) {
    SoftBody *body = &world->bodies[i];
    if (body->particles) {
      core_free(body->particles);
      body->particles = NULL;
    }
    if (body->constraints) {
      core_free(body->constraints);
      body->constraints = NULL;
    }
    if (body->faces) {
      core_free(body->faces);
      body->faces = NULL;
    }
    if (body->vertex_positions) {
      core_free(body->vertex_positions);
      body->vertex_positions = NULL;
    }
    if (body->vertex_normals) {
      core_free(body->vertex_normals);
      body->vertex_normals = NULL;
    }
    if (body->collision.collision_normals) {
      core_free(body->collision.collision_normals);
      body->collision.collision_normals = NULL;
    }
    if (body->collision.penetration_depths) {
      core_free(body->collision.penetration_depths);
      body->collision.penetration_depths = NULL;
    }
  }

  if (world->bodies) {
    core_free(world->bodies);
    world->bodies = NULL;
  }

  memset(world, 0, sizeof(SoftBodyWorld));
  LOG_INFO("Soft body world cleaned up");
}

void soft_body_world_update(SoftBodyWorld *world, float time_step) {
  if (!world)
    return;

  float scaled_time_step = time_step * world->time_scale;

  // Update statistics
  world->stats.active_bodies = 0;
  world->stats.total_particles = 0;
  world->stats.total_constraints = 0;

  float start_time = get_time();

  // Update each soft body
  for (uint32_t i = 0; i < world->body_count; i++) {
    SoftBody *body = &world->bodies[i];

    if (!body->active)
      continue;

    world->stats.active_bodies++;
    world->stats.total_particles += body->config.particle_count;
    world->stats.total_constraints += body->config.constraint_count;

    // Check if body should sleep
    if (world->enable_sleeping && !body->sleeping) {
      float total_velocity = 0.0f;
      for (uint32_t j = 0; j < body->config.particle_count; j++) {
        total_velocity += vec3_length_sq(body->particles[j].velocity);
      }

      float avg_velocity = total_velocity / body->config.particle_count;
      if (avg_velocity < body->sleep_threshold * body->sleep_threshold) {
        body->sleep_timer += scaled_time_step;
        if (body->sleep_timer > 1.0f) {
          body->sleeping = true;
          LOG_DEBUG("Soft body %u went to sleep", i);
        }
      } else {
        body->sleep_timer = 0.0f;
      }
    }

    if (!body->sleeping) {
      soft_body_update(body, scaled_time_step);
    }
  }

  world->stats.simulation_time = get_time() - start_time;
}

uint32_t soft_body_create_cloth(SoftBodyWorld *world,
                                const SoftBodyConfig *config, uint32_t width,
                                uint32_t height, float spacing) {
  if (!world || !config || width == 0 || height == 0 || spacing <= 0.0f)
    return UINT32_MAX;

  if (world->body_count >= world->body_capacity)
    return UINT32_MAX;

  SoftBody *body = &world->bodies[world->body_count];
  memset(body, 0, sizeof(SoftBody));
  body->config = *config;

  // Calculate particle count
  body->config.particle_count = width * height;
  body->config.constraint_count =
      (width - 1) * height + width * (height - 1); // Horizontal + vertical
  body->config.face_count = (width - 1) * (height - 1) * 2; // Triangles

  // Allocate particles
  body->particles = (SoftParticle *)core_alloc(body->config.particle_count *
                                               sizeof(SoftParticle));
  if (!body->particles)
    return UINT32_MAX;
  memset(body->particles, 0,
         body->config.particle_count * sizeof(SoftParticle));

  // Allocate constraints
  body->constraints = (SoftConstraint *)core_alloc(
      body->config.constraint_count * sizeof(SoftConstraint));
  if (!body->constraints) {
    core_free(body->particles);
    return UINT32_MAX;
  }
  memset(body->constraints, 0,
         body->config.constraint_count * sizeof(SoftConstraint));

  // Allocate faces
  body->faces =
      (SoftFace *)core_alloc(body->config.face_count * sizeof(SoftFace));
  if (!body->faces) {
    core_free(body->particles);
    core_free(body->constraints);
    return UINT32_MAX;
  }
  memset(body->faces, 0, body->config.face_count * sizeof(SoftFace));

  // Initialize particles
  float particle_mass = body->config.total_mass / body->config.particle_count;
  Vec3 start_pos = vec3_mul((Vec3){-(width - 1) * spacing * 0.5f, 0.0f,
                                   -(height - 1) * spacing * 0.5f},
                            1.0f);

  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      uint32_t index = y * width + x;
      SoftParticle *particle = &body->particles[index];

      particle->position =
          vec3_add(start_pos, (Vec3){x * spacing, 0.0f, y * spacing});
      particle->old_position = particle->position;
      particle->velocity = vec3_zero();
      particle->force = vec3_zero();
      particle->mass = particle_mass;
      particle->inv_mass = 1.0f / particle_mass;
      particle->type = PARTICLE_FREE;
      particle->collision_enabled = true;
      particle->collision_mask = 0xFFFFFFFF;
      particle->collision_group = 1;
      particle->normal = (Vec3){0, 1, 0};
      particle->friction = body->config.friction;
      particle->restitution = body->config.restitution;
    }
  }

  // Create constraints
  uint32_t constraint_index = 0;

  // Horizontal constraints
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width - 1; x++) {
      uint32_t p1 = y * width + x;
      uint32_t p2 = y * width + (x + 1);

      SoftConstraint *constraint = &body->constraints[constraint_index++];
      constraint->particle_a = p1;
      constraint->particle_b = p2;
      constraint->type = SOFT_CONSTRAINT_DISTANCE;
      constraint->rest_length = spacing;
      constraint->stiffness = body->config.stiffness;
      constraint->damping = body->config.damping;
      constraint->enabled = true;
      constraint->max_stretch = spacing * 1.5f;
      constraint->max_compression = spacing * 0.5f;
    }
  }

  // Vertical constraints
  for (uint32_t y = 0; y < height - 1; y++) {
    for (uint32_t x = 0; x < width; x++) {
      uint32_t p1 = y * width + x;
      uint32_t p2 = (y + 1) * width + x;

      SoftConstraint *constraint = &body->constraints[constraint_index++];
      constraint->particle_a = p1;
      constraint->particle_b = p2;
      constraint->type = SOFT_CONSTRAINT_DISTANCE;
      constraint->rest_length = spacing;
      constraint->stiffness = body->config.stiffness;
      constraint->damping = body->config.damping;
      constraint->enabled = true;
      constraint->max_stretch = spacing * 1.5f;
      constraint->max_compression = spacing * 0.5f;
    }
  }

  // Create faces for rendering
  uint32_t face_index = 0;
  for (uint32_t y = 0; y < height - 1; y++) {
    for (uint32_t x = 0; x < width - 1; x++) {
      uint32_t tl = y * width + x;
      uint32_t tr = y * width + (x + 1);
      uint32_t bl = (y + 1) * width + x;
      uint32_t br = (y + 1) * width + (x + 1);

      // First triangle
      SoftFace *face1 = &body->faces[face_index++];
      face1->indices[0] = tl;
      face1->indices[1] = bl;
      face1->indices[2] = tr;
      face1->active = true;

      // Second triangle
      SoftFace *face2 = &body->faces[face_index++];
      face2->indices[0] = tr;
      face2->indices[1] = bl;
      face2->indices[2] = br;
      face2->active = true;
    }
  }

  // Initialize other properties
  body->active = true;
  body->sleeping = false;
  body->sleep_threshold = 0.1f;
  body->sleep_timer = 0.0f;
  body->aabb.bounds_dirty = true;

  // Allocate rendering data
  body->vertex_count = body->config.particle_count;
  body->vertex_positions =
      (Vec3 *)core_alloc(body->vertex_count * sizeof(Vec3));
  body->vertex_normals = (Vec3 *)core_alloc(body->vertex_count * sizeof(Vec3));

  if (!body->vertex_positions || !body->vertex_normals) {
    // Cleanup on failure
    core_free(body->particles);
    core_free(body->constraints);
    core_free(body->faces);
    if (body->vertex_positions)
      core_free(body->vertex_positions);
    if (body->vertex_normals)
      core_free(body->vertex_normals);
    return UINT32_MAX;
  }

  // Initialize collision data
  body->collision.collision_normals =
      (Vec3 *)core_alloc(world->collision.max_contacts_per_body * sizeof(Vec3));
  body->collision.penetration_depths = (float *)core_alloc(
      world->collision.max_contacts_per_body * sizeof(float));

  if (!body->collision.collision_normals ||
      !body->collision.penetration_depths) {
    // Cleanup on failure
    core_free(body->particles);
    core_free(body->constraints);
    core_free(body->faces);
    core_free(body->vertex_positions);
    core_free(body->vertex_normals);
    if (body->collision.collision_normals)
      core_free(body->collision.collision_normals);
    if (body->collision.penetration_depths)
      core_free(body->collision.penetration_depths);
    return UINT32_MAX;
  }

  uint32_t body_id = world->body_count++;
  LOG_INFO("Created cloth soft body %u: %ux%u particles, %u constraints",
           body_id, width, height, body->config.constraint_count);

  return body_id;
}

uint32_t soft_body_create_rope(SoftBodyWorld *world,
                               const SoftBodyConfig *config,
                               uint32_t segment_count, float segment_length) {
  if (!world || !config || segment_count < 2 || segment_length <= 0.0f)
    return UINT32_MAX;

  if (world->body_count >= world->body_capacity)
    return UINT32_MAX;

  SoftBody *body = &world->bodies[world->body_count];
  memset(body, 0, sizeof(SoftBody));
  body->config = *config;

  // Rope has particles = segments + 1, constraints = segments
  body->config.particle_count = segment_count + 1;
  body->config.constraint_count = segment_count;
  body->config.face_count = 0; // Rope doesn't have faces

  // Allocate particles
  body->particles = (SoftParticle *)core_alloc(body->config.particle_count *
                                               sizeof(SoftParticle));
  if (!body->particles)
    return UINT32_MAX;
  memset(body->particles, 0,
         body->config.particle_count * sizeof(SoftParticle));

  // Allocate constraints
  body->constraints = (SoftConstraint *)core_alloc(
      body->config.constraint_count * sizeof(SoftConstraint));
  if (!body->constraints) {
    core_free(body->particles);
    return UINT32_MAX;
  }
  memset(body->constraints, 0,
         body->config.constraint_count * sizeof(SoftConstraint));

  // Initialize particles
  float particle_mass = body->config.total_mass / body->config.particle_count;
  Vec3 start_pos = vec3_zero();

  for (uint32_t i = 0; i <= segment_count; i++) {
    SoftParticle *particle = &body->particles[i];

    particle->position =
        vec3_add(start_pos, (Vec3){0.0f, -i * segment_length, 0.0f});
    particle->old_position = particle->position;
    particle->velocity = vec3_zero();
    particle->force = vec3_zero();
    particle->mass = particle_mass;
    particle->inv_mass = 1.0f / particle_mass;
    particle->type = PARTICLE_FREE;
    particle->collision_enabled = true;
    particle->collision_mask = 0xFFFFFFFF;
    particle->collision_group = 1;
    particle->normal = vec3_zero();
    particle->friction = body->config.friction;
    particle->restitution = body->config.restitution;
  }

  // Create constraints
  for (uint32_t i = 0; i < segment_count; i++) {
    SoftConstraint *constraint = &body->constraints[i];

    constraint->particle_a = i;
    constraint->particle_b = i + 1;
    constraint->type = SOFT_CONSTRAINT_DISTANCE;
    constraint->rest_length = segment_length;
    constraint->stiffness = body->config.stiffness;
    constraint->damping = body->config.damping;
    constraint->enabled = true;
    constraint->max_stretch = segment_length * 1.2f;
    constraint->max_compression = segment_length * 0.8f;
  }

  // Initialize other properties
  body->active = true;
  body->sleeping = false;
  body->sleep_threshold = 0.1f;
  body->sleep_timer = 0.0f;
  body->aabb.bounds_dirty = true;

  // Allocate rendering data
  body->vertex_count = body->config.particle_count;
  body->vertex_positions =
      (Vec3 *)core_alloc(body->vertex_count * sizeof(Vec3));
  body->vertex_normals = (Vec3 *)core_alloc(body->vertex_count * sizeof(Vec3));

  if (!body->vertex_positions || !body->vertex_normals) {
    core_free(body->particles);
    core_free(body->constraints);
    if (body->vertex_positions)
      core_free(body->vertex_positions);
    if (body->vertex_normals)
      core_free(body->vertex_normals);
    return UINT32_MAX;
  }

  uint32_t body_id = world->body_count++;
  LOG_INFO("Created rope soft body %u: %u segments", body_id, segment_count);

  return body_id;
}

void soft_body_update(SoftBody *body, float time_step) {
  if (!body || !body->active || body->sleeping)
    return;

  float start_time = get_time();

  // Reset statistics
  body->stats.particles_updated = 0;
  body->stats.constraints_solved = 0;
  body->stats.collisions_handled = 0;

  // Apply external forces
  soft_body_apply_gravity(body, g_soft_body_world.gravity);
  soft_body_apply_wind(body, g_soft_body_world.wind_velocity,
                       g_soft_body_world.air_density);
  soft_body_apply_drag(body, body->config.material.air_resistance);

  // Integrate particle motion (Verlet integration)
  soft_body_integrate_particles(body, time_step);

  // Solve constraints
  soft_body_solve_constraints(body);

  // Handle collisions
  if (g_soft_body_world.collision.enabled) {
    soft_body_handle_collisions(body, &g_soft_body_world);
  }

  // Update bounds and normals
  soft_body_update_bounds(body);
  soft_body_update_normals(body);

  // Update rendering data
  for (uint32_t i = 0; i < body->vertex_count; i++) {
    body->vertex_positions[i] = body->particles[i].position;
    body->vertex_normals[i] = body->particles[i].normal;
  }

  body->stats.simulation_time = get_time() - start_time;
}

void soft_body_integrate_particles(SoftBody *body, float time_step) {
  if (!body)
    return;

  float time_step_sq = time_step * time_step;

  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    SoftParticle *particle = &body->particles[i];

    if (particle->type == PARTICLE_ANCHORED ||
        particle->type == PARTICLE_KINEMATIC) {
      continue; // Don't integrate anchored or kinematic particles
    }

    // Verlet integration
    Vec3 acceleration = vec3_mul(particle->force, particle->inv_mass);
    Vec3 new_position =
        vec3_add(particle->position,
                 vec3_add(vec3_sub(particle->position, particle->old_position),
                          vec3_mul(acceleration, time_step_sq)));

    particle->old_position = particle->position;
    particle->position = new_position;
    particle->velocity =
        vec3_div(vec3_sub(new_position, particle->old_position), time_step);

    // Reset force for next frame
    particle->force = vec3_zero();

    body->stats.particles_updated++;
  }
}

void soft_body_solve_constraints(SoftBody *body) {
  if (!body)
    return;

  // Solve distance constraints (most common for cloth and rope)
  soft_body_solve_distance_constraints(body, body->config.solver_iterations);

  // Solve other constraint types based on soft body type
  if (body->config.type == SOFT_BODY_CLOTH) {
    soft_body_solve_bend_constraints(body, body->config.solver_iterations / 2);
    soft_body_solve_shear_constraints(body, body->config.solver_iterations / 2);
  }
}

void soft_body_solve_distance_constraints(SoftBody *body, int iterations) {
  if (!body || iterations <= 0)
    return;

  for (int iter = 0; iter < iterations; iter++) {
    for (uint32_t i = 0; i < body->config.constraint_count; i++) {
      SoftConstraint *constraint = &body->constraints[i];

      if (!constraint->enabled ||
          constraint->type != SOFT_CONSTRAINT_DISTANCE) {
        continue;
      }

      SoftParticle *p1 = &body->particles[constraint->particle_a];
      SoftParticle *p2 = &body->particles[constraint->particle_b];

      // Skip if both particles are anchored
      if (p1->type == PARTICLE_ANCHORED && p2->type == PARTICLE_ANCHORED) {
        continue;
      }

      Vec3 delta = vec3_sub(p2->position, p1->position);
      float distance = vec3_length(delta);

      if (distance < 0.001f)
        continue; // Particles are at same position

      Vec3 direction = vec3_div(delta, distance);

      // Check constraint limits
      if (distance > constraint->max_stretch) {
        // Constraint is stretched too much, apply correction
        float correction =
            (distance - constraint->rest_length) * constraint->stiffness;
        Vec3 correction_vector = vec3_mul(direction, correction);

        float total_inv_mass = p1->inv_mass + p2->inv_mass;
        if (total_inv_mass > 0.0f) {
          float p1_ratio = p1->inv_mass / total_inv_mass;
          float p2_ratio = p2->inv_mass / total_inv_mass;

          if (p1->type != PARTICLE_ANCHORED && p1->type != PARTICLE_KINEMATIC) {
            p1->position =
                vec3_add(p1->position, vec3_mul(correction_vector, p1_ratio));
          }
          if (p2->type != PARTICLE_ANCHORED && p2->type != PARTICLE_KINEMATIC) {
            p2->position =
                vec3_sub(p2->position, vec3_mul(correction_vector, p2_ratio));
          }
        }
      } else if (distance < constraint->max_compression) {
        // Constraint is compressed too much, apply correction
        float correction =
            (constraint->rest_length - distance) * constraint->stiffness;
        Vec3 correction_vector = vec3_mul(direction, correction);

        float total_inv_mass = p1->inv_mass + p2->inv_mass;
        if (total_inv_mass > 0.0f) {
          float p1_ratio = p1->inv_mass / total_inv_mass;
          float p2_ratio = p2->inv_mass / total_inv_mass;

          if (p1->type != PARTICLE_ANCHORED && p1->type != PARTICLE_KINEMATIC) {
            p1->position =
                vec3_sub(p1->position, vec3_mul(correction_vector, p1_ratio));
          }
          if (p2->type != PARTICLE_ANCHORED && p2->type != PARTICLE_KINEMATIC) {
            p2->position =
                vec3_add(p2->position, vec3_mul(correction_vector, p2_ratio));
          }
        }
      }

      body->stats.constraints_solved++;
    }
  }
}

void soft_body_apply_gravity(SoftBody *body, Vec3 gravity) {
  if (!body)
    return;

  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    SoftParticle *particle = &body->particles[i];

    if (particle->type == PARTICLE_FREE) {
      particle->force = vec3_add(
          particle->force,
          vec3_mul(gravity, particle->mass * body->config.gravity_scale));
    }
  }
}

void soft_body_apply_wind(SoftBody *body, Vec3 wind_velocity,
                          float air_density) {
  if (!body || body->config.type != SOFT_BODY_CLOTH)
    return;

  for (uint32_t i = 0; i < body->config.face_count; i++) {
    SoftFace *face = &body->faces[i];
    if (!face->active)
      continue;

    SoftParticle *p1 = &body->particles[face->indices[0]];
    SoftParticle *p2 = &body->particles[face->indices[1]];
    SoftParticle *p3 = &body->particles[face->indices[2]];

    // Calculate face normal
    Vec3 edge1 = vec3_sub(p2->position, p1->position);
    Vec3 edge2 = vec3_sub(p3->position, p1->position);
    Vec3 normal = vec3_normalize(vec3_cross(edge1, edge2));

    // Calculate wind force
    Vec3 relative_wind = vec3_sub(
        wind_velocity,
        vec3_mul(vec3_add(vec3_add(p1->velocity, p2->velocity), p3->velocity),
                 1.0f / 3.0f));
    float wind_speed = vec3_length(relative_wind);

    if (wind_speed > 0.001f) {
      float wind_force_magnitude =
          0.5f * air_density * wind_speed * wind_speed * face->area *
          body->config.wind_scale *
          vec3_dot(normal, vec3_normalize(relative_wind));

      Vec3 wind_force = vec3_mul(normal, wind_force_magnitude);

      // Distribute force to particles
      Vec3 particle_force = vec3_mul(wind_force, 1.0f / 3.0f);

      if (p1->type == PARTICLE_FREE)
        p1->force = vec3_add(p1->force, particle_force);
      if (p2->type == PARTICLE_FREE)
        p2->force = vec3_add(p2->force, particle_force);
      if (p3->type == PARTICLE_FREE)
        p3->force = vec3_add(p3->force, particle_force);
    }
  }
}

void soft_body_apply_drag(SoftBody *body, float drag_coefficient) {
  if (!body || drag_coefficient <= 0.0f)
    return;

  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    SoftParticle *particle = &body->particles[i];

    if (particle->type == PARTICLE_FREE) {
      Vec3 drag_force = vec3_mul(particle->velocity, -drag_coefficient);
      particle->force = vec3_add(particle->force, drag_force);
    }
  }
}

void soft_body_update_bounds(SoftBody *body) {
  if (!body || body->config.particle_count == 0)
    return;

  Vec3 min_bounds = body->particles[0].position;
  Vec3 max_bounds = body->particles[0].position;

  for (uint32_t i = 1; i < body->config.particle_count; i++) {
    Vec3 pos = body->particles[i].position;

    if (pos.x < min_bounds.x)
      min_bounds.x = pos.x;
    if (pos.y < min_bounds.y)
      min_bounds.y = pos.y;
    if (pos.z < min_bounds.z)
      min_bounds.z = pos.z;

    if (pos.x > max_bounds.x)
      max_bounds.x = pos.x;
    if (pos.y > max_bounds.y)
      max_bounds.y = pos.y;
    if (pos.z > max_bounds.z)
      max_bounds.z = pos.z;
  }

  body->aabb.bounds_min = min_bounds;
  body->aabb.bounds_max = max_bounds;
  body->aabb.bounds_dirty = false;
}

void soft_body_update_normals(SoftBody *body) {
  if (!body || body->config.type != SOFT_BODY_CLOTH)
    return;

  // Reset all particle normals
  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    body->particles[i].normal = vec3_zero();
  }

  // Calculate face normals and accumulate to particles
  for (uint32_t i = 0; i < body->config.face_count; i++) {
    SoftFace *face = &body->faces[i];
    if (!face->active)
      continue;

    SoftParticle *p1 = &body->particles[face->indices[0]];
    SoftParticle *p2 = &body->particles[face->indices[1]];
    SoftParticle *p3 = &body->particles[face->indices[2]];

    // Calculate face normal
    Vec3 edge1 = vec3_sub(p2->position, p1->position);
    Vec3 edge2 = vec3_sub(p3->position, p1->position);
    Vec3 normal = vec3_normalize(vec3_cross(edge1, edge2));

    // Accumulate normal to each particle
    p1->normal = vec3_add(p1->normal, normal);
    p2->normal = vec3_add(p2->normal, normal);
    p3->normal = vec3_add(p3->normal, normal);
  }

  // Normalize particle normals
  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    SoftParticle *particle = &body->particles[i];
    float length = vec3_length(particle->normal);
    if (length > 0.001f) {
      particle->normal = vec3_div(particle->normal, length);
    } else {
      particle->normal = (Vec3){0, 1, 0}; // Default up normal
    }
  }
}

// Cloth-specific functions
void soft_body_cloth_pin_corner(SoftBody *body, uint32_t corner_index) {
  if (!body || body->config.type != SOFT_BODY_CLOTH || corner_index >= 4)
    return;

  uint32_t width = (uint32_t)sqrtf(body->config.particle_count);
  uint32_t height = width;

  uint32_t particle_index = 0;
  switch (corner_index) {
  case 0:
    particle_index = 0;
    break; // Top-left
  case 1:
    particle_index = width - 1;
    break; // Top-right
  case 2:
    particle_index = (height - 1) * width;
    break; // Bottom-left
  case 3:
    particle_index = height * width - 1;
    break; // Bottom-right
  }

  if (particle_index < body->config.particle_count) {
    body->particles[particle_index].type = PARTICLE_ANCHORED;
    LOG_DEBUG("Pinned corner %u (particle %u) of cloth", corner_index,
              particle_index);
  }
}

// Rope-specific functions
void soft_body_rope_pin_end(SoftBody *body, uint32_t end_index) {
  if (!body || body->config.type != SOFT_BODY_ROPE || end_index > 1)
    return;

  uint32_t particle_index =
      (end_index == 0) ? 0 : body->config.particle_count - 1;

  if (particle_index < body->config.particle_count) {
    body->particles[particle_index].type = PARTICLE_ANCHORED;
    LOG_DEBUG("Pinned end %u (particle %u) of rope", end_index, particle_index);
  }
}

// Utility functions
Vec3 soft_body_get_center_of_mass(const SoftBody *body) {
  if (!body || body->config.particle_count == 0)
    return vec3_zero();

  Vec3 center = vec3_zero();
  float total_mass = 0.0f;

  for (uint32_t i = 0; i < body->config.particle_count; i++) {
    const SoftParticle *particle = &body->particles[i];
    center = vec3_add(center, vec3_mul(particle->position, particle->mass));
    total_mass += particle->mass;
  }

  return (total_mass > 0.0f) ? vec3_div(center, total_mass) : vec3_zero();
}

// Global accessor functions
SoftBodyWorld *get_soft_body_world(void) { return &g_soft_body_world; }

bool init_soft_body_system(uint32_t max_bodies) {
  return soft_body_world_init(&g_soft_body_world, max_bodies);
}

void cleanup_soft_body_system(void) {
  soft_body_world_cleanup(&g_soft_body_world);
}
