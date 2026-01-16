#include "core/memory/common_pools.h"
#include "core/memory/frame_allocator.h"
#include "core/memory/memory_macros.h"
#include "core/memory/unified_memory_allocator.h"
#include "test_framework.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// BASIC ALLOCATION TESTS
// ============================================================================

void test_basic_allocation(void) {
  unified_memory_init(NULL);

  void *ptr = ALLOC(1024);
  TEST_ASSERT(ptr != NULL, "Basic allocation failed");

  FREE(ptr);
  unified_memory_shutdown();
}

void test_zero_allocation(void) {
  unified_memory_init(NULL);

  size_t size = 256;
  char *ptr = (char *)ALLOC_ZERO(size);
  TEST_ASSERT(ptr != NULL, "Zero allocation failed");

  // Verify memory is zeroed
  for (size_t i = 0; i < size; i++) {
    TEST_ASSERT(ptr[i] == 0, "Memory not zeroed");
  }

  FREE(ptr);
  unified_memory_shutdown();
}

void test_type_safe_allocation(void) {
  unified_memory_init(NULL);

  typedef struct {
    int x, y, z;
  } Vec3;

  Vec3 *vec = ALLOC_TYPE(Vec3);
  TEST_ASSERT(vec != NULL, "Type-safe allocation failed");

  vec->x = 1;
  vec->y = 2;
  vec->z = 3;

  TEST_ASSERT(vec->x == 1 && vec->y == 2 && vec->z == 3,
              "Type-safe allocation data corrupt");

  FREE(vec);
  unified_memory_shutdown();
}

void test_array_allocation(void) {
  unified_memory_init(NULL);

  int *array = ALLOC_ARRAY(int, 100);
  TEST_ASSERT(array != NULL, "Array allocation failed");

  for (int i = 0; i < 100; i++) {
    array[i] = i;
  }

  for (int i = 0; i < 100; i++) {
    TEST_ASSERT(array[i] == i, "Array data corrupt");
  }

  FREE(array);
  unified_memory_shutdown();
}

// ============================================================================
// POOL ALLOCATION TESTS
// ============================================================================

void test_pool_creation(void) {
  unified_memory_init(NULL);

  MemoryPoolConfig config = {
      .block_size = 128, .block_count = 10, .auto_expand = false};

  MemoryPool *pool = unified_memory_pool_create(&config);
  TEST_ASSERT(pool != NULL, "Pool creation failed");
  TEST_ASSERT(pool->free_blocks == 10, "Pool free blocks incorrect");

  unified_memory_pool_destroy(pool);
  unified_memory_shutdown();
}

void test_pool_allocation(void) {
  unified_memory_init(NULL);

  MemoryPoolConfig config = {
      .block_size = 128, .block_count = 10, .auto_expand = false};

  MemoryPool *pool = unified_memory_pool_create(&config);

  void *ptr1 = POOL_ALLOC(pool, 64);
  TEST_ASSERT(ptr1 != NULL, "Pool allocation 1 failed");
  TEST_ASSERT(pool->free_blocks == 9, "Pool free blocks incorrect after alloc");

  void *ptr2 = POOL_ALLOC(pool, 64);
  TEST_ASSERT(ptr2 != NULL, "Pool allocation 2 failed");
  TEST_ASSERT(pool->free_blocks == 8,
              "Pool free blocks incorrect after 2nd alloc");

  POOL_FREE(pool, ptr1);
  TEST_ASSERT(pool->free_blocks == 9, "Pool free blocks incorrect after free");

  POOL_FREE(pool, ptr2);
  TEST_ASSERT(pool->free_blocks == 10,
              "Pool free blocks incorrect after 2nd free");

  unified_memory_pool_destroy(pool);
  unified_memory_shutdown();
}

