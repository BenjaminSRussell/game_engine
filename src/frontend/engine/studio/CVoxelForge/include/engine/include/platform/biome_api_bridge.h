// Biome System API Bridge
// Exposes biome generation and sampling to VoxelForgeStudio

#ifndef BIOME_API_BRIDGE_H
#define BIOME_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Biome System API
// ============================================================================

typedef enum {
  BIOME_DESERT = 0,
  BIOME_FOREST = 1,
  BIOME_PLAINS = 2,
  BIOME_TUNDRA = 3,
  BIOME_JUNGLE = 4,
  BIOME_OCEAN = 5,
  BIOME_MOUNTAINS = 6
} BiomeType;

typedef struct {
  BiomeType type;
  float temperature;        // -1.0 to 1.0
  float humidity;           // 0.0 to 1.0
  float vegetation_density; // 0.0 to 1.0
} BiomeConfig;

/// Create a custom biome
/// @param name Biome name
/// @param config Biome configuration
/// @return Biome ID
uint64_t biome_create(const char *name, BiomeConfig config);

/// Sample biome at world position
BiomeType biome_sample_at(float x, float y, float z);

/// Set biome blend distance
void biome_set_blend_distance(float distance);

/// Get biome blend distance
float biome_get_blend_distance(void);

/// Get biome config
BiomeConfig biome_get_config(BiomeType type);

#ifdef __cplusplus
}
#endif

#endif // BIOME_API_BRIDGE_H
