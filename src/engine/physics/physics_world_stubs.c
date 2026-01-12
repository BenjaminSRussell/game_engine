#include <math.h> // For sqrtf, etc.
#include <physics/physics.h>
#include <physics/physics_engine_core.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static _Atomic uint32_t global_body_id_counter = ATOMIC_VAR_INIT(1);

static bool ray_intersect_sphere(const Vec3 origin, const Vec3 dir,
                                 const Vec3 center, float radius,
                                 float max_dist, float *out_t) {
  Vec3 oc = {origin.x - center.x, origin.y - center.y, origin.z - center.z};
  float b = oc.x * dir.x + oc.y * dir.y + oc.z * dir.z;
  float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - radius * radius;
  float discriminant = b * b - c;
  if (discriminant < 0.0f)
    return false;

  float sqrt_disc = sqrtf(discriminant);
  float t = -b - sqrt_disc;
  if (t < 0.0f)
    t = -b + sqrt_disc;
  if (t < 0.0f || t > max_dist)
    return false;

  if (out_t)
    *out_t = t;
  return true;
}

static bool ray_intersect_aabb(const Vec3 origin, const Vec3 dir,
                               const Vec3 min, const Vec3 max, float max_dist,
                               float *out_t, Vec3 *out_normal) {
  float tmin = 0.0f;
  float tmax = max_dist;
  Vec3 normal = {0.0f, 0.0f, 0.0f};

  const float dir_vals[3] = {dir.x, dir.y, dir.z};
  const float origin_vals[3] = {origin.x, origin.y, origin.z};
  const float min_vals[3] = {min.x, min.y, min.z};
  const float max_vals[3] = {max.x, max.y, max.z};

  for (int i = 0; i < 3; ++i) {
    float inv_d = (fabsf(dir_vals[i]) > 1e-6f) ? 1.0f / dir_vals[i] : FLT_MAX;
    float t0 = (min_vals[i] - origin_vals[i]) * inv_d;
    float t1 = (max_vals[i] - origin_vals[i]) * inv_d;
    Vec3 axis_normal = {0.0f, 0.0f, 0.0f};
    axis_normal.x = (i == 0) ? -1.0f : 0.0f;
    axis_normal.y = (i == 1) ? -1.0f : 0.0f;
    axis_normal.z = (i == 2) ? -1.0f : 0.0f;

    if (t0 > t1) {
      float tmp = t0;
      t0 = t1;
      t1 = tmp;
      axis_normal.x = -axis_normal.x;
      axis_normal.y = -axis_normal.y;
      axis_normal.z = -axis_normal.z;
    }

    if (t0 > tmin) {
      tmin = t0;
      normal = axis_normal;
    }
    if (t1 < tmax)
      tmax = t1;
    if (tmax < tmin)
      return false;
  }

  if (out_t)
    *out_t = tmin;
  if (out_normal)
    *out_normal = normal;
  return true;
}

