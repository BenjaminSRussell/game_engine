#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/physics_components.h"
#include "ecs/ecs.h"
#include "ecs/systems/physics_systems.h"
#include "physics/physics_allocator.h"
#include "test_framework.h"
#include <stdio.h>

void test_physics_simulation(void) {
  unified_memory_init(NULL);
  physics_allocator_init();
  World *world = ecs_world_create(NULL);

  register_physics_components(world);
  register_physics_systems(world);

  // Create falling object
  Entity entity = ecs_create_entity(world);
  RigidbodyComponent rb = rigidbody_create(1.0f);
  rb.velocity = (Vec3){0, 0, 0};
  ecs_add_component(world, entity, g_rigidbody_component_id, &rb);

  // Simulate 1 second (approx)
  for (int i = 0; i < 60; i++) {
    ecs_world_update(world, 1.0f / 60.0f);
  }

  // Verify gravity applied
  RigidbodyComponent *updated_rb = (RigidbodyComponent *)ecs_get_component(
      world, entity, g_rigidbody_component_id);

  // Expected velocity after 1s ~ -9.81 m/s (gravity)
  // Allow some error margin due to drag/integration
  TEST_ASSERT(updated_rb->velocity.y < -9.0f, "Gravity not applied correctly");

  ecs_world_destroy(world);
  physics_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Physics Logic Tests ===\n");
  RUN_TEST(test_physics_simulation);
  printf("Tests Passed\n");
  return 0;
}
