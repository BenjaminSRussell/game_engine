#include "../Public/unified_physics.h"
#include "physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unified_logger.h>
#include <unified_memory.h>

// Utility for time
static f64 get_current_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = UNIFIED_ALLOC(sizeof(PhysicsWorld));
  if (!world) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate physics world");
    return NULL;
  }

  memset(world, 0, sizeof(PhysicsWorld));

  world->gravity = config.gravity;
  world->fixed_timestep = config.fixed_timestep;
  world->velocity_iterations = config.velocity_iterations;
  world->position_iterations = config.position_iterations;

  // Initialize body storage
  world->body_capacity = 1024;
  world->bodies = UNIFIED_ALLOC(sizeof(RigidBody) * world->body_capacity);
  if (!world->bodies) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate body storage");
    UNIFIED_FREE(world);
    return NULL;
  }

  // Initialize manifold storage
  world->manifold_capacity = 2048;
  world->manifolds =
      UNIFIED_ALLOC(sizeof(CollisionManifold) * world->manifold_capacity);
  if (!world->manifolds) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate manifold storage");
    UNIFIED_FREE(world->bodies);
    UNIFIED_FREE(world);
    return NULL;
  }

  // Initialize threading
  world->solver_mutex = UNIFIED_ALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init((pthread_mutex_t *)world->solver_mutex, NULL);

  LOG_INFO(LOG_CAT_PHYSICS, "Physics world created successfully");
  return world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world)
    return;

  // Destroy all bodies
  for (u32 i = 0; i < world->body_count; i++) {
    if (world->bodies[i].collider) {
      collider_destroy(world->bodies[i].collider);
    }
  }

  // Cleanup resources
  UNIFIED_FREE(world->bodies);
  UNIFIED_FREE(world->manifolds);

  if (world->solver_mutex) {
    pthread_mutex_destroy((pthread_mutex_t *)world->solver_mutex);
    UNIFIED_FREE(world->solver_mutex);
  }

  UNIFIED_FREE(world);
  LOG_INFO(LOG_CAT_PHYSICS, "Physics world destroyed");
}

RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return NULL;

  if (world->body_count >= world->body_capacity) {
    // Expand capacity
    world->body_capacity *= 2;
    RigidBody *new_bodies = UNIFIED_REALLOC(
        world->bodies, sizeof(RigidBody) * world->body_capacity);
    if (!new_bodies) {
      LOG_ERROR(LOG_CAT_PHYSICS, "Failed to expand body storage");
      return NULL;
    }
    world->bodies = new_bodies;
  }

  world->bodies[world->body_count] = *body;
  return &world->bodies[world->body_count++];
}

void physics_world_remove_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return;

  // Find and remove body
  for (u32 i = 0; i < world->body_count; i++) {
    if (&world->bodies[i] == body) {
      // Shift remaining bodies
      memmove(&world->bodies[i], &world->bodies[i + 1],
              sizeof(RigidBody) * (world->body_count - i - 1));
      world->body_count--;
      return;
    }
  }
}