static void integrate_body(RigidBody *body, f32 dt, const Vec3 gravity) {
  if (!body || body->type == RIGID_BODY_STATIC || !body->is_active) {
    return;
  }

  // Apply Gravity
  if (body->type == RIGID_BODY_DYNAMIC) {
    body->velocity[0] += gravity.x * dt;
    body->velocity[1] += gravity.y * dt;
    body->velocity[2] += gravity.z * dt;
  }

  // Apply accumulated forces
  if (body->type == RIGID_BODY_DYNAMIC && body->inv_mass > 0.0f) {
    body->velocity[0] += (body->accumulated_force[0] * body->inv_mass) * dt;
    body->velocity[1] += (body->accumulated_force[1] * body->inv_mass) * dt;
    body->velocity[2] += (body->accumulated_force[2] * body->inv_mass) * dt;
  }

  body->accumulated_force[0] = 0.0f;
  body->accumulated_force[1] = 0.0f;
  body->accumulated_force[2] = 0.0f;

  // Apply Damping (simplified)
  f32 linear_damping_factor = 1.0f - (body->linear_damping * dt);
  if (linear_damping_factor < 0.0f)
    linear_damping_factor = 0.0f;

  body->velocity[0] *= linear_damping_factor;
  body->velocity[1] *= linear_damping_factor;
  body->velocity[2] *= linear_damping_factor;

  // Integrate Position
  body->position[0] += body->velocity[0] * dt;
  body->position[1] += body->velocity[1] * dt;
  body->position[2] += body->velocity[2] * dt;

  // Simple Ground Plane Collision (y = 0)
  // TODO: Replace with proper broadphase/narrowphase later
  if (body->position[1] < 0.0f) {
    body->position[1] = 0.0f;
    if (body->velocity[1] < 0.0f) {
      // Bounce with restitution
      body->velocity[1] = -body->velocity[1] * body->restitution;

      // Apply friction
      body->velocity[0] *= (1.0f - body->friction);
      body->velocity[2] *= (1.0f - body->friction);
    }
  }
}

// ============================================================================
// PHYSICS WORLD IMPLEMENTATION
// ============================================================================

PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = (PhysicsWorld *)calloc(1, sizeof(PhysicsWorld));
  if (!world)
    return NULL;

  world->body_capacity = 1024; // Default capacity
  world->bodies =
      (RigidBody **)calloc(world->body_capacity, sizeof(RigidBody *));
  world->gravity[0] = config.gravity.x;
  world->gravity[1] = config.gravity.y;
  world->gravity[2] = config.gravity.z;
  world->timestep =
      config.fixed_timestep > 0 ? config.fixed_timestep : 1.0f / 60.0f;
  world->velocity_iterations = config.velocity_iterations;
  world->position_iterations = config.position_iterations;

  return world;
}

void physics_world_init(PhysicsWorld *world, const Vec3 *gravity) {
  if (!world)
    return;

  if (!world->bodies) {
    world->body_capacity = 1024;
    world->bodies =
        (RigidBody **)calloc(world->body_capacity, sizeof(RigidBody *));
  }

  if (gravity) {
    world->gravity[0] = gravity->x;
    world->gravity[1] = gravity->y;
    world->gravity[2] = gravity->z;
  } else {
    world->gravity[0] = 0.0f;
    world->gravity[1] = -9.81f;
    world->gravity[2] = 0.0f;
  }

  if (world->timestep <= 0.0f)
    world->timestep = 1.0f / 60.0f;

  if (world->velocity_iterations == 0)
    world->velocity_iterations = 8;
  if (world->position_iterations == 0)
    world->position_iterations = 3;

  world->paused = false;
  world->time_accumulator = 0.0f;
}

void physics_world_free(PhysicsWorld *world) {
  if (!world)
    return;
  if (world->bodies) {
    free(world->bodies);
    world->bodies = NULL;
  }
  world->body_count = 0;
  world->body_capacity = 0;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world)
    return;
  if (world->bodies) {
    // Note: We don't free individual bodies here as they might be managed
    // elsewhere or we should iterate and free if we own them. For now, assume
    // ownership is shared or managed by caller/ECS.
    free(world->bodies);
  }
  free(world);
}

void physics_world_step(PhysicsWorld *world, f32 dt) {
  if (!world || world->paused)
    return;

  world->time_accumulator += dt;

  // Fixed timestep updates
  while (world->time_accumulator >= world->timestep) {
    Vec3 grav = {world->gravity[0], world->gravity[1], world->gravity[2]};

    for (uint32_t i = 0; i < world->body_count; i++) {
      integrate_body(world->bodies[i], world->timestep, grav);
    }

    world->time_accumulator -= world->timestep;
  }
}

RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return NULL;

  if (world->body_count >= world->body_capacity) {
    // Simple resize
    uint32_t new_cap = world->body_capacity * 2;
    world->bodies = realloc(world->bodies, new_cap * sizeof(RigidBody *));
    world->body_capacity = new_cap;
  }

  world->bodies[world->body_count++] = body;
  return body;
}

