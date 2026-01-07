#ifndef BIOME_SYSTEM_H
#define BIOME_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  BIOME_DESERT = 0,
  BIOME_FOREST = 1,
  BIOME_PLAINS = 2,
  BIOME_TUNDRA = 3,
  BIOME_JUNGLE = 4,
  BIOME_OCEAN = 5,
  BIOME_MOUNTAINS = 6
} BiomeType;

void biome_sys_init(void);
void biome_sys_shutdown(void);

// Temperature/Humidity control
void biome_sys_set_temperature(float temperature);
float biome_sys_get_temperature(void);

void biome_sys_set_humidity(float humidity);
float biome_sys_get_humidity(void);

// Biome queries
BiomeType biome_sys_get_at_position(float x, float y, float z);

// Vegetation
void biome_sys_set_vegetation_density(float density);
float biome_sys_get_vegetation_density(void);

// Blending
void biome_sys_set_blend_distance(float distance);
float biome_sys_get_blend_distance(void);

#endif // BIOME_SYSTEM_H
