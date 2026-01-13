#include <math.h>
#include <physics/physics.h>
#include <physics/physics_engine_core.h>
// Removed collision_gjk_epa.h to avoid conflicts
#include <float.h>
#include <physics/core/physics_types.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// External functions from collision_detection.c
void collision_system_init(void);
void collision_system_clear_bodies(void);
void collision_system_add_body(u32 id, const AABB *bounds, Vec3 position);
void collision_system_update(f32 delta_time);
u32 collision_system_get_contacts_for_body(u32 body_id, void *out_contacts,
                                           u32 max_count);

// Internal helper to calculate AABB
static AABB calculate_aabb(RigidBody *body) {
  AABB bounds = {{0}};
  float rx = 0.5f, ry = 0.5f, rz = 0.5f;

  if (body->shape) {
    if (body->shape->type == COLLISION_SHAPE_SPHERE) {
      rx = ry = rz = body->shape->data.sphere.radius;
    } else if (body->shape->type == COLLISION_SHAPE_BOX) {
      rx = body->shape->data.box.half_extents[0];
      ry = body->shape->data.box.half_extents[1];
      rz = body->shape->data.box.half_extents[2];
    }
  }

  bounds.min = (Vec3){body->position[0] - rx, body->position[1] - ry,
                      body->position[2] - rz};
  bounds.max = (Vec3){body->position[0] + rx, body->position[1] + ry,
                      body->position[2] + rz};
  return bounds;
}

static void integrate_body(RigidBody *body, f32 dt, const Vec3 gravity) {
  if (!body || body->type == RIGID_BODY_STATIC || !body->is_active) {
    return;
  }

  // 1. Core Force Integration (Linear)
  if (body->type == RIGID_BODY_DYNAMIC) {
    body->velocity[0] += gravity.x * dt;
    body->velocity[1] += gravity.y * dt;
    body->velocity[2] += gravity.z * dt;
  }

  if (body->type == RIGID_BODY_DYNAMIC && body->inv_mass > 0.0f) {
    body->velocity[0] += (body->accumulated_force[0] * body->inv_mass) * dt;
    body->velocity[1] += (body->accumulated_force[1] * body->inv_mass) * dt;
    body->velocity[2] += (body->accumulated_force[2] * body->inv_mass) * dt;
  }

  // 2. Core Torque Integration (Angular: ω' = ω + I⁻¹ * τ * dt)
  if (body->type == RIGID_BODY_DYNAMIC) {
    // Simplified: assume identity inertia for now or constant
    body->angular_velocity[0] += body->accumulated_torque[0] * dt;
    body->angular_velocity[1] += body->accumulated_torque[1] * dt;
    body->angular_velocity[2] += body->accumulated_torque[2] * dt;
  }

  // Clear forces and torques
  body->accumulated_force[0] = 0.0f;
  body->accumulated_force[1] = 0.0f;
  body->accumulated_force[2] = 0.0f;
  body->accumulated_torque[0] = 0.0f;
  body->accumulated_torque[1] = 0.0f;
  body->accumulated_torque[2] = 0.0f;

  // 3. Apply Damping
  f32 l_damping = 1.0f - (body->linear_damping * dt);
  if (l_damping < 0.0f)
    l_damping = 0.0f;
  body->velocity[0] *= l_damping;
  body->velocity[1] *= l_damping;
  body->velocity[2] *= l_damping;

  f32 a_damping = 1.0f - (body->angular_damping * dt);
  if (a_damping < 0.0f)
    a_damping = 0.0f;
  body->angular_velocity[0] *= a_damping;
  body->angular_velocity[1] *= a_damping;
  body->angular_velocity[2] *= a_damping;

  // 4. Retrieve contacts resolved by collision system
  ContactPoint contacts[16];
  u32 contact_count =
      collision_system_get_contacts_for_body(body->id, contacts, 16);

  // Apply collision response
  for (u32 i = 0; i < contact_count; i++) {
    ContactPoint *contact = &contacts[i];
    float velocity_dot_normal = body->velocity[0] * contact->normal[0] +
                                body->velocity[1] * contact->normal[1] +
                                body->velocity[2] * contact->normal[2];

    if (velocity_dot_normal < 0.0f) {
      // Bounce
      float j = -(1.0f + body->restitution) * velocity_dot_normal;
      body->velocity[0] += j * contact->normal[0];
      body->velocity[1] += j * contact->normal[1];
      body->velocity[2] += j * contact->normal[2];

      // Friction
      body->velocity[0] *= (1.0f - body->friction);
      body->velocity[2] *= (1.0f - body->friction);

      // Positional correction
      body->position[0] +=
          contact->normal[0] * contact->penetration_depth * 0.8f;
      body->position[1] +=
          contact->normal[1] * contact->penetration_depth * 0.8f;
      body->position[2] +=
          contact->normal[2] * contact->penetration_depth * 0.8f;
    }
  }

  // 5. Integrate Position and Orientation
  body->position[0] += body->velocity[0] * dt;
  body->position[1] += body->velocity[1] * dt;
  body->position[2] += body->velocity[2] * dt;

  // q' = q + 0.5 * ω_quat * q * dt
  Vec3 angular_vel = {body->angular_velocity[0], body->angular_velocity[1],
                      body->angular_velocity[2]};
  if (vec3_length_squared(angular_vel) > 1e-6f) {
    Vec3 rot_vec = vec3_scale(angular_vel, 0.5f * dt);
    Quat rotation_change = quat_from_angular_velocity(&rot_vec);
    Quat current_rot = {body->rotation[0], body->rotation[1], body->rotation[2],
                        body->rotation[3]};
    Quat new_rot = quat_normalize(quat_mul(current_rot, rotation_change));
    body->rotation[0] = new_rot.x;
    body->rotation[1] = new_rot.y;
    body->rotation[2] = new_rot.z;
    body->rotation[3] = new_rot.w;
  }
}

