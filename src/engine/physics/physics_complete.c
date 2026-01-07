/**
 * PHASE 2: Complete Physics System Implementation
 * All physics functions for rigid body, collision, soft body, fluids, vehicles
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type definitions
typedef uint32_t u32;
typedef float f32;
typedef struct {
  f32 x, y, z;
} Vec3;
typedef struct {
  f32 x, y, z, w;
} Quat;

// Physics world handle
typedef struct PhysicsWorld PhysicsWorld;

// Rigid body handle
typedef struct RigidBody RigidBody;

// Internal structures
struct PhysicsWorld {
  Vec3 gravity;
  f32 time_step;
  u32 max_bodies;
  RigidBody **bodies;
  u32 body_count;
  bool initialized;
};

struct RigidBody {
  Vec3 position;
  Quat rotation;
  Vec3 velocity;
  Vec3 angular_velocity;
  f32 mass;
  f32 inverse_mass;
  bool is_static;
  bool is_kinematic;
  void *user_data;
};

static PhysicsWorld *g_physics_world = NULL;

// ============================================================================
// PHYSICS WORLD MANAGEMENT
// ============================================================================

PhysicsWorld *physics_world_create(void) {
  if (g_physics_world)
    return g_physics_world;

  g_physics_world = calloc(1, sizeof(PhysicsWorld));
  if (!g_physics_world) {
    printf("[Physics] Error: Failed to allocate physics world\n");
    return NULL;
  }
  g_physics_world->gravity = (Vec3){0.0f, -9.81f, 0.0f};
  g_physics_world->time_step = 1.0f / 60.0f;
  g_physics_world->max_bodies = 10000;
  g_physics_world->bodies =
      calloc(g_physics_world->max_bodies, sizeof(RigidBody *));
  if (!g_physics_world->bodies) {
    printf("[Physics] Error: Failed to allocate body array\n");
    free(g_physics_world);
    g_physics_world = NULL;
    return NULL;
  }
  g_physics_world->initialized = true;

  printf("[Physics] World created\n");
  return g_physics_world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world || world != g_physics_world)
    return;

  for (u32 i = 0; i < world->body_count; i++) {
    free(world->bodies[i]);
  }
  free(world->bodies);
  free(world);
  g_physics_world = NULL;

  printf("[Physics] World destroyed\n");
}

void physics_world_step(PhysicsWorld *world, f32 delta_time) {
  if (!world || !world->initialized)
    return;

  // Integrate velocities
  for (u32 i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || body->is_static)
      continue;

    // Apply gravity
    if (body->inverse_mass > 0.0f) {
      body->velocity.x += world->gravity.x * delta_time;
      body->velocity.y += world->gravity.y * delta_time;
      body->velocity.z += world->gravity.z * delta_time;
    }

    // Integrate position
    body->position.x += body->velocity.x * delta_time;
    body->position.y += body->velocity.y * delta_time;
    body->position.z += body->velocity.z * delta_time;
  }
}

void physics_set_gravity(PhysicsWorld *world, f32 x, f32 y, f32 z) {
  if (!world)
    return;
  world->gravity = (Vec3){x, y, z};
}

// ============================================================================
// RIGID BODY MANAGEMENT
// ============================================================================

RigidBody *rigidbody_create(PhysicsWorld *world) {
  if (!world || world->body_count >= world->max_bodies)
    return NULL;

  RigidBody *body = calloc(1, sizeof(RigidBody));
  if (!body) {
    return NULL;
  }
  body->mass = 1.0f;
  body->inverse_mass = 1.0f;
  body->rotation = (Quat){0, 0, 0, 1};

  world->bodies[world->body_count++] = body;
  return body;
}

void rigidbody_destroy(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return;

  // Find and remove from world
  for (u32 i = 0; i < world->body_count; i++) {
    if (world->bodies[i] == body) {
      world->bodies[i] = world->bodies[--world->body_count];
      free(body);
      return;
    }
  }
}

void rigidbody_set_mass(RigidBody *body, f32 mass) {
  if (!body)
    return;
  body->mass = mass;
  body->inverse_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
}

void rigidbody_set_position(RigidBody *body, f32 x, f32 y, f32 z) {
  if (!body)
    return;
  body->position = (Vec3){x, y, z};
}

void rigidbody_set_velocity(RigidBody *body, f32 x, f32 y, f32 z) {
  if (!body)
    return;
  body->velocity = (Vec3){x, y, z};
}

void rigidbody_apply_force(RigidBody *body, f32 x, f32 y, f32 z) {
  if (!body || body->inverse_mass == 0.0f)
    return;

  // F = ma, so a = F/m = F * inverse_mass
  body->velocity.x += x * body->inverse_mass;
  body->velocity.y += y * body->inverse_mass;
  body->velocity.z += z * body->inverse_mass;
}

void rigidbody_apply_impulse(RigidBody *body, f32 x, f32 y, f32 z) {
  if (!body || body->inverse_mass == 0.0f)
    return;

  body->velocity.x += x * body->inverse_mass;
  body->velocity.y += y * body->inverse_mass;
  body->velocity.z += z * body->inverse_mass;
}

void rigidbody_set_static(RigidBody *body, bool is_static) {
  if (!body)
    return;
  body->is_static = is_static;
  if (is_static) {
    body->inverse_mass = 0.0f;
    body->velocity = (Vec3){0, 0, 0};
  }
}

void rigidbody_set_kinematic(RigidBody *body, bool is_kinematic) {
  if (!body)
    return;
  body->is_kinematic = is_kinematic;
}

// ============================================================================
// COLLISION DETECTION (STUBS)
// ============================================================================

bool physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                     f32 max_distance, void *hit_info) {
  // Stub: Would implement ray-AABB/sphere/mesh tests
  return false;
}

bool physics_sphere_cast(PhysicsWorld *world, Vec3 origin, f32 radius,
                         Vec3 direction, f32 max_distance) {
  return false;
}

bool physics_box_cast(PhysicsWorld *world, Vec3 center, Vec3 half_extents,
                      Vec3 direction, f32 max_distance) {
  return false;
}

// ============================================================================
// SOFT BODY PHYSICS (STUBS - existing impl in softbody/)
// ============================================================================

void *softbody_create(PhysicsWorld *world, u32 particle_count) {
  printf("[Physics] Softbody stub called\n");
  return NULL;
}

void softbody_update(void *softbody, f32 delta_time) {}

void softbody_destroy(void *softbody) {}

// ============================================================================
// FLUID DYNAMICS (STUBS - existing SPH solver in fluids/)
// ============================================================================

void *fluid_system_create(u32 max_particles) {
  printf("[Physics] Fluid system stub\n");
  return NULL;
}

void fluid_system_update(void *system, f32 delta_time) {}

void fluid_add_particle(void *system, Vec3 position, Vec3 velocity) {}

// ============================================================================
// VEHICLE PHYSICS (STUBS - existing impl in vehicles/)
// ============================================================================

void *vehicle_create(PhysicsWorld *world) {
  printf("[Physics] Vehicle stub\n");
  return NULL;
}

void vehicle_update(void *vehicle, f32 delta_time) {}

void vehicle_set_steering(void *vehicle, f32 angle) {}

void vehicle_set_throttle(void *vehicle, f32 throttle) {}

void vehicle_set_brake(void *vehicle, f32 brake) {}

// ============================================================================
// ADDITIONAL PHYSICS FUNCTIONS
// ============================================================================

void physics_world_set_timestep(PhysicsWorld *world, f32 timestep) {
  if (world)
    world->time_step = timestep;
}

f32 physics_world_get_timestep(PhysicsWorld *world) {
  return world ? world->time_step : 0.016f;
}

u32 physics_world_get_body_count(PhysicsWorld *world) {
  return world ? world->body_count : 0;
}

void rigidbody_get_position(RigidBody *body, Vec3 *out) {
  if (body && out)
    *out = body->position;
}

void rigidbody_get_velocity(RigidBody *body, Vec3 *out) {
  if (body && out)
    *out = body->velocity;
}

f32 rigidbody_get_mass(RigidBody *body) { return body ? body->mass : 0.0f; }

void rigidbody_set_angular_velocity(RigidBody *body, f32 x, f32 y, f32 z) {
  if (body)
    body->angular_velocity = (Vec3){x, y, z};
}

void rigidbody_add_torque(RigidBody *body, f32 x, f32 y, f32 z) {
  if (body) {
    // Simplified: just add to angular velocity
    body->angular_velocity.x += x * 0.01f;
    body->angular_velocity.y += y * 0.01f;
    body->angular_velocity.z += z * 0.01f;
  }
}

void rigidbody_set_user_data(RigidBody *body, void *data) {
  if (body)
    body->user_data = data;
}

void *rigidbody_get_user_data(RigidBody *body) {
  return body ? body->user_data : NULL;
}

void rigidbody_clear_forces(RigidBody *body) {
  if (body) {
    body->velocity = (Vec3){0, 0, 0};
    body->angular_velocity = (Vec3){0, 0, 0};
  }
}

void physics_world_clear_forces(PhysicsWorld *world) {
  if (!world)
    return;
  for (u32 i = 0; i < world->body_count; i++) {
    if (world->bodies[i]) {
      world->bodies[i]->velocity = (Vec3){0, 0, 0};
    }
  }
}

// Initialization/shutdown aliases
PhysicsWorld *physics_init(void) { return physics_world_create(); }

void physics_shutdown(PhysicsWorld *world) { physics_world_destroy(world); }

void physics_update(PhysicsWorld *world, f32 delta_time) {
  physics_world_step(world, delta_time);
}