void physics_world_remove_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body || world->body_count == 0)
    return;

  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i] == body) {
      uint32_t last = world->body_count - 1;
      if (i != last) {
        world->bodies[i] = world->bodies[last];
      }
      world->bodies[last] = NULL;
      world->body_count--;
      break;
    }
  }
}

bool physics_world_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                           f32 max_distance, void *out_hit) {
  if (!world || max_distance <= 0.0f)
    return false;

  float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y +
                        direction.z * direction.z);
  if (dir_len < 1e-6f)
    return false;

  direction.x /= dir_len;
  direction.y /= dir_len;
  direction.z /= dir_len;

  bool hit = false;
  float closest_t = max_distance;
  Vec3 hit_point = {0};
  Vec3 hit_normal = {0};
  RigidBody *hit_body = NULL;

  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || !body->shape)
      continue;

    Vec3 center = {body->position[0], body->position[1], body->position[2]};
    float t = 0.0f;
    Vec3 normal = {0.0f, 0.0f, 0.0f};
    bool body_hit = false;

    switch (body->shape->type) {
    case COLLISION_SHAPE_SPHERE: {
      float radius = body->shape->data.sphere.radius;
      body_hit = ray_intersect_sphere(origin, direction, center, radius,
                                      closest_t, &t);
      if (body_hit) {
        Vec3 point = {origin.x + direction.x * t,
                      origin.y + direction.y * t,
                      origin.z + direction.z * t};
        Vec3 diff = {point.x - center.x, point.y - center.y, point.z - center.z};
        float diff_len = sqrtf(diff.x * diff.x + diff.y * diff.y +
                               diff.z * diff.z);
        if (diff_len > 1e-6f) {
          normal.x = diff.x / diff_len;
          normal.y = diff.y / diff_len;
          normal.z = diff.z / diff_len;
        }
      }
      break;
    }
    case COLLISION_SHAPE_BOX: {
      Vec3 half = {body->shape->data.box.half_extents[0],
                   body->shape->data.box.half_extents[1],
                   body->shape->data.box.half_extents[2]};
      Vec3 min = {center.x - half.x, center.y - half.y, center.z - half.z};
      Vec3 max = {center.x + half.x, center.y + half.y, center.z + half.z};
      body_hit =
          ray_intersect_aabb(origin, direction, min, max, closest_t, &t,
                             &normal);
      break;
    }
    default:
      break;
    }

    if (body_hit && t < closest_t) {
      closest_t = t;
      hit = true;
      hit_point = (Vec3){origin.x + direction.x * t,
                         origin.y + direction.y * t,
                         origin.z + direction.z * t};
      hit_normal = normal;
      hit_body = body;
    }
  }

  if (out_hit) {
    RaycastResult *result = (RaycastResult *)out_hit;
    result->origin = origin;
    result->direction = direction;
    result->max_distance = max_distance;
    result->hit = hit;
    result->hit_point = hit_point;
    result->hit_normal = hit_normal;
    result->hit_distance = hit ? closest_t : max_distance;
    result->hit_body = hit_body;
  }

  return hit;
}

// ============================================================================
// RIGID BODY IMPLEMENTATION
// ============================================================================

