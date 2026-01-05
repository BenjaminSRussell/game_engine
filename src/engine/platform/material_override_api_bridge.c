// Material Override API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/material_override_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static MaterialOverrideMode g_mode = MATERIAL_OVERRIDE_NONE;
static float g_wireframe_width = 1.0f;

void material_override_set_mode(MaterialOverrideMode mode) {
  g_mode = mode;
  LOG_INFO("Material Override set to mode: %d", mode);
}

MaterialOverrideMode material_override_get_mode(void) { return g_mode; }

void material_override_set_wireframe_width(float width) {
  g_wireframe_width = width;
}

void material_override_set_wireframe_color(float r, float g, float b, float a) {
  // Set color in renderer
}
