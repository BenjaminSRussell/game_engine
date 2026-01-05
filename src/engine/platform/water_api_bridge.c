// Water System API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/water_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;
static uint32_t g_quality = 1; // Medium
static float g_wave_amplitude = 0.5f;
static float g_wave_frequency = 1.0f;
static bool g_reflections = true;
static bool g_refractions = true;

void water_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Water: %s", enabled ? "Enabled" : "Disabled");
}

bool water_is_enabled(void) { return g_enabled; }

void water_set_quality(uint32_t quality) {
  g_quality = quality;
  LOG_INFO("Water quality set to: %u", quality);
}

uint32_t water_get_quality(void) { return g_quality; }

void water_set_wave_amplitude(float amplitude) { g_wave_amplitude = amplitude; }

float water_get_wave_amplitude(void) { return g_wave_amplitude; }

void water_set_wave_frequency(float frequency) { g_wave_frequency = frequency; }

float water_get_wave_frequency(void) { return g_wave_frequency; }

void water_set_reflections_enabled(bool enabled) { g_reflections = enabled; }

bool water_get_reflections_enabled(void) { return g_reflections; }

void water_set_refractions_enabled(bool enabled) { g_refractions = enabled; }

bool water_get_refractions_enabled(void) { return g_refractions; }