// ... helper for raycast ...
static _Atomic uint32_t global_body_id_counter = ATOMIC_VAR_INIT(1);

// ============================================================================
// PHYSICS WORLD IMPLEMENTATION
// ============================================================================

PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = (PhysicsWorld *)calloc(1, sizeof(PhysicsWorld));
  if (!world)
    return NULL;

  world->body_capacity = 1024;
  world->bodies =
      (RigidBody **)calloc(world->body_capacity, sizeof(RigidBody *));
  world->constraints = (Constraint *)calloc(1024, sizeof(Constraint));

  world->gravity[0] = config.gravity.x;
  world->gravity[1] = config.gravity.y;
  world->gravity[2] = config.gravity.z;
  world->timestep =
      config.fixed_timestep > 0 ? config.fixed_timestep : 1.0f / 60.0f;
  world->velocity_iterations = config.velocity_iterations;
  world->position_iterations = config.position_iterations;

  collision_system_init();

  return world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world)
    return;
  if (world->bodies)
    free(world->bodies);
  if (world->constraints)
    free(world->constraints);
  free(world);
}

void physics_world_step(PhysicsWorld *world, f32 dt) {
  if (!world || world->paused)
    return;

  world->time_accumulator += dt;

  while (world->time_accumulator >= world->timestep) {
    // 1. Update Collision System
    collision_system_clear_bodies();

    // Add active bodies
    for (uint32_t i = 0; i < world->body_count; i++) {
      RigidBody *body = world->bodies[i];
      if (body && body->is_active && body->shape) {
        AABB bounds = calculate_aabb(body);
        Vec3 pos = {body->position[0], body->position[1], body->position[2]};
        collision_system_add_body(body->id, &bounds, pos);
      }
    }

    // Add Ground Plane (Static Body)
    // ID 999999
    {
      AABB ground_bounds = {{-1000.0f, -50.0f, -1000.0f},
                            {1000.0f, 0.0f, 1000.0f}};
      Vec3 ground_pos = {0.0f, -25.0f, 0.0f};
      collision_system_add_body(999999, &ground_bounds, ground_pos);
    }

    collision_system_update(world->timestep);

    // 2. Integrate
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
  if (world->body_count >= world->body_capacity)
    return NULL; // simplified

  world->bodies[world->body_count++] = body;
  return body;
}

void physics_world_remove_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i] == body) {
      world->bodies[i] = world->bodies[--world->body_count];
      return;
    }
  }
}

// RigidBody creation
RigidBody *rigid_body_create(BodyType type, Vec3 position) {
  RigidBody *body = (RigidBody *)calloc(1, sizeof(RigidBody));
  if (!body)
    return NULL;

  body->id = atomic_fetch_add(&global_body_id_counter, 1);
  // Map type...
  body->type =
      (type == BODY_TYPE_STATIC) ? RIGID_BODY_STATIC : RIGID_BODY_DYNAMIC;

  body->position[0] = position.x;
  body->position[1] = position.y;
  body->position[2] = position.z;

  body->mass = 1.0f;
  body->inv_mass = 1.0f;
  body->friction = 0.5f;
  body->restitution = 0.5f;
  body->is_active = true;
  body->rotation[3] = 1.0f; // Identity quaternion

  if (type == BODY_TYPE_STATIC) {
    body->inv_mass = 0.0f;
    body->mass = 0.0f;
  }

  return body;
}

void rigid_body_destroy(RigidBody *body) { free(body); }

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
  if (body)
    body->shape = (CollisionShape *)collider;
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

void rigid_body_set_velocity(RigidBody *body, Vec3 velocity) {
  if (body) {
    body->velocity[0] = velocity.x;
    body->velocity[1] = velocity.y;
    body->velocity[2] = velocity.z;
  }
}

