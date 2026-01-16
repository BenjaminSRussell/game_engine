#include "core/memory/unified_memory_allocator.h"
#include "rendering/render_allocator.h"
#include "test_framework.h"
#include <stdio.h>

// ============================================================================
// RENDER ALLOCATOR TESTS
// ============================================================================

void test_render_allocator_init(void) {
  unified_memory_init(NULL);

  render_allocator_init();

  RenderAllocator *allocator = render_allocator_get();
  TEST_ASSERT(allocator != NULL, "Render allocator not initialized");
  TEST_ASSERT(allocator->frame_arena != NULL, "Frame arena not created");
  TEST_ASSERT(allocator->draw_call_pool != NULL, "Draw call pool not created");
  TEST_ASSERT(allocator->mesh_pool != NULL, "Mesh pool not created");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

void test_frame_allocation(void) {
  unified_memory_init(NULL);
  render_allocator_init();

  // Allocate from frame arena
  void *ptr1 = render_alloc_frame(1024);
  void *ptr2 = render_alloc_frame(2048);

  TEST_ASSERT(ptr1 != NULL, "Frame allocation failed");
  TEST_ASSERT(ptr2 != NULL, "Frame allocation failed");

  // Reset frame
  render_allocator_reset_frame();

  // Allocate again (should reuse memory)
  void *ptr3 = render_alloc_frame(1024);
  TEST_ASSERT(ptr3 == ptr1, "Frame arena not reset properly");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

void test_draw_call_pool(void) {
  unified_memory_init(NULL);
  render_allocator_init();

  // Allocate draw calls
  void *dc1 = render_alloc_draw_call();
  void *dc2 = render_alloc_draw_call();
  void *dc3 = render_alloc_draw_call();

  TEST_ASSERT(dc1 != NULL, "Draw call allocation failed");
  TEST_ASSERT(dc2 != NULL, "Draw call allocation failed");
  TEST_ASSERT(dc3 != NULL, "Draw call allocation failed");

  RenderAllocator *allocator = render_allocator_get();
  TEST_ASSERT(allocator->draw_calls_allocated == 3,
              "Draw call count incorrect");

  // Free one
  render_free_draw_call(dc2);
  TEST_ASSERT(allocator->draw_calls_allocated == 2,
              "Draw call count after free incorrect");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

void test_mesh_pool(void) {
  unified_memory_init(NULL);
  render_allocator_init();

  // Allocate meshes
  void *mesh1 = render_alloc_mesh();
  void *mesh2 = render_alloc_mesh();

  TEST_ASSERT(mesh1 != NULL, "Mesh allocation failed");
  TEST_ASSERT(mesh2 != NULL, "Mesh allocation failed");

  RenderAllocator *allocator = render_allocator_get();
  TEST_ASSERT(allocator->meshes_allocated == 2, "Mesh count incorrect");

  // Free
  render_free_mesh(mesh1);
  render_free_mesh(mesh2);
  TEST_ASSERT(allocator->meshes_allocated == 0,
              "Mesh count after free incorrect");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

void test_frame_arena_peak_tracking(void) {
  unified_memory_init(NULL);
  render_allocator_init();

  // Allocate varying amounts
  render_alloc_frame(1024);
  render_allocator_reset_frame();

  render_alloc_frame(2048);
  render_allocator_reset_frame();

  render_alloc_frame(4096);
  render_allocator_reset_frame();

  RenderAllocator *allocator = render_allocator_get();
  TEST_ASSERT(allocator->frame_arena_peak_usage >= 4096,
              "Peak usage not tracked correctly");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

void test_large_frame_allocation(void) {
  unified_memory_init(NULL);
  render_allocator_init();

  // Allocate 1MB from frame arena
  void *large = render_alloc_frame(1024 * 1024);
  TEST_ASSERT(large != NULL, "Large frame allocation failed");

  // Reset and allocate again
  render_allocator_reset_frame();
  void *large2 = render_alloc_frame(1024 * 1024);
  TEST_ASSERT(large2 != NULL, "Large frame allocation after reset failed");

  render_allocator_shutdown();
  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running Render Allocator Tests ===\n\n");

  RUN_TEST(test_render_allocator_init);
  RUN_TEST(test_frame_allocation);
  RUN_TEST(test_draw_call_pool);
  RUN_TEST(test_mesh_pool);
  RUN_TEST(test_frame_arena_peak_tracking);
  RUN_TEST(test_large_frame_allocation);

  printf("\n=== All Render Allocator Tests Passed! ===\n");
  return 0;
}
