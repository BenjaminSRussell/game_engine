// LOD Control API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/lod_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static float g_bias = 0.0f;
static float g_distance_multiplier = 1.0f;
static bool g_enabled = true;
static int32_t g_force_level = -1; // -1 = auto

void lod_set_bias(float bias) { g_bias = bias; }

float lod_get_bias(void) { return g_bias; }

void lod_set_distance_multiplier(float multiplier) {
  g_distance_multiplier = multiplier;
}

float lod_get_distance_multiplier(void) { return g_distance_multiplier; }

void lod_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("LOD System: %s", enabled ? "Enabled" : "Disabled");
}

bool lod_is_enabled(void) { return g_enabled; }

void lod_set_force_level(int32_t level) {
  g_force_level = level;
  if (level >= 0) {
    LOG_INFO("Forcing LOD level: %d", level);
  } else {
    LOG_INFO("LOD level: Auto");
  }
}

int32_t lod_get_force_level(void) { return g_force_level; }
