#include "core/memory/unified_memory_allocator.h"
#include "physics/physics_allocator.h"
#include "test_framework.h"
#include <stdio.h>

// ============================================================================
// PHYSICS ALLOCATOR TESTS
// ============================================================================

void test_physics_allocator_init(void) {
  unified_memory_init(NULL);

  physics_allocator_init();

  PhysicsAllocator *allocator = physics_allocator_get();
  TEST_ASSERT(allocator != NULL, "Physics allocator not initialized");
  TEST_ASSERT(allocator->frame_arena != NULL, "Frame arena not created");
  TEST_ASSERT(allocator->rigidbody_pool != NULL, "Rigidbody pool not created");
  TEST_ASSERT(allocator->collider_pool != NULL, "Collider pool not created");

  physics_allocator_shutdown();
  unified_memory_shutdown();
}

void test_physics_pool_allocation(void) {
  unified_memory_init(NULL);
  physics_allocator_init();

  // Rigidbody allocation
  void *rb1 = physics_alloc_rigidbody();
  void *rb2 = physics_alloc_rigidbody();
  TEST_ASSERT(rb1 != NULL && rb2 != NULL, "Rigidbody allocation failed");
  TEST_ASSERT(rb1 != rb2, "Allocator returned same pointer");

  physics_free_rigidbody(rb1);

  // Collider allocation
  void *col1 = physics_alloc_collider();
  TEST_ASSERT(col1 != NULL, "Collider allocation failed");

  PhysicsAllocator *allocator = physics_allocator_get();
  TEST_ASSERT(allocator->rigidbodies_allocated == 1,
              "Rigidbody count incorrect");
  TEST_ASSERT(allocator->colliders_allocated == 1, "Collider count incorrect");

  physics_allocator_shutdown();
  unified_memory_shutdown();
}

void test_physics_frame_allocation(void) {
  unified_memory_init(NULL);
  physics_allocator_init();

  void *temp1 = physics_alloc_frame(1024);
  TEST_ASSERT(temp1 != NULL, "Frame allocation failed");

  physics_allocator_reset_frame();

  void *temp2 = physics_alloc_frame(1024);
  TEST_ASSERT(temp2 == temp1, "Frame not reset correctly");

  physics_allocator_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running Physics Allocator Tests ===\n\n");

  RUN_TEST(test_physics_allocator_init);
  RUN_TEST(test_physics_pool_allocation);
  RUN_TEST(test_physics_frame_allocation);

  printf("\n=== All Physics Allocator Tests Passed! ===\n");
  return 0;
}
