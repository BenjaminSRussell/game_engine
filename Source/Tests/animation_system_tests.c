#include "animation/animation_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/animation_components.h"
#include "ecs/ecs.h"
#include "ecs/systems/animation_systems.h"
#include "test_framework.h"
#include <stdio.h>

void test_animation_systems(void) {
  unified_memory_init(NULL);
  animation_allocator_init();
  World *world = ecs_world_create(NULL);

  register_animation_components(world);
  register_animation_systems(world);

  // Create Entity
  Entity e = ecs_create_entity(world);
  AnimatorComponent anim = animator_create(1);
  animator_play(&anim, 100, true);
  ecs_add_component(world, e, g_animator_component_id, &anim);

  SkeletonComponent skel = skeleton_create(10);
  ecs_add_component(world, e, g_skeleton_component_id, &skel);

  // Test 1: Update advances time
  ecs_world_update(world, 0.1f);

  AnimatorComponent *r_anim =
      (AnimatorComponent *)ecs_get_component(world, e, g_animator_component_id);
  TEST_ASSERT(r_anim->time > 0.09f, "Time not advanced");

  // Test 2: Crossfade logic
  animator_cross_fade(r_anim, 200, 1.0f, true);

  // Advance half way
  ecs_world_update(world, 0.5f);
  TEST_ASSERT(r_anim->blend_weight > 0.4f, "Blending not active");

  // Advance to finish
  ecs_world_update(world, 0.6f); // 0.5 + 0.6 > 1.0
  TEST_ASSERT(r_anim->current_clip_id == 200, "Blend finish clip ID mismatch");
  TEST_ASSERT(r_anim->blend_weight == 0.0f, "Blend weight didn't reset");

  ecs_world_destroy(world);
  animation_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Animation System Tests ===\n");
  RUN_TEST(test_animation_systems);
  printf("Tests Passed\n");
  return 0;
}
