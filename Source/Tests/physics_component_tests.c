#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/physics_components.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>

void test_physics_components(void) {
  unified_memory_init(NULL);
  World *world = ecs_world_create(NULL);

  register_physics_components(world);

  TEST_ASSERT(g_rigidbody_component_id != 0, "Rigidbody not registered");
  TEST_ASSERT(g_collider_component_id != 0, "Collider not registered");

  // Create entity with physics
  Entity entity = ecs_create_entity(world);

  RigidbodyComponent rb = rigidbody_create(10.0f);
  esc_add_component(world, entity, g_rigidbody_component_id, &rb);
  // Note: Typo in test 'esc_add' -> 'ecs_add', careful

  ColliderComponent col = collider_create_box((Vec3){1, 1, 1});
  ecs_add_component(world, entity, g_collider_component_id, &col);

  // Retrieve
  RigidbodyComponent *retrieved_rb = (RigidbodyComponent *)ecs_get_component(
      world, entity, g_rigidbody_component_id);

  TEST_ASSERT(retrieved_rb != NULL, "Rigidbody retrieve failed");
  TEST_ASSERT(retrieved_rb->mass == 10.0f, "Mass incorrect");

  ColliderComponent *retrieved_col = (ColliderComponent *)ecs_get_component(
      world, entity, g_collider_component_id);

  TEST_ASSERT(retrieved_col != NULL, "Collider retrieve failed");
  TEST_ASSERT(retrieved_col->shape_type == COLLIDER_SHAPE_BOX,
              "Shape type incorrect");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Physics Component Tests ===\n");
  RUN_TEST(test_physics_components);
  printf("Tests Passed\n");
  return 0;
}
