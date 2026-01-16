#include "ecs/systems/render_systems.h"
#include "ecs/components/rendering_components.h"
#include "engine/include/core/logger.h"
#include "rendering/render_allocator.h"
#include <stdio.h>

// Placeholder frustum (in real implementation, get from camera)
typedef struct {
  float planes[6][4]; // 6 planes, each with 4 coefficients
} Frustum;

static Frustum g_main_frustum = {0};

// Simple frustum culling (placeholder)
static bool is_in_frustum(Vec3 position, float radius) {
  // TODO: Implement actual frustum culling
  // For now, always return true
  return true;
}

void culling_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Query: MeshRenderer + Transform
  ComponentType types[] = {g_mesh_renderer_component_id,
                           0}; // Transform ID would be here
  QueryDesc desc = {
      .all_components = types,
      .all_count = 1 // Just MeshRenderer for now
  };

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];
  u32 visible_count = 0;
  u32 culled_count = 0;

  while (ecs_query_next(query, &entity, comps)) {
    MeshRendererComponent *mesh = (MeshRendererComponent *)comps[0];

    // Frustum culling
    // In real implementation, get position from Transform component
    Vec3 position = {0, 0, 0};
    float radius = 1.0f;

    if (is_in_frustum(position, radius)) {
      mesh->visible = true;
      visible_count++;
    } else {
      mesh->visible = false;
      culled_count++;
    }
  }

  ecs_query_destroy(ctx->world, query);
}

void render_submission_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Query: MeshRenderer (visible only)
  ComponentType types[] = {g_mesh_renderer_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];
  u32 draw_call_count = 0;

  while (ecs_query_next(query, &entity, comps)) {
    MeshRendererComponent *mesh = (MeshRendererComponent *)comps[0];

    if (!mesh->visible)
      continue;

    // Allocate draw call from pool
    void *draw_call = render_alloc_draw_call();
    if (draw_call) {
      // TODO: Fill draw call data
      // For now, just count
      draw_call_count++;

      // In real implementation, submit to render queue
      // render_queue_submit(draw_call);
    }
  }

  ecs_query_destroy(ctx->world, query);
}

void camera_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Query: Camera + Transform
  ComponentType types[] = {g_camera_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    CameraComponent *camera = (CameraComponent *)comps[0];

    if (!camera->enabled)
      continue;

    // TODO: Update camera matrices
    // - View matrix from Transform
    // - Projection matrix from camera parameters
    // - Update frustum for culling
  }

  ecs_query_destroy(ctx->world, query);
}

void light_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Query: Light + Transform
  ComponentType types[] = {g_light_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];
  u32 light_count = 0;

  while (ecs_query_next(query, &entity, comps)) {
    LightComponent *light = (LightComponent *)comps[0];

    if (!light->enabled)
      continue;

    // TODO: Update light data
    // - Position/direction from Transform
    // - Submit to renderer

    light_count++;
  }

  ecs_query_destroy(ctx->world, query);
}

void register_rendering_systems(World *world) {
  if (!world) {
    LOG_ERROR("[Render] Cannot register systems: null world");
    return;
  }

  // Camera system (priority 10 - runs first)
  QueryDesc camera_query = {0};
  System *camera_sys =
      ecs_system_create(world, "CameraSystem", camera_system, &camera_query);
  ecs_system_set_priority(world, camera_sys, 10);

  // Culling system (priority 20 - after camera, parallel)
  QueryDesc culling_query = {0};
  System *culling_sys =
      ecs_system_create(world, "CullingSystem", culling_system, &culling_query);
  ecs_system_set_priority(world, culling_sys, 20);
  ecs_system_set_parallel(world, culling_sys,
                          true); // Enable parallel execution

  // Light system (priority 30)
  QueryDesc light_query = {0};
  System *light_sys =
      ecs_system_create(world, "LightSystem", light_system, &light_query);
  ecs_system_set_priority(world, light_sys, 30);

  // Render submission (priority 40 - runs last)
  QueryDesc submission_query = {0};
  System *submission_sys = ecs_system_create(
      world, "RenderSubmission", render_submission_system, &submission_query);
  ecs_system_set_priority(world, submission_sys, 40);

  // Rebuild execution order
  ecs_world_rebuild_execution_order(world);

  LOG_INFO("[Render] Registered rendering systems:");
  LOG_INFO("  CameraSystem (priority 10)");
  LOG_INFO("  CullingSystem (priority 20, parallel)");
  LOG_INFO("  LightSystem (priority 30)");
  LOG_INFO("  RenderSubmission (priority 40)");
}
