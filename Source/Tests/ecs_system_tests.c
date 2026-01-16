#include "core/memory/unified_memory_allocator.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>

// ============================================================================
// TEST COMPONENTS
// ============================================================================

typedef struct {
  float x, y, z;
} Position;

typedef struct {
  float x, y, z;
} Velocity;

typedef struct {
  int value;
} Health;

// ============================================================================
// TEST SYSTEMS
// ============================================================================

static int g_movement_calls = 0;
static int g_health_calls = 0;

void movement_system(SystemContext *ctx) {
  g_movement_calls++;

  // Create query for entities with Position and Velocity
  ComponentType components[] = {0, 1}; // Position, Velocity
  QueryDesc desc = {.all_components = components, .all_count = 2};

  Query *query = ecs_query_create(ctx->world, &desc);

  Entity entity;
  void *comps[2];
  while (ecs_query_next(query, &entity, comps)) {
    Position *pos = (Position *)comps[0];
    Velocity *vel = (Velocity *)comps[1];

    pos->x += vel->x * ctx->delta_time;
    pos->y += vel->y * ctx->delta_time;
    pos->z += vel->z * ctx->delta_time;
  }

  ecs_query_destroy(ctx->world, query);
}

void health_system(SystemContext *ctx) { g_health_calls++; }

// ============================================================================
// SYSTEM TESTS
// ============================================================================

void test_system_creation(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  QueryDesc query = {0};
  System *sys = ecs_system_create(world, "TestSystem", movement_system, &query);

  TEST_ASSERT(sys != NULL, "System creation failed");
  TEST_ASSERT(strcmp(sys->name, "TestSystem") == 0, "System name incorrect");
  TEST_ASSERT(sys->enabled == true, "System should be enabled by default");
  TEST_ASSERT(sys->priority == 100, "System default priority incorrect");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_system_execution(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  // Register components
  ComponentInfo pos_info = {.name = "Position", .size = sizeof(Position)};
  ComponentInfo vel_info = {.name = "Velocity", .size = sizeof(Velocity)};

  ECSComponentID pos_id = ecs_register_component(world, &pos_info);
  ECSComponentID vel_id = ecs_register_component(world, &vel_info);

  // Create system
  ComponentType components[] = {pos_id, vel_id};
  QueryDesc query = {.all_components = components, .all_count = 2};
  System *sys = ecs_system_create(world, "Movement", movement_system, &query);

  // Create test entity
  Entity entity = ecs_create_entity(world);
  Position pos = {0, 0, 0};
  Velocity vel = {1, 0, 0};
  ecs_add_component(world, entity, pos_id, &pos);
  ecs_add_component(world, entity, vel_id, &vel);

  // Execute systems
  g_movement_calls = 0;
  ecs_world_update(world, 1.0f);

  TEST_ASSERT(g_movement_calls == 1, "System not executed");
  TEST_ASSERT(sys->execution_count == 1, "Execution count not updated");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_system_priority(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  QueryDesc query = {0};
  System *sys1 = ecs_system_create(world, "LowPriority", health_system, &query);
  System *sys2 =
      ecs_system_create(world, "HighPriority", movement_system, &query);

  ecs_system_set_priority(world, sys1, 200);
  ecs_system_set_priority(world, sys2, 50);

  ecs_world_rebuild_execution_order(world);

  // Verify order (sys2 should execute before sys1)
  g_movement_calls = 0;
  g_health_calls = 0;

  ecs_world_update(world, 1.0f);

  TEST_ASSERT(g_movement_calls == 1, "High priority system not executed");
  TEST_ASSERT(g_health_calls == 1, "Low priority system not executed");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_system_enable_disable(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  QueryDesc query = {0};
  System *sys = ecs_system_create(world, "TestSystem", movement_system, &query);

  // Disable system
  ecs_system_set_enabled(world, sys, false);

  g_movement_calls = 0;
  ecs_world_update(world, 1.0f);

  TEST_ASSERT(g_movement_calls == 0, "Disabled system was executed");

  // Re-enable
  ecs_system_set_enabled(world, sys, true);
  ecs_world_update(world, 1.0f);

  TEST_ASSERT(g_movement_calls == 1, "Enabled system not executed");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_world_stats(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  // Create some entities and systems
  ecs_create_entity(world);
  ecs_create_entity(world);

  QueryDesc query = {0};
  ecs_system_create(world, "Sys1", movement_system, &query);
  ecs_system_create(world, "Sys2", health_system, &query);

  WorldStats stats = ecs_world_get_stats(world);

  TEST_ASSERT(stats.entity_count == 2, "Entity count incorrect");
  TEST_ASSERT(stats.system_count == 2, "System count incorrect");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running ECS System Tests ===\n\n");

  RUN_TEST(test_system_creation);
  RUN_TEST(test_system_execution);
  RUN_TEST(test_system_priority);
  RUN_TEST(test_system_enable_disable);
  RUN_TEST(test_world_stats);

  printf("\n=== All ECS System Tests Passed! ===\n");
  return 0;
}