RigidBody *rigid_body_create(BodyType type, Vec3 position) {
  RigidBody *body = (RigidBody *)calloc(1, sizeof(RigidBody));
  if (!body)
    return NULL;

  body->id = atomic_fetch_add(&global_body_id_counter, 1);

  // Map BodyType to RigidBodyType
  switch (type) {
  case BODY_TYPE_STATIC:
    body->type = RIGID_BODY_STATIC;
    break;
  case BODY_TYPE_KINEMATIC:
    body->type = RIGID_BODY_KINEMATIC;
    break;
  case BODY_TYPE_DYNAMIC:
    body->type = RIGID_BODY_DYNAMIC;
    break;
  default:
    body->type = RIGID_BODY_DYNAMIC;
    break;
  }

  body->position[0] = position.x;
  body->position[1] = position.y;
  body->position[2] = position.z;

  // Defaults
  body->rotation[3] = 1.0f; // Identity quaternion (w=1)
  body->mass = 1.0f;
  body->inv_mass = 1.0f;
  body->friction = 0.5f;
  body->restitution = 0.3f;
  body->linear_damping = 0.1f;
  body->angular_damping = 0.1f;
  body->is_active = true;

  if (body->type == RIGID_BODY_STATIC) {
    body->mass = 0.0f;
    body->inv_mass = 0.0f;
  }

  return body;
}

void rigid_body_destroy(RigidBody *body) {
  if (body) {
    // Free shape if needed (could be shared)
    free(body);
  }
}

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
  if (body) {
    body->shape = (CollisionShape *)collider; // Structs are compatible/aliased
  }
}

Vec3 rigid_body_get_position(const RigidBody *body) {
  if (!body)
    return (Vec3){0};
  return (Vec3){body->position[0], body->position[1], body->position[2]};
}

Vec3 rigid_body_get_velocity(const RigidBody *body) {
  if (!body)
    return (Vec3){0};
  return (Vec3){body->velocity[0], body->velocity[1], body->velocity[2]};
}

void rigid_body_set_friction(RigidBody *body, f32 friction) {
  if (body)
    body->friction = friction;
}

void rigid_body_set_linear_damping(RigidBody *body, f32 damping) {
  if (body)
    body->linear_damping = damping;
}

void rigid_body_set_angular_damping(RigidBody *body, f32 damping) {
  if (body)
    body->angular_damping = damping;
}

void rigid_body_set_position(RigidBody *body, Vec3 position) {
  if (body) {
    body->position[0] = position.x;
    body->position[1] = position.y;
    body->position[2] = position.z;
  }
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
  if (body)
    body->restitution = restitution;
}

void rigid_body_set_velocity(RigidBody *body, Vec3 velocity) {
  if (body) {
    body->velocity[0] = velocity.x;
    body->velocity[1] = velocity.y;
    body->velocity[2] = velocity.z;
  }
}

void rigid_body_set_rotation(RigidBody *body, Quat rotation) {
  if (body) {
    body->rotation[0] = rotation.x;
    body->rotation[1] = rotation.y;
    body->rotation[2] = rotation.z;
    body->rotation[3] = rotation.w;
  }
}

void rigid_body_add_force(RigidBody *body, Vec3 force) {
  if (!body || body->type == RIGID_BODY_STATIC)
    return;
  body->accumulated_force[0] += force.x;
  body->accumulated_force[1] += force.y;
  body->accumulated_force[2] += force.z;
}

void rigid_body_add_impulse(RigidBody *body, Vec3 impulse) {
  if (!body || body->type == RIGID_BODY_STATIC || body->inv_mass <= 0.0f)
    return;
  body->velocity[0] += impulse.x * body->inv_mass;
  body->velocity[1] += impulse.y * body->inv_mass;
  body->velocity[2] += impulse.z * body->inv_mass;
}

void rigid_body_clear_forces(RigidBody *body) {
  if (!body)
    return;
  body->accumulated_force[0] = 0.0f;
  body->accumulated_force[1] = 0.0f;
  body->accumulated_force[2] = 0.0f;
  body->accumulated_torque[0] = 0.0f;
  body->accumulated_torque[1] = 0.0f;
  body->accumulated_torque[2] = 0.0f;
}

f32 rigid_body_get_inv_mass(const RigidBody *body) {
  if (!body)
    return 0.0f;
  return body->inv_mass;
}

Collider *rigid_body_get_collider(const RigidBody *body) {
  if (!body)
    return NULL;
  return (Collider *)body->shape;
}