void physics_world_step(PhysicsWorld *world, f32 delta_time) {
  if (!world)
    return;

  f64 start_time = get_current_time_ms();

  // Fixed timestep with accumulator
  static f32 accumulator = 0.0f;
  accumulator += delta_time;

  while (accumulator >= world->fixed_timestep) {
    // Apply forces
    for (u32 i = 0; i < world->body_count; i++) {
      RigidBody *body = &world->bodies[i];
      if (body->type == BODY_TYPE_DYNAMIC) {
        // Apply gravity
        body->force = vec3_add(body->force, world->gravity);

        // Update acceleration
        body->acceleration = vec3_mul(body->force, body->inv_mass);
        body->angular_acceleration = vec3_mul(body->torque, body->inv_inertia);

        // Integrate velocity
        body->velocity =
            vec3_add(body->velocity,
                     vec3_mul(body->acceleration, world->fixed_timestep));
        body->angular_velocity = vec3_add(
            body->angular_velocity,
            vec3_mul(body->angular_acceleration, world->fixed_timestep));

        // Apply damping
        body->velocity =
            vec3_mul(body->velocity,
                     1.0f - body->linear_damping * world->fixed_timestep);
        body->angular_velocity =
            vec3_mul(body->angular_velocity,
                     1.0f - body->angular_damping * world->fixed_timestep);

        // Clear forces
        body->force = (Vec3){0, 0, 0};
        body->torque = (Vec3){0, 0, 0};
      }
    }

    // Broadphase collision detection
    world->manifold_count = 0;
    for (u32 i = 0; i < world->body_count; i++) {
      for (u32 j = i + 1; j < world->body_count; j++) {
        CollisionManifold manifold;
        if (physics_check_collision(&world->bodies[i], &world->bodies[j],
                                    &manifold)) {
          if (world->manifold_count < world->manifold_capacity) {
            world->manifolds[world->manifold_count++] = manifold;
          }
        }
      }
    }

    // Solve collisions
    pthread_mutex_lock((pthread_mutex_t *)world->solver_mutex);
    for (u32 iteration = 0; iteration < world->velocity_iterations;
         iteration++) {
      for (u32 i = 0; i < world->manifold_count; i++) {
        physics_resolve_collision(world->manifolds[i].body_a,
                                  world->manifolds[i].body_b,
                                  &world->manifolds[i]);
      }
    }
    pthread_mutex_unlock((pthread_mutex_t *)world->solver_mutex);

    // Position correction
    for (u32 iteration = 0; iteration < world->position_iterations;
         iteration++) {
      for (u32 i = 0; i < world->manifold_count; i++) {
        // Position correction using Baumgarte stabilization
        CollisionManifold *manifold = &world->manifolds[i];
        const f32 percent = 0.2f;
        const f32 slop = 0.01f;
        Vec3 correction =
            vec3_mul(manifold->normal, (manifold->depth - slop) * percent /
                                           (manifold->body_a->inv_mass +
                                            manifold->body_b->inv_mass));

        manifold->body_a->position =
            vec3_add(manifold->body_a->position,
                     vec3_mul(correction, manifold->body_a->inv_mass));
        manifold->body_b->position =
            vec3_sub(manifold->body_b->position,
                     vec3_mul(correction, manifold->body_b->inv_mass));
      }
    }

    // Integrate positions
    for (u32 i = 0; i < world->body_count; i++) {
      RigidBody *body = &world->bodies[i];
      if (body->type == BODY_TYPE_DYNAMIC) {
        body->position = vec3_add(
            body->position, vec3_mul(body->velocity, world->fixed_timestep));

        // Update rotation (simplified)
        f32 angle = vec3_length(body->angular_velocity) * world->fixed_timestep;
        if (angle > 0.0f) {
          Vec3 axis = vec3_normalize(body->angular_velocity);
          Quat rotation = quat_from_axis_angle(axis, angle);
          body->rotation = quat_mul(body->rotation, rotation);
        }
      }
    }

    accumulator -= world->fixed_timestep;
  }

  // Update performance stats
  f64 end_time = get_current_time_ms();
  world->performance.total_time_ms = end_time - start_time;
  world->performance.bodies_processed = world->body_count;
  world->performance.collision_pairs = world->manifold_count;
}

RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                              f32 max_distance) {
  RaycastResult result = {0};
  result.origin = origin;
  result.direction = direction;
  result.max_distance = max_distance;
  result.hit = false;
  result.hit_distance = max_distance;

  if (!world)
    return result;

  Vec3 normalized_dir = vec3_normalize(direction);
  f32 closest_distance = max_distance;

  for (u32 i = 0; i < world->body_count; i++) {
    RigidBody *body = &world->bodies[i];
    if (!body->collider)
      continue;

    // Simple sphere intersection for now
    if (body->collider->type == COLLIDER_TYPE_SPHERE) {
      f32 radius = body->collider->shape.sphere.radius;
      Vec3 to_center = vec3_sub(body->position, origin);
      f32 projection = vec3_dot(to_center, normalized_dir);

      if (projection < 0.0f || projection > closest_distance)
        continue;

      Vec3 closest_point =
          vec3_add(origin, vec3_mul(normalized_dir, projection));

      Vec3 dist_vec = vec3_sub(closest_point, body->position);
      f32 distance_to_center = vec3_length(dist_vec);

      if (distance_to_center <= radius) {
        f32 hit_distance =
            projection -
            sqrtf(radius * radius - distance_to_center * distance_to_center);
        if (hit_distance < closest_distance && hit_distance >= 0.0f) {
          closest_distance = hit_distance;
          result.hit = true;
          result.hit_distance = hit_distance;
          result.hit_point = closest_point;
          Vec3 normal_vec = vec3_sub(closest_point, body->position);
          result.hit_normal = vec3_normalize(normal_vec);
          result.hit_body = body;
        }
      }
    }
  }

  return result;
}
