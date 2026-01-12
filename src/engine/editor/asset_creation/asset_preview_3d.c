/**
 * =================================================================================================
 *                          ASSET PREVIEW RENDERER
 * =================================================================================================
 */

#include "editor/asset_creation/asset_preview_3d.h"
#include <include/math/math.h>
#include <stdio.h>
#include <string.h>

static AssetPreviewContext g_preview_ctx;

void asset_preview_init(void) {
  memset(&g_preview_ctx, 0, sizeof(AssetPreviewContext));
  g_preview_ctx.camera_distance = 5.0f;
  g_preview_ctx.camera_pitch = -0.5f; // Look down slightly
  g_preview_ctx.camera_yaw = 0.0f;
  g_preview_ctx.auto_rotate_speed = 0.5f; // Radians per second
  g_preview_ctx.transparent_background = false;
  g_preview_ctx.light_intensity = 1.0f;
  g_preview_ctx.light_dir[0] = -1.0f;
  g_preview_ctx.light_dir[1] = -1.0f;
  g_preview_ctx.light_dir[2] = -1.0f;
  // TODO(Jules): Add MSAA support to asset_preview_init for better quality.
}

AssetPreviewContext *asset_preview_get_context(void) { return &g_preview_ctx; }

void asset_preview_center_camera(const float *bbox_min, const float *bbox_max) {
  // Calculate center
  float center[3];
  center[0] = (bbox_min[0] + bbox_max[0]) * 0.5f;
  center[1] = (bbox_min[1] + bbox_max[1]) * 0.5f;
  center[2] = (bbox_min[2] + bbox_max[2]) * 0.5f;

  // Calculate radius (half diagonal)
  float dx = bbox_max[0] - bbox_min[0];
  float dy = bbox_max[1] - bbox_min[1];
  float dz = bbox_max[2] - bbox_min[2];
  float radius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;

  // Position camera to fit sphere
  // tan(fov/2) = radius / distance
  // distance = radius / tan(fov/2)
  const float fov = 1.047f;      // 60 degrees in radians
  const float fit_factor = 1.2f; // Margin
  // TODO(Jules): Fix potential divide by zero in asset_preview_center_camera if
  // radius is 0.
  g_preview_ctx.camera_distance = (radius / tanf(fov * 0.5f)) * fit_factor;

  // Ensure min distance
  if (g_preview_ctx.camera_distance < 0.1f)
    g_preview_ctx.camera_distance = 2.0f;
}

void asset_preview_update(float delta_time) {
  if (g_preview_ctx.auto_rotate_speed != 0.0f) {
    g_preview_ctx.camera_yaw += g_preview_ctx.auto_rotate_speed * delta_time;
    // Keep in 0-2PI range
    if (g_preview_ctx.camera_yaw > 6.28318f)
      g_preview_ctx.camera_yaw -= 6.28318f;
  }
  // TODO(Jules): Implement orbit camera damping for smoother rotation.
}

void asset_preview_render_scene(void *asset_ptr) {
  // Determine View Matrix
  // Orbit camera logic
  float cx = sinf(g_preview_ctx.camera_yaw) * cosf(g_preview_ctx.camera_pitch) *
             g_preview_ctx.camera_distance;
  float cy = sinf(g_preview_ctx.camera_pitch) *
             g_preview_ctx.camera_distance; // This is a bit simplified for Y-up
  float cz = cosf(g_preview_ctx.camera_yaw) * cosf(g_preview_ctx.camera_pitch) *
             g_preview_ctx.camera_distance;

  // Camera Pos: Center + offset (simplified, assume object at origin)
  // float cam_pos[3] = { cx, cy - g_preview_ctx.camera_distance, cz }; //
  // Adjusted

  // TODO(Jules): Add support for background environment maps.
  // Clear buffer (Transparent or Solid)
  if (g_preview_ctx.transparent_background) {
    // clear_color(0, 0, 0, 0);
  } else {
    // clear_color(0.2f, 0.2f, 0.2f, 1.0f);
  }

  // Render Asset (placeholder)
  // renderer_draw_mesh(asset_ptr, ...);
  // TODO(Jules): Implement proper lighting model in asset_preview_render_scene.
}

void asset_preview_resize(int width, int height) {
  g_preview_ctx.width = width;
  g_preview_ctx.height = height;
}
