#include "ecs/components/rendering_components.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

// Global component IDs
u32 g_mesh_renderer_component_id = 0;
u32 g_camera_component_id = 0;
u32 g_light_component_id = 0;
u32 g_skybox_component_id = 0;

void register_rendering_components(void *world) {
  if (!world) {
    LOG_ERROR("[Render] Cannot register components: null world");
    return;
  }

  // Register MeshRenderer component
  ComponentInfo mesh_renderer_info = {.name = "MeshRenderer",
                                      .size = sizeof(MeshRendererComponent),
                                      .alignment =
                                          alignof(MeshRendererComponent)};
  g_mesh_renderer_component_id =
      ecs_register_component(world, &mesh_renderer_info);

  // Register Camera component
  ComponentInfo camera_info = {.name = "Camera",
                               .size = sizeof(CameraComponent),
                               .alignment = alignof(CameraComponent)};
  g_camera_component_id = ecs_register_component(world, &camera_info);

  // Register Light component
  ComponentInfo light_info = {.name = "Light",
                              .size = sizeof(LightComponent),
                              .alignment = alignof(LightComponent)};
  g_light_component_id = ecs_register_component(world, &light_info);

  // Register Skybox component
  ComponentInfo skybox_info = {.name = "Skybox",
                               .size = sizeof(SkyboxComponent),
                               .alignment = alignof(SkyboxComponent)};
  g_skybox_component_id = ecs_register_component(world, &skybox_info);

  LOG_INFO("[Render] Registered rendering components:");
  LOG_INFO("  MeshRenderer: ID %u", g_mesh_renderer_component_id);
  LOG_INFO("  Camera: ID %u", g_camera_component_id);
  LOG_INFO("  Light: ID %u", g_light_component_id);
  LOG_INFO("  Skybox: ID %u", g_skybox_component_id);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

MeshRendererComponent mesh_renderer_create(u32 mesh_id, u32 material_id) {
  MeshRendererComponent renderer = {0};
  renderer.mesh_id = mesh_id;
  renderer.material_id = material_id;
  renderer.visible = true;
  renderer.cast_shadow = true;
  renderer.receive_shadow = true;
  renderer.render_layer = 0;
  renderer.lod_bias = 0.0f;
  return renderer;
}

CameraComponent camera_create_perspective(f32 fov, f32 aspect, f32 near,
                                          f32 far) {
  CameraComponent camera = {0};
  camera.projection = CAMERA_PROJECTION_PERSPECTIVE;
  camera.fov = fov;
  camera.aspect_ratio = aspect;
  camera.near_plane = near;
  camera.far_plane = far;
  camera.render_target_id = 0; // Screen
  camera.viewport_x = 0.0f;
  camera.viewport_y = 0.0f;
  camera.viewport_width = 1.0f;
  camera.viewport_height = 1.0f;
  camera.priority = 0;
  camera.enabled = true;
  camera.clear_color = true;
  camera.clear_depth = true;
  camera.clear_color_value = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  return camera;
}

CameraComponent camera_create_orthographic(f32 size, f32 aspect, f32 near,
                                           f32 far) {
  CameraComponent camera = {0};
  camera.projection = CAMERA_PROJECTION_ORTHOGRAPHIC;
  camera.ortho_size = size;
  camera.aspect_ratio = aspect;
  camera.near_plane = near;
  camera.far_plane = far;
  camera.render_target_id = 0;
  camera.viewport_x = 0.0f;
  camera.viewport_y = 0.0f;
  camera.viewport_width = 1.0f;
  camera.viewport_height = 1.0f;
  camera.priority = 0;
  camera.enabled = true;
  camera.clear_color = true;
  camera.clear_depth = true;
  camera.clear_color_value = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  return camera;
}

LightComponent light_create_directional(Vec3 color, f32 intensity) {
  LightComponent light = {0};
  light.type = LIGHT_TYPE_DIRECTIONAL;
  light.color = color;
  light.intensity = intensity;
  light.cast_shadow = true;
  light.shadow_map_size = 2048;
  light.shadow_bias = 0.005f;
  light.enabled = true;
  return light;
}

LightComponent light_create_point(Vec3 color, f32 intensity, f32 range) {
  LightComponent light = {0};
  light.type = LIGHT_TYPE_POINT;
  light.color = color;
  light.intensity = intensity;
  light.range = range;
  light.attenuation = 1.0f;
  light.cast_shadow = false; // Expensive for point lights
  light.shadow_map_size = 1024;
  light.shadow_bias = 0.005f;
  light.enabled = true;
  return light;
}

LightComponent light_create_spot(Vec3 color, f32 intensity, f32 range,
                                 f32 inner_angle, f32 outer_angle) {
  LightComponent light = {0};
  light.type = LIGHT_TYPE_SPOT;
  light.color = color;
  light.intensity = intensity;
  light.range = range;
  light.attenuation = 1.0f;
  light.inner_cone_angle = inner_angle;
  light.outer_cone_angle = outer_angle;
  light.cast_shadow = true;
  light.shadow_map_size = 1024;
  light.shadow_bias = 0.005f;
  light.enabled = true;
  return light;
}
