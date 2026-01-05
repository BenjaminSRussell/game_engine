// Decal System API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/decal_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;
static uint64_t g_next_decal_id = 1;

uint64_t decal_create(const char *texture_path, float pos_x, float pos_y,
                      float pos_z, float size) {
  if (!texture_path)
    return 0;

  uint64_t id = g_next_decal_id++;
  LOG_INFO("Created decal %llu at (%.2f, %.2f, %.2f) size %.2f", id, pos_x,
           pos_y, pos_z, size);
  return id;
}

void decal_destroy(uint64_t decal_id) {
  LOG_INFO("Destroyed decal %llu", decal_id);
}

void decal_set_opacity(uint64_t decal_id, float opacity) {
  // Set opacity
}

void decal_set_rotation(uint64_t decal_id, float angle) {
  // Set rotation
}

void decal_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Decal System: %s", enabled ? "Enabled" : "Disabled");
}

bool decal_is_enabled(void) { return g_enabled; }
