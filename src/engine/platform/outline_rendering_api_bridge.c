// Outline Rendering API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/outline_rendering_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Mock implementation state
static bool g_enabled = true;
static float g_width = 2.0f;
static bool g_selection_only = true;
static float g_color[4] = {1.0f, 0.5f, 0.0f, 1.0f}; // Orange default

void outline_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Outline: %s", enabled ? "Enabled" : "Disabled");
}

bool outline_is_enabled(void) { return g_enabled; }

void outline_set_color(float r, float g, float b, float a) {
  g_color[0] = r;
  g_color[1] = g;
  g_color[2] = b;
  g_color[3] = a;
}

void outline_set_width(float width) { g_width = width; }

float outline_get_width(void) { return g_width; }

void outline_set_selection_only(bool selection_only) {
  g_selection_only = selection_only;
}

void outline_add_entity(uint64_t entity_id) {
  // Add to internal list of highlighted entities
}

void outline_remove_entity(uint64_t entity_id) {
  // Remove from internal list
}

void outline_clear_entities(void) {
  // Clear list
}
