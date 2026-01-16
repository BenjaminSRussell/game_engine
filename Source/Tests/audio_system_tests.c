#include "audio/audio_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/audio_components.h"
#include "ecs/ecs.h"
#include "ecs/systems/audio_systems.h"
#include "test_framework.h"
#include <stdio.h>

void test_audio_systems(void) {
  unified_memory_init(NULL);
  audio_allocator_init();
  World *world = ecs_world_create(NULL);

  register_audio_components(world);
  register_audio_systems(world);

  // Create 3D source
  Entity e = ecs_create_entity(world);
  AudioSourceComponent src = audio_source_create_3d(1, 1.0f, 10.0f, 100.0f);
  src.is_playing = true;
  ecs_add_component(world, e, g_audio_source_component_id, &src);

  // Update world (runs systems)
  ecs_world_update(world, 0.016f);

  // Verify system ran (by checking if component is still there/valid)
  AudioSourceComponent *r_src = (AudioSourceComponent *)ecs_get_component(
      world, e, g_audio_source_component_id);
  TEST_ASSERT(r_src != NULL, "Component lost");

  ecs_world_destroy(world);
  audio_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Audio System Tests ===\n");
  RUN_TEST(test_audio_systems);
  printf("Tests Passed\n");
  return 0;
}
