// Grid Renderer - Phase 1: Grid Visualization
// Implements grid rendering in the viewport for visual reference and snapping
// Roadmap: grid_snapping_roadmap.md Phase 1

#include "rendering/grid_settings.h"
#include <core/logger.h>
#include <math/vec3.h>
#include <string.h>

typedef struct GridRenderer {
  GridSettings settings;
  bool initialized;
  // Metal/Vulkan resources would go here
  // For now this is a stub for the C side
} GridRenderer;

static GridRenderer g_grid_renderer = {0};

void grid_renderer_init(void) {
  g_grid_renderer.settings = grid_settings_default();
  g_grid_renderer.initialized = true;
  LOG_INFO("Grid renderer initialized");
}

void grid_renderer_shutdown(void) {
  g_grid_renderer.initialized = false;
  LOG_INFO("Grid renderer shut down");
}

void grid_renderer_set_settings(const GridSettings *settings) {
  if (!g_grid_renderer.initialized || !settings)
    return;
  memcpy(&g_grid_renderer.settings, settings, sizeof(GridSettings));
}

void grid_renderer_get_settings(GridSettings *out_settings) {
  if (!g_grid_renderer.initialized || !out_settings)
    return;
  memcpy(out_settings, &g_grid_renderer.settings, sizeof(GridSettings));
}

void grid_renderer_toggle_grid(void) {
  g_grid_renderer.settings.grid_enabled =
      !g_grid_renderer.settings.grid_enabled;
  LOG_INFO("Grid %s",
           g_grid_renderer.settings.grid_enabled ? "enabled" : "disabled");
}

void grid_renderer_set_grid_spacing(float spacing) {
  if (spacing > 0.0f) {
    g_grid_renderer.settings.grid_spacing = spacing;
    LOG_INFO("Grid spacing set to %.2f", spacing);
  }
}

void grid_renderer_toggle_position_snap(void) {
  g_grid_renderer.settings.snap_position_enabled =
      !g_grid_renderer.settings.snap_position_enabled;
  LOG_INFO("Position snapping %s",
           g_grid_renderer.settings.snap_position_enabled ? "enabled"
                                                          : "disabled");
}

void grid_renderer_toggle_rotation_snap(void) {
  g_grid_renderer.settings.snap_rotation_enabled =
      !g_grid_renderer.settings.snap_rotation_enabled;
  LOG_INFO("Rotation snapping %s",
           g_grid_renderer.settings.snap_rotation_enabled ? "enabled"
                                                          : "disabled");
}

void grid_renderer_toggle_scale_snap(void) {
  g_grid_renderer.settings.snap_scale_enabled =
      !g_grid_renderer.settings.snap_scale_enabled;
  LOG_INFO("Scale snapping %s", g_grid_renderer.settings.snap_scale_enabled
                                    ? "enabled"
                                    : "disabled");
}

void grid_renderer_render(const float camera_position[3]) {
  if (!g_grid_renderer.initialized || !g_grid_renderer.settings.grid_enabled) {
    return;
  }

  // TODO: Actual grid rendering using Metal/Vulkan
  // This would involve:
  // 1. Generate grid lines in world space relative to camera
  // 2. Apply fade-out based on distance
  // 3. Render major/minor lines with different thickness
  // 4. Apply grid color and opacity

  (void)camera_position; // Suppress unused warning
}

// Snapping utility accessible to transform systems
void grid_renderer_apply_position_snap(float position[3]) {
  if (g_grid_renderer.settings.snap_position_enabled) {
    snap_position(position, g_grid_renderer.settings.snap_position_increment);
  }
}

void grid_renderer_apply_rotation_snap(float rotation[3]) {
  if (g_grid_renderer.settings.snap_rotation_enabled) {
    snap_rotation(rotation, g_grid_renderer.settings.snap_rotation_increment);
  }
}

void grid_renderer_apply_scale_snap(float scale[3]) {
  if (g_grid_renderer.settings.snap_scale_enabled) {
    snap_scale(scale, g_grid_renderer.settings.snap_scale_increment);
  }
}
