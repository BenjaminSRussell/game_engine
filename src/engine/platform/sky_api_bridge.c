// Sky & Atmosphere API Bridge Implementation

#include "../include/core/logger.h"
#include <math.h>
#include "../include/platform/sky_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static float g_sun_dir[3] = {0.0f, 1.0f, 0.0f};
static float g_time_of_day = 12.0f; // Noon
static float g_scattering_intensity = 1.0f;
static float g_cloud_density = 0.5f;
static bool g_clouds_enabled = true;

void sky_set_sun_direction(float x, float y, float z) {
  g_sun_dir[0] = x;
  g_sun_dir[1] = y;
  g_sun_dir[2] = z;
}

void sky_set_time_of_day(float hours) {
  g_time_of_day = fmodf(hours, 24.0f);
  if (g_time_of_day < 0.0f)
    g_time_of_day += 24.0f;
  LOG_INFO("Time of day set to: %.2f", g_time_of_day);
}

float sky_get_time_of_day(void) { return g_time_of_day; }

void sky_set_scattering_intensity(float intensity) {
  g_scattering_intensity = intensity;
}

float sky_get_scattering_intensity(void) { return g_scattering_intensity; }

void sky_set_cloud_density(float density) { g_cloud_density = density; }

float sky_get_cloud_density(void) { return g_cloud_density; }

void sky_set_clouds_enabled(bool enabled) {
  g_clouds_enabled = enabled;
  LOG_INFO("Clouds: %s", enabled ? "Enabled" : "Disabled");
}

bool sky_get_clouds_enabled(void) { return g_clouds_enabled; }
