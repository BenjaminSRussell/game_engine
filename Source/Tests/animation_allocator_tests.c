#include "animation/animation_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "test_framework.h"
#include <stdio.h>

void test_anim_allocator(void) {
  unified_memory_init(NULL);
  animation_allocator_init();

  AnimationAllocator *allocator = g_animation_allocator;
  TEST_ASSERT(allocator != NULL, "Allocator init failed");

  // Test Skeleton
  void *skel = anim_alloc_skeleton();
  TEST_ASSERT(skel != NULL, "Skeleton alloc failed");
  TEST_ASSERT(allocator->skeletons_allocated == 1, "Skeleton count error");
  anim_free_skeleton(skel);

  // Test Pose
  void *pose = anim_alloc_pose();
  TEST_ASSERT(pose != NULL, "Pose alloc failed");
  anim_free_pose(pose);

  // Test Blend Buffer (Arena)
  void *buf = anim_alloc_blend_buffer(1024);
  TEST_ASSERT(buf != NULL, "Blend buffer alloc failed");
  animation_allocator_reset_blend_arena();

  animation_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Animation Allocator Tests ===\n");
  RUN_TEST(test_anim_allocator);
  printf("Tests Passed\n");
  return 0;
}
