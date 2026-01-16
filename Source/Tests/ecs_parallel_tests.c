#include "core/memory/frame_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "core/thread_pool.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>
#include <unistd.h>

// ============================================================================
// TEST COMPONENTS
// ============================================================================

typedef struct {
  float x, y, z;
} Transform;

typedef struct {
  float vx, vy, vz;
} Velocity;

// ============================================================================
// TEST SYSTEMS
// ============================================================================

static volatile int g_parallel_calls = 0;

void parallel_movement_system(SystemContext *ctx) {
  __sync_fetch_and_add(&g_parallel_calls, 1);

  // Simulate work
  usleep(10000); // 10ms
}

void serial_system(SystemContext *ctx) {
  // Quick system
}

// ============================================================================
// PARALLEL EXECUTION TESTS
// ============================================================================

void test_parallel_system_execution(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  World *world = ecs_world_create(NULL);

  // Create parallel systems
  QueryDesc query = {0};
  System *sys1 =
      ecs_system_create(world, "Parallel1", parallel_movement_system, &query);
  System *sys2 =
      ecs_system_create(world, "Parallel2", parallel_movement_system, &query);
  System *sys3 =
      ecs_system_create(world, "Parallel3", parallel_movement_system, &query);

  ecs_system_set_parallel(world, sys1, true);
  ecs_system_set_parallel(world, sys2, true);
  ecs_system_set_parallel(world, sys3, true);

  // Execute in parallel
  g_parallel_calls = 0;
  ecs_world_execute_parallel(world, 1.0f);

  // Wait a bit for jobs to complete
  usleep(100000); // 100ms

  TEST_ASSERT(g_parallel_calls == 3, "Not all parallel systems executed");

  frame_allocator_reset();
  thread_pool_shutdown();
  ecs_world_destroy(world);
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_mixed_serial_parallel(void) {
  unified_memory_init(NULL);
  frame_allocator_init();
  thread_pool_init(4);

  World *world = ecs_world_create(NULL);

  QueryDesc query = {0};
  System *serial = ecs_system_create(world, "Serial", serial_system, &query);
  System *parallel =
      ecs_system_create(world, "Parallel", parallel_movement_system, &query);

  ecs_system_set_parallel(world, parallel, true);

  // Execute
  ecs_world_execute_parallel(world, 1.0f);

  TEST_ASSERT(serial->execution_count == 1, "Serial system not executed");

  usleep(50000); // Wait for parallel

  frame_allocator_reset();
  thread_pool_shutdown();
  ecs_world_destroy(world);
  frame_allocator_shutdown();
  unified_memory_shutdown();
}

void test_worker_thread_config(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  // Set worker threads
  ecs_world_set_worker_threads(world, 8);

  ThreadPool *pool = thread_pool_get_global();
  TEST_ASSERT(pool != NULL, "Thread pool not initialized");

  thread_pool_shutdown();
  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_query_count(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  // Register component
  ComponentInfo info = {.name = "Transform", .size = sizeof(Transform)};
  ECSComponentID id = ecs_register_component(world, &info);

  // Create entities
  for (int i = 0; i < 10; i++) {
    Entity e = ecs_create_entity(world);
    Transform t = {0, 0, 0};
    ecs_add_component(world, e, id, &t);
  }

  // Create query
  ComponentType components[] = {id};
  QueryDesc desc = {.all_components = components, .all_count = 1};
  Query *query = ecs_query_create(world, &desc);

  u32 count = ecs_query_count(query);
  TEST_ASSERT(count == 10, "Query count incorrect: expected 10, got %u", count);

  ecs_query_destroy(world, query);
  ecs_world_destroy(world);
  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running ECS Parallel Execution Tests ===\n\n");

  RUN_TEST(test_parallel_system_execution);
  RUN_TEST(test_mixed_serial_parallel);
  RUN_TEST(test_worker_thread_config);
  RUN_TEST(test_query_count);

  printf("\n=== All Parallel Execution Tests Passed! ===\n");
  return 0;
}
