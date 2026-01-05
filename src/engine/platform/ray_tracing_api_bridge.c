// Ray Tracing API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/ray_tracing_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = false;
static uint32_t g_max_bounces = 3;
static uint32_t g_samples = 1;
static bool g_denoising = true;

void raytracing_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Ray Tracing: %s", enabled ? "Enabled" : "Disabled");
}

bool raytracing_is_enabled(void) { return g_enabled; }

bool raytracing_is_supported(void) {
  // In real engine, check Vulkan/DX capabilities
#ifdef VULKAN_AVAILABLE
  return true; // Mock: pretend we have an RTX card
#else
  return false;
#endif
}

void raytracing_set_max_bounces(uint32_t bounces) { g_max_bounces = bounces; }

uint32_t raytracing_get_max_bounces(void) { return g_max_bounces; }

void raytracing_set_samples_per_pixel(uint32_t samples) { g_samples = samples; }

uint32_t raytracing_get_samples_per_pixel(void) { return g_samples; }

void raytracing_set_denoising_enabled(bool enabled) { g_denoising = enabled; }

bool raytracing_get_denoising_enabled(void) { return g_denoising; }
