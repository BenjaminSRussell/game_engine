#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/ai_components.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>

void test_ai_components(void) {
  unified_memory_init(NULL);
  World *world = ecs_world_create(NULL);

  register_ai_components(world);

  // Test Agent Creation
  Entity e = ecs_create_entity(world);
  AgentComponent ag = agent_create(1, 5.0f, 0.8f);

  ecs_add_component(world, e, g_agent_component_id, &ag);

  // Test Retrieve
  AgentComponent *r_ag =
      (AgentComponent *)ecs_get_component(world, e, g_agent_component_id);
  TEST_ASSERT(r_ag != NULL, "Agent retrieve failed");
  TEST_ASSERT(r_ag->speed == 5.0f, "Speed mismatch");

  // Test Pathfinding
  PathfindingComponent pf = pathfinding_create();
  ecs_add_component(world, e, g_pathfinding_component_id, &pf);
  TEST_ASSERT(ecs_has_component(world, e, g_pathfinding_component_id),
              "Pathfinding missing");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== AI Component Tests ===\n");
  RUN_TEST(test_ai_components);
  printf("Tests Passed\n");
  return 0;
}
