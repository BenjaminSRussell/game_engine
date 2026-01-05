// Grid Rendering API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/grid_rendering_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;
static float g_size = 100.0f;
static float g_spacing = 1.0f;

void grid_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Grid: %s", enabled ? "Enabled" : "Disabled");
}

bool grid_is_enabled(void) { return g_enabled; }

void grid_set_size(float size) { g_size = size; }

float grid_get_size(void) { return g_size; }

void grid_set_spacing(float spacing) { g_spacing = spacing; }

float grid_get_spacing(void) { return g_spacing; }

void grid_set_primary_color(float r, float g, float b, float a) {
  // Set color in renderer
}

void grid_set_secondary_color(float r, float g, float b, float a) {
  // Set color in renderer
}

void grid_render(void *view_matrix, void *projection_matrix) {
  // Pass to renderer
}
