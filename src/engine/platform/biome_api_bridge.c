#include "../include/platform/biome_api_bridge.h"
#include "core/logger.h"
#include "scene/world/biome/biome_system.h"

void biome_set_temperature(float temperature) {
  biome_sys_set_temperature(temperature);
}

float biome_get_temperature(void) { return biome_sys_get_temperature(); }

void biome_set_humidity(float humidity) { biome_sys_set_humidity(humidity); }

float biome_get_humidity(void) { return biome_sys_get_humidity(); }

PlatformBiomeType biome_get_at_position(float x, float y, float z) {
  return (PlatformBiomeType)biome_sys_get_at_position(x, y, z);
}

void biome_set_vegetation_density(float density) {
  biome_sys_set_vegetation_density(density);
}

float biome_get_vegetation_density(void) {
  return biome_sys_get_vegetation_density();
}

void biome_set_blend_distance(float distance) {
  biome_sys_set_blend_distance(distance);
}

float biome_get_blend_distance(void) { return biome_sys_get_blend_distance(); }
