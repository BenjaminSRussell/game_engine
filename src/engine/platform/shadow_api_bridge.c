// Shadow Cascades API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/shadow_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;
static uint32_t g_cascade_count = 4;
static uint32_t g_resolution = 2048;
static float g_split_lambda = 0.5f;
static bool g_soft_shadows = true;

void shadow_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Shadows: %s", enabled ? "Enabled" : "Disabled");
}

bool shadow_is_enabled(void) { return g_enabled; }

void shadow_set_cascade_count(uint32_t count) {
  g_cascade_count = count;
  if (g_cascade_count < 1)
    g_cascade_count = 1;
  if (g_cascade_count > 4)
    g_cascade_count = 4;
  LOG_INFO("Shadow cascade count set to: %u", g_cascade_count);
}

uint32_t shadow_get_cascade_count(void) { return g_cascade_count; }

void shadow_set_resolution(uint32_t resolution) {
  g_resolution = resolution;
  LOG_INFO("Shadow resolution set to: %u", g_resolution);
}

uint32_t shadow_get_resolution(void) { return g_resolution; }

void shadow_set_split_lambda(float lambda) { g_split_lambda = lambda; }

float shadow_get_split_lambda(void) { return g_split_lambda; }

void shadow_set_soft_shadows(bool enabled) {
  g_soft_shadows = enabled;
  LOG_INFO("Soft shadows: %s", enabled ? "Enabled" : "Disabled");
}
