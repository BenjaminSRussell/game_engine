#include "world/biome/biome_system.h"
#include "core/logger.h"
#include <math.h>

static struct {
  float temperature; // -1.0 to 1.0
  float humidity;    // 0.0 to 1.0
  float vegetation_density;
  float blend_distance;
} biome_state;

void biome_sys_init(void) {
  biome_state.temperature = 0.0f;
  biome_state.humidity = 0.5f;
  biome_state.vegetation_density = 0.5f;
  biome_state.blend_distance = 50.0f;
  LOG_INFO("Biome System Initialized");
}

void biome_sys_shutdown(void) { LOG_INFO("Biome System Shutdown"); }

void biome_sys_set_temperature(float temperature) {
  biome_state.temperature = temperature;
}

float biome_sys_get_temperature(void) { return biome_state.temperature; }

void biome_sys_set_humidity(float humidity) { biome_state.humidity = humidity; }

float biome_sys_get_humidity(void) { return biome_state.humidity; }

BiomeType biome_sys_get_at_position(float x, float y, float z) {
  // Simple noise-based biome selection
  float noise = sinf(x * 0.01f) * cosf(z * 0.01f);
  float temp = biome_state.temperature + noise * 0.3f;
  float humid = biome_state.humidity + noise * 0.2f;

  if (temp < -0.5f)
    return BIOME_TUNDRA;
  if (temp > 0.7f && humid > 0.6f)
    return BIOME_JUNGLE;
  if (temp > 0.5f && humid < 0.3f)
    return BIOME_DESERT;
  if (humid > 0.7f)
    return BIOME_FOREST;
  if (y < 60.0f)
    return BIOME_OCEAN;
  if (y > 120.0f)
    return BIOME_MOUNTAINS;
  return BIOME_PLAINS;
}

void biome_sys_set_vegetation_density(float density) {
  biome_state.vegetation_density = density;
}

float biome_sys_get_vegetation_density(void) {
  return biome_state.vegetation_density;
}

void biome_sys_set_blend_distance(float distance) {
  biome_state.blend_distance = distance;
}

float biome_sys_get_blend_distance(void) { return biome_state.blend_distance; }