void physics_world_init(PhysicsWorld *world, const Vec3 *gravity) {
  if (!world)
    return;
  if (gravity) {
    world->gravity[0] = gravity->x;
    world->gravity[1] = gravity->y;
    world->gravity[2] = gravity->z;
  } else {
    world->gravity[0] = 0.0f;
    world->gravity[1] = -9.81f;
    world->gravity[2] = 0.0f;
  }
  world->timestep = 1.0f / 60.0f;
  world->velocity_iterations = 8;
  world->position_iterations = 3;
}

void physics_world_free(PhysicsWorld *world) { physics_world_destroy(world); }

void physics_world_add_constraint(PhysicsWorld *world, Constraint *constraint) {
  if (!world || !constraint)
    return;
  if (world->constraint_count < 1024) {
    world->constraints[world->constraint_count++] = *constraint;
  }
}

bool physics_world_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                           f32 max_distance, void *out_hit) {
  return false; // stub
}

void rigid_body_set_mass(RigidBody *body, f32 mass) {
  if (body) {
    body->mass = mass;
    body->inv_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
  }
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
  if (body)
    body->restitution = restitution;
}

void rigid_body_set_friction(RigidBody *body, f32 friction) {
  if (body)
    body->friction = friction;
}

// Property setters and getters
void rigid_body_set_position(RigidBody *body, Vec3 position) {
  if (body) {
    body->position[0] = position.x;
    body->position[1] = position.y;
    body->position[2] = position.z;
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

Quat rigid_body_get_rotation(const RigidBody *body) {
  if (!body)
    return quat_identity();
  return (Quat){body->rotation[0], body->rotation[1], body->rotation[2],
                body->rotation[3]};
}

void rigid_body_set_linear_damping(RigidBody *body, f32 damping) {
  if (body)
    body->linear_damping = damping;
}

void rigid_body_set_angular_damping(RigidBody *body, f32 damping) {
  if (body)
    body->angular_damping = damping;
}

void rigid_body_add_force(RigidBody *body, Vec3 force) {
  if (body) {
    body->accumulated_force[0] += force.x;
    body->accumulated_force[1] += force.y;
    body->accumulated_force[2] += force.z;
  }
}

void rigid_body_add_impulse(RigidBody *body, Vec3 impulse) {
  if (body && body->inv_mass > 0.0f) {
    body->velocity[0] += impulse.x * body->inv_mass;
    body->velocity[1] += impulse.y * body->inv_mass;
    body->velocity[2] += impulse.z * body->inv_mass;
  }
}

void rigid_body_clear_forces(RigidBody *body) {
  if (body) {
    body->accumulated_force[0] = 0.0f;
    body->accumulated_force[1] = 0.0f;
    body->accumulated_force[2] = 0.0f;
    body->accumulated_torque[0] = 0.0f;
    body->accumulated_torque[1] = 0.0f;
    body->accumulated_torque[2] = 0.0f;
  }
}

f32 rigid_body_get_inv_mass(const RigidBody *body) {
  return body ? body->inv_mass : 0.0f;
}

Collider *rigid_body_get_collider(const RigidBody *body) {
  return (Collider *)(body ? body->shape : NULL);
}

// -----------------------------------------------------------------------------
// Character Controller Implementation
// -----------------------------------------------------------------------------

CharacterController *character_controller_create(PhysicsWorld *world,
                                                 Vec3 position) {
  return NULL; // stub
}

void character_controller_destroy(CharacterController *controller) {}

void character_controller_set_move_speed(CharacterController *controller,
                                         f32 speed) {}
void character_controller_set_jump_height(CharacterController *controller,
                                          f32 height) {}
void character_controller_set_step_height(CharacterController *controller,
                                          f32 height) {}
void character_controller_set_slope_limit(CharacterController *controller,
                                          f32 angle_degrees) {}
void character_controller_set_air_control(CharacterController *controller,
                                          f32 air_control) {}

bool character_controller_is_grounded(CharacterController *controller) {
  return false;
}
Vec3 character_controller_get_velocity(CharacterController *controller) {
  return (Vec3){0};
}
Vec3 character_controller_get_position(CharacterController *controller) {
  return (Vec3){0};
}

void character_controller_move(CharacterController *controller,
                               PhysicsWorld *world, Vec3 direction,
                               f32 delta_time) {}
void character_controller_jump(CharacterController *controller,
                               PhysicsWorld *world) {}
void character_controller_update(CharacterController *controller,
                                 PhysicsWorld *world, f32 delta_time) {}

void character_controller_teleport(CharacterController *controller,
                                   Vec3 position) {}
void character_controller_reset(CharacterController *controller) {}

void character_controller_init_system(void) {}
void character_controller_cleanup_system(void) {}
void character_controller_update_all(PhysicsWorld *world, f32 delta_time) {}
