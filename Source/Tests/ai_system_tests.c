#include "ai/ai_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/ai_components.h"
#include "ecs/components/physics_components.h"
#include "ecs/ecs.h"
#include "ecs/systems/ai_systems.h"
#include "test_framework.h"
#include <stdio.h>

void test_ai_logic(void) {
  unified_memory_init(NULL);
  ai_allocator_init();
  World *world = ecs_world_create(NULL);

  register_ai_components(world);
  register_physics_components(world); // Needed for Rigidbody
  register_ai_systems(world);

  // Create Agent
  Entity entity = ecs_create_entity(world);

  AgentComponent ag = agent_create(1, 5.0f, 1.0f); // High aggression
  ecs_add_component(world, entity, g_agent_component_id, &ag);

  RigidbodyComponent rb = rigidbody_create(1.0f);
  ecs_add_component(world, entity, g_rigidbody_component_id, &rb);

  PathfindingComponent pf = pathfinding_create();
  pf.waypoint_count = 1; // Give it a path to valid navigation
  ecs_add_component(world, entity, g_pathfinding_component_id, &pf);

  // Run systems
  ecs_world_update(world, 0.1f);

  // Verify Decision (State change)
  AgentComponent *r_ag =
      (AgentComponent *)ecs_get_component(world, entity, g_agent_component_id);
  TEST_ASSERT(r_ag->state == AGENT_STATE_PATROL, "State transition failed");

  // Verify Navigation (Velocity update)
  RigidbodyComponent *r_rb = (RigidbodyComponent *)ecs_get_component(
      world, entity, g_rigidbody_component_id);
  TEST_ASSERT(r_rb->velocity.x > 0.0f, "Navigation failed to move");

  ecs_world_destroy(world);
  ai_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== AI System Tests ===\n");
  RUN_TEST(test_ai_logic);
  printf("Tests Passed\n");
  return 0;
}
