#include "ecs/systems/ai_systems.h"
#include "ecs/components/ai_components.h"
#include "ecs/components/physics_components.h" // For Rigidbody movement
#include "engine/include/core/logger.h"
#include "engine/include/math/math_all.h"
#include <stdio.h>

// Mock Transform/Physics query for now
// In real impl: Use Physics Raycast/OverlapSphere

// ============================================================================
// PERCEPTION SYSTEM
// ============================================================================

void ai_perception_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Iterate Perception + Transform (conceptually)
  ComponentType types[] = {g_perception_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    PerceptionComponent *perception = (PerceptionComponent *)comps[0];

    // Mock Sense Update:
    // In real code: Physics.OverlapSphere(pos, view_dist) -> check FOV

    // Reset visible count
    perception->visible_count = 0;
    perception->can_see_target = true; // Pretend we see target for testing
  }

  ecs_query_destroy(ctx->world, query);
}

// ============================================================================
// DECISION SYSTEM
// ============================================================================

void ai_decision_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Iterate Agents
  ComponentType types[] = {g_agent_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    AgentComponent *agent = (AgentComponent *)comps[0];

    // Simple FSM
    switch (agent->state) {
    case AGENT_STATE_IDLE:
      // If aggressive, look for target
      if (agent->aggression > 0.5f) {
        // Check Perception (requires getting comp)
        // if (see_target) state = CHASE;
        agent->state = AGENT_STATE_PATROL; // Auto transition for test
      }
      break;

    case AGENT_STATE_PATROL:
      // Move around...
      break;

    case AGENT_STATE_CHASE:
      // Move to target...
      break;

    default:
      break;
    }
  }

  ecs_query_destroy(ctx->world, query);
}

// ============================================================================
// NAVIGATION SYSTEM
// ============================================================================

void ai_navigation_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Iterate Pathfinding + Rigidbody
  ComponentType types[] = {g_pathfinding_component_id,
                           g_rigidbody_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 2};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[2];

  while (ecs_query_next(query, &entity, comps)) {
    PathfindingComponent *path = (PathfindingComponent *)comps[0];
    RigidbodyComponent *rb = (RigidbodyComponent *)comps[1];

    if (path->waypoint_count == 0)
      continue;

    // Move towards current waypoint
    // In real code: Dir = Normalize(Target - Pos)
    // rb->velocity = Dir * speed

    // Mock movement
    rb->velocity.x = 1.0f; // Just move X for test
  }

  ecs_query_destroy(ctx->world, query);
}

// ============================================================================
// REGISTRATION
// ============================================================================

void register_ai_systems(World *world) {
  if (!world) {
    LOG_ERROR("[AI] Cannot register systems: null world");
    return;
  }

  // Perception (45)
  QueryDesc p_q = {0};
  System *perception =
      ecs_system_create(world, "Perception", ai_perception_system, &p_q);
  ecs_system_set_priority(world, perception, 45);

  // Decision (46)
  QueryDesc d_q = {0};
  System *decision =
      ecs_system_create(world, "Decision", ai_decision_system, &d_q);
  ecs_system_set_priority(world, decision, 46);

  // Navigation (47)
  QueryDesc n_q = {0};
  System *nav =
      ecs_system_create(world, "Navigation", ai_navigation_system, &n_q);
  ecs_system_set_priority(world, nav, 47);

  LOG_INFO("[AI] Registered systems:");
  LOG_INFO("  Perception (priority 45)");
  LOG_INFO("  Decision (priority 46)");
  LOG_INFO("  Navigation (priority 47)");
}
