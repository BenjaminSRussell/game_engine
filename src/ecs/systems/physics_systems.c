#include "ecs/systems/physics_systems.h"
#include "ecs/components/physics_components.h"
#include "ecs/components/rendering_components.h" // For Transform (assumed existing or mocked)
#include "engine/include/core/logger.h"
#include "physics/physics_allocator.h"
#include <stdio.h>

// Placeholder Gravity
static Vec3 g_gravity = {0.0f, -9.81f, 0.0f};

// Placeholder Transform Component ID (assuming defined elsewhere or will be
// mocked) In a real integration, we'd include transform_components.h
extern u32 g_transform_component_id;

// Mock Transform for compilation if missing
#ifndef TRANSFORM_COMPONENT_DEFINED
typedef struct {
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
} TransformComponent;
#endif

// ============================================================================
// PHYSICS SIMULATION SYSTEM (Integration)
// ============================================================================

void physics_simulation_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  f32 dt = ctx->delta_time;
  if (dt <= 0.0001f)
    return; // Skip small steps

  // Query: Rigidbody + Transform (conceptually)
  // For now, we'll iterate just Rigidbodies and assume we can get transform
  // In real implementation: ComponentType types[] = {g_rigidbody_component_id,
  // g_transform_component_id};

  ComponentType types[] = {g_rigidbody_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    RigidbodyComponent *rb = (RigidbodyComponent *)comps[0];

    if (rb->is_kinematic || rb->mass <= 0.0f)
      continue;

    // Semi-implicit Euler Integration

    // 1. Apply Gravity
    if (rb->use_gravity) {
      rb->velocity.x += g_gravity.x * dt;
      rb->velocity.y += g_gravity.y * dt;
      rb->velocity.z += g_gravity.z * dt;
    }

    // 2. Apply Drag
    f32 drag_factor = 1.0f - (rb->drag * dt);
    if (drag_factor < 0)
      drag_factor = 0;
    rb->velocity.x *= drag_factor;
    rb->velocity.y *= drag_factor;
    rb->velocity.z *= drag_factor;

    // 3. Update Position (stub - requires Transform component)
    // In real code: transform->position += rb->velocity * dt;
    // For now, we simulate "velocity doing something"

    // Note: Writing actual position integration requires acquiring the
    // Transform component. Since we are mocking dependencies, we'll leave the
    // logic hook here. TransformComponent* tr = ecs_get_component(ctx->world,
    // entity, g_transform_component_id);
  }

  ecs_query_destroy(ctx->world, query);
}

// ============================================================================
// COLLISION DETECTION SYSTEM
// ============================================================================

void collision_detection_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // 1. Reset frame allocator for contacts
  physics_allocator_reset_frame();

  // 2. Broadphase (Placeholder)
  // In real implementation: Update AABBs, insert into BVH/Grid

  // 3. Narrowphase (Placeholder)
  // Iterate collider pairs

  // For this step, we just demonstrate the system structure
}

// ============================================================================
// REGISTRATION
// ============================================================================

void register_physics_systems(World *world) {
  if (!world) {
    LOG_ERROR("[Physics] Cannot register systems: null world");
    return;
  }

  // Simulation System (Priority 50 - update velocities)
  QueryDesc sim_query = {0}; // System creates own query
  System *sim_sys = ecs_system_create(world, "PhysicsSimulation",
                                      physics_simulation_system, &sim_query);
  ecs_system_set_priority(world, sim_sys, 50);

  // Collision System (Priority 60 - detect collisions)
  QueryDesc col_query = {0};
  System *col_sys = ecs_system_create(world, "CollisionDetection",
                                      collision_detection_system, &col_query);
  ecs_system_set_priority(world, col_sys, 60);
  // Collision is good candidate for parallel execution (Phase 6.4)
  ecs_system_set_parallel(world, col_sys, true);

  // Rebuild execution order
  ecs_world_rebuild_execution_order(world);

  LOG_INFO("[Physics] Registered systems:");
  LOG_INFO("  PhysicsSimulation (priority 50)");
  LOG_INFO("  CollisionDetection (priority 60, parallel)");
}
