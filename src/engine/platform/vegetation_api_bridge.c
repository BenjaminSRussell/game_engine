// Vegetation System API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/vegetation_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;
static float g_density = 0.7f;
static float g_wind_strength = 0.3f;
static float g_lod_bias = 0.0f;

void vegetation_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Vegetation: %s", enabled ? "Enabled" : "Disabled");
}

bool vegetation_is_enabled(void) { return g_enabled; }

void vegetation_set_density(float density) { g_density = density; }

float vegetation_get_density(void) { return g_density; }

void vegetation_set_wind_strength(float strength) {
  g_wind_strength = strength;
}

float vegetation_get_wind_strength(void) { return g_wind_strength; }

void vegetation_set_lod_bias(float bias) { g_lod_bias = bias; }

float vegetation_get_lod_bias(void) { return g_lod_bias; }
