#include "core/memory/unified_memory_allocator.h"
#include "ecs/components/rendering_components.h"
#include "ecs/ecs.h"
#include "test_framework.h"
#include <stdio.h>

// ============================================================================
// RENDERING COMPONENT TESTS
// ============================================================================

void test_component_registration(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);

  register_rendering_components(world);

  TEST_ASSERT(g_mesh_renderer_component_id != 0, "MeshRenderer not registered");
  TEST_ASSERT(g_camera_component_id != 0, "Camera not registered");
  TEST_ASSERT(g_light_component_id != 0, "Light not registered");
  TEST_ASSERT(g_skybox_component_id != 0, "Skybox not registered");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_mesh_renderer_component(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);
  register_rendering_components(world);

  // Create entity with mesh renderer
  Entity entity = ecs_create_entity(world);

  MeshRendererComponent renderer = mesh_renderer_create(123, 456);
  ecs_add_component(world, entity, g_mesh_renderer_component_id, &renderer);

  // Retrieve component
  MeshRendererComponent *retrieved = (MeshRendererComponent *)ecs_get_component(
      world, entity, g_mesh_renderer_component_id);

  TEST_ASSERT(retrieved != NULL, "Failed to retrieve MeshRenderer component");
  TEST_ASSERT(retrieved->mesh_id == 123, "Mesh ID incorrect");
  TEST_ASSERT(retrieved->material_id == 456, "Material ID incorrect");
  TEST_ASSERT(retrieved->visible == true, "Visibility flag incorrect");
  TEST_ASSERT(retrieved->cast_shadow == true, "Shadow casting flag incorrect");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_camera_component(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);
  register_rendering_components(world);

  // Create entity with camera
  Entity entity = ecs_create_entity(world);

  CameraComponent camera =
      camera_create_perspective(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
  ecs_add_component(world, entity, g_camera_component_id, &camera);

  // Retrieve component
  CameraComponent *retrieved = (CameraComponent *)ecs_get_component(
      world, entity, g_camera_component_id);

  TEST_ASSERT(retrieved != NULL, "Failed to retrieve Camera component");
  TEST_ASSERT(retrieved->projection == CAMERA_PROJECTION_PERSPECTIVE,
              "Projection type incorrect");
  TEST_ASSERT(retrieved->fov == 60.0f, "FOV incorrect");
  TEST_ASSERT(retrieved->enabled == true, "Camera not enabled");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_light_component(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);
  register_rendering_components(world);

  // Create entity with directional light
  Entity entity = ecs_create_entity(world);

  Vec3 color = {1.0f, 1.0f, 1.0f};
  LightComponent light = light_create_directional(color, 1.0f);
  ecs_add_component(world, entity, g_light_component_id, &light);

  // Retrieve component
  LightComponent *retrieved =
      (LightComponent *)ecs_get_component(world, entity, g_light_component_id);

  TEST_ASSERT(retrieved != NULL, "Failed to retrieve Light component");
  TEST_ASSERT(retrieved->type == LIGHT_TYPE_DIRECTIONAL,
              "Light type incorrect");
  TEST_ASSERT(retrieved->intensity == 1.0f, "Intensity incorrect");
  TEST_ASSERT(retrieved->enabled == true, "Light not enabled");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_multiple_components(void) {
  unified_memory_init(NULL);

  World *world = ecs_world_create(NULL);
  register_rendering_components(world);

  // Create entity with multiple rendering components
  Entity entity = ecs_create_entity(world);

  MeshRendererComponent renderer = mesh_renderer_create(1, 2);
  ecs_add_component(world, entity, g_mesh_renderer_component_id, &renderer);

  CameraComponent camera = camera_create_perspective(45.0f, 1.0f, 0.1f, 100.0f);
  ecs_add_component(world, entity, g_camera_component_id, &camera);

  // Verify both components exist
  TEST_ASSERT(ecs_has_component(world, entity, g_mesh_renderer_component_id),
              "MeshRenderer component missing");
  TEST_ASSERT(ecs_has_component(world, entity, g_camera_component_id),
              "Camera component missing");

  ecs_world_destroy(world);
  unified_memory_shutdown();
}

void test_light_types(void) {
  unified_memory_init(NULL);

  Vec3 white = {1.0f, 1.0f, 1.0f};

  // Test directional light
  LightComponent dir = light_create_directional(white, 1.0f);
  TEST_ASSERT(dir.type == LIGHT_TYPE_DIRECTIONAL,
              "Directional light type incorrect");

  // Test point light
  LightComponent point = light_create_point(white, 1.0f, 10.0f);
  TEST_ASSERT(point.type == LIGHT_TYPE_POINT, "Point light type incorrect");
  TEST_ASSERT(point.range == 10.0f, "Point light range incorrect");

  // Test spot light
  LightComponent spot = light_create_spot(white, 1.0f, 15.0f, 30.0f, 45.0f);
  TEST_ASSERT(spot.type == LIGHT_TYPE_SPOT, "Spot light type incorrect");
  TEST_ASSERT(spot.inner_cone_angle == 30.0f,
              "Spot light inner angle incorrect");
  TEST_ASSERT(spot.outer_cone_angle == 45.0f,
              "Spot light outer angle incorrect");

  unified_memory_shutdown();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

int main(void) {
  printf("\n=== Running Rendering Component Tests ===\n\n");

  RUN_TEST(test_component_registration);
  RUN_TEST(test_mesh_renderer_component);
  RUN_TEST(test_camera_component);
  RUN_TEST(test_light_component);
  RUN_TEST(test_multiple_components);
  RUN_TEST(test_light_types);

  printf("\n=== All Rendering Component Tests Passed! ===\n");
  return 0;
}
