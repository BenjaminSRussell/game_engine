// Terrain Clipmap API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/terrain_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static uint32_t g_lod_count = 5;
static float g_center[3] = {0.0f, 0.0f, 0.0f};
static float g_tile_size = 64.0f;
static float g_height_scale = 1.0f;
static bool g_enabled = true;

void terrain_set_lod_count(uint32_t count) {
  g_lod_count = count;
  LOG_INFO("Terrain LOD count set to: %u", count);
}

uint32_t terrain_get_lod_count(void) { return g_lod_count; }

void terrain_set_center(float x, float y, float z) {
  g_center[0] = x;
  g_center[1] = y;
  g_center[2] = z;
}

void terrain_set_tile_size(float size) { g_tile_size = size; }

float terrain_get_tile_size(void) { return g_tile_size; }

void terrain_set_height_scale(float scale) { g_height_scale = scale; }

float terrain_get_height_scale(void) { return g_height_scale; }

void terrain_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Terrain: %s", enabled ? "Enabled" : "Disabled");
}

bool terrain_is_enabled(void) { return g_enabled; }
