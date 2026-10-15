/*
 * Force-included (-include) into mega_terrain.c and the mega_terrain vegetation
 * test (issue #141). Not part of the engine.
 *
 * mega_terrain.c does not compile on its own: it calls snprintf without
 * <stdio.h>, and its terrain_get_biome_at(TerrainSystem *, ...) conflicts with
 * the prototype in src/engine/procedural/terrain/advanced_terrain.h (tracked in
 * #275). Defining ADVANCED_TERRAIN_H skips that header. This shim supplies the
 * only things the unit takes from it: the TerrainData type and the two erosion
 * prototypes. Fixing #275 removes the need for this shim.
 */

#ifndef MEGA_TERRAIN_TEST_SHIM_H
#define MEGA_TERRAIN_TEST_SHIM_H

#define ADVANCED_TERRAIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Layout matches advanced_terrain.h. */
typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  uint32_t width, height, depth;
  float *heightmap;
  float *moisture_map;
  float *temperature_map;
  uint8_t *biome_map;
  Vec3 *normal_map;
} TerrainData;

bool terrain_thermal_erosion(TerrainData *terrain, float rate,
                             uint32_t iterations);
bool terrain_hydraulic_erosion(TerrainData *terrain, float rate,
                               uint32_t iterations);

#endif /* MEGA_TERRAIN_TEST_SHIM_H */