void test_common_pools(void) {
  unified_memory_init(NULL);
  common_pools_init();

  // Test small allocation
  void *small = common_pool_alloc(32);
  TEST_ASSERT(small != NULL, "Common pool small allocation failed");

  // Test medium allocation
  void *medium = common_pool_alloc(128);
  TEST_ASSERT(medium != NULL, "Common pool medium allocation failed");

  // Test large allocation
  void *large = common_pool_alloc(512);
  TEST_ASSERT(large != NULL, "Common pool large allocation failed");

  common_pool_free(small, 32);
  common_pool_free(medium, 128);
  common_pool_free(large, 512);

  common_pools_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// STACK ALLOCATOR TESTS
// ============================================================================

void test_stack_creation(void) {
  unified_memory_init(NULL);

  StackAllocator *stack = unified_memory_stack_create(4096);
  TEST_ASSERT(stack != NULL, "Stack creation failed");
  TEST_ASSERT(stack->buffer_size == 4096, "Stack size incorrect");
  TEST_ASSERT(stack->current_offset == 0, "Stack offset should be 0");

  unified_memory_stack_destroy(stack);
  unified_memory_shutdown();
}

void test_stack_allocation(void) {
  unified_memory_init(NULL);

  StackAllocator *stack = unified_memory_stack_create(4096);

  void *ptr1 = STACK_ALLOC(stack, 256);
  TEST_ASSERT(ptr1 != NULL, "Stack allocation 1 failed");

  void *ptr2 = STACK_ALLOC(stack, 512);
  TEST_ASSERT(ptr2 != NULL, "Stack allocation 2 failed");

  TEST_ASSERT(stack->current_offset > 0, "Stack offset should be > 0");

  unified_memory_stack_reset(stack);
  TEST_ASSERT(stack->current_offset == 0, "Stack reset failed");

  unified_memory_stack_destroy(stack);
  unified_memory_shutdown();
}

void test_frame_allocator(void) {
  unified_memory_init(NULL);
  frame_allocator_init();

  void *ptr1 = FRAME_ALLOC(256);
  TEST_ASSERT(ptr1 != NULL, "Frame allocation 1 failed");

  void *ptr2 = FRAME_ALLOC(512);
  TEST_ASSERT(ptr2 != NULL, "Frame allocation 2 failed");

  size_t usage_before = frame_allocator_get_usage();
  TEST_ASSERT(usage_before > 0, "Frame allocator usage should be > 0");

  frame_allocator_reset();
  size_t usage_after = frame_allocator_get_usage();
  TEST_ASSERT(usage_after == 0, "Frame allocator reset failed");

  frame_allocator_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// ARENA ALLOCATOR TESTS
// ============================================================================

void test_arena_creation(void) {
  unified_memory_init(NULL);

  ArenaAllocator *arena = unified_memory_arena_create(4096);
  TEST_ASSERT(arena != NULL, "Arena creation failed");
  TEST_ASSERT(arena->block_count == 1, "Arena should have 1 block");

  unified_memory_arena_destroy(arena);
  unified_memory_shutdown();
}

void test_arena_allocation(void) {
  unified_memory_init(NULL);

  ArenaAllocator *arena = unified_memory_arena_create(1024);

  void *ptr1 = ARENA_ALLOC(arena, 256);
  TEST_ASSERT(ptr1 != NULL, "Arena allocation 1 failed");

  void *ptr2 = ARENA_ALLOC(arena, 512);
  TEST_ASSERT(ptr2 != NULL, "Arena allocation 2 failed");

  // This should trigger a new block
  void *ptr3 = ARENA_ALLOC(arena, 1024);
  TEST_ASSERT(ptr3 != NULL, "Arena allocation 3 failed");
  TEST_ASSERT(arena->block_count == 2, "Arena should have grown to 2 blocks");

  unified_memory_arena_destroy(arena);
  unified_memory_shutdown();
}

// ============================================================================
// LEAK DETECTION TESTS
// ============================================================================

void test_leak_detection(void) {
  MemoryPolicy policy = {.enable_leak_detection = true,
                         .enable_canaries = true};
  unified_memory_init(&policy);

  // Allocate and intentionally leak
  void *leak = ALLOC_TRACKED(256);
  (void)leak; // Suppress unused warning

  // Check leaks (should detect 1)
  printf("\\nExpecting 1 leak to be detected:\\n");
  unified_memory_check_leaks();

  // Clean up the leak
  FREE(leak);

  unified_memory_shutdown();
}

// ============================================================================
// STATISTICS TESTS
// ============================================================================

void test_statistics(void) {
  unified_memory_init(NULL);

  MemoryStats stats_before = unified_memory_get_stats();

  void *ptr = ALLOC(1024);

  MemoryStats stats_after = unified_memory_get_stats();
  TEST_ASSERT(stats_after.total_allocations > stats_before.total_allocations,
              "Statistics not tracking allocations");

  FREE(ptr);

  MemoryStats stats_final = unified_memory_get_stats();
  TEST_ASSERT(stats_final.total_deallocations >
                  stats_before.total_deallocations,
              "Statistics not tracking deallocations");

  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\\n=== Running Memory System Tests ===\\n\\n");

  // Basic allocation tests
  RUN_TEST(test_basic_allocation);
  RUN_TEST(test_zero_allocation);
  RUN_TEST(test_type_safe_allocation);
  RUN_TEST(test_array_allocation);

  // Pool tests
  RUN_TEST(test_pool_creation);
  RUN_TEST(test_pool_allocation);
  RUN_TEST(test_common_pools);

  // Stack tests
  RUN_TEST(test_stack_creation);
  RUN_TEST(test_stack_allocation);
  RUN_TEST(test_frame_allocator);

  // Arena tests
  RUN_TEST(test_arena_creation);
  RUN_TEST(test_arena_allocation);

  // Leak detection
  RUN_TEST(test_leak_detection);

  // Statistics
  RUN_TEST(test_statistics);

  printf("\\n=== All Memory Tests Passed! ===\\n");
  return 0;
}
