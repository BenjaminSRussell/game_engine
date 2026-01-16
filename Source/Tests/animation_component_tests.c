#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/animation_components.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>

void test_animation_components(void) {
  unified_memory_init(NULL);
  World *world = ecs_world_create(NULL);

  register_animation_components(world);

  // Create Entity
  Entity e = ecs_create_entity(world);

  // Add Animator
  AnimatorComponent anim = animator_create(1);
  animator_play(&anim, 100, true);
  ecs_add_component(world, e, g_animator_component_id, &anim);

  // Add Skeleton
  SkeletonComponent skel = skeleton_create(32);
  ecs_add_component(world, e, g_skeleton_component_id, &skel);

  // Check
  AnimatorComponent *r_anim =
      (AnimatorComponent *)ecs_get_component(world, e, g_animator_component_id);
  TEST_ASSERT(r_anim != NULL, "Animator missing");
  TEST_ASSERT(r_anim->current_clip_id == 100, "Clip ID incorrect");
  TEST_ASSERT(r_anim->is_playing == true, "Playing state incorrect");

  // Test Crossfade
  animator_cross_fade(r_anim, 200, 0.5f, true);
  TEST_ASSERT(r_anim->next_clip_id == 200, "Next clip incorrect");
  TEST_ASSERT(r_anim->blend_duration == 0.5f, "Blend duration incorrect");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Animation Component Tests ===\n");
  RUN_TEST(test_animation_components);
  printf("Tests Passed\n");
  return 0;
}
