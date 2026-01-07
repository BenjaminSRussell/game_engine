// Global Illumination API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/gi_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = false;
static uint32_t g_quality = 1; // Medium
static float g_intensity = 1.0f;
static uint32_t g_samples = 64;

void gi_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Global Illumination: %s", enabled ? "Enabled" : "Disabled");
}

bool gi_is_enabled(void) { return g_enabled; }

void gi_set_quality(uint32_t quality) {
  g_quality = quality;
  LOG_INFO("GI Quality set to: %u", quality);
}

uint32_t gi_get_quality(void) { return g_quality; }

void gi_set_intensity(float intensity) { g_intensity = intensity; }

float gi_get_intensity(void) { return g_intensity; }

void gi_set_samples(uint32_t samples) { g_samples = samples; }

uint32_t gi_get_samples(void) { return g_samples; }
