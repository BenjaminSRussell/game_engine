#include <math.h> // For sqrtf, etc.
#include <physics/physics.h>
#include <physics/physics_engine_core.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static _Atomic uint32_t global_body_id_counter = ATOMIC_VAR_INIT(1);

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
