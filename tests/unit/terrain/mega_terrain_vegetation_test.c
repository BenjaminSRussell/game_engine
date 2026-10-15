/*
 * Biome-specific vegetation placement in mega_terrain.c (issue #141).
 *
 * terrain_update_biome_effects() must set the tree_count of the loaded chunk
 * containing (x, z) from that point's biome tree_density, and streaming must
 * place vegetation for each chunk it loads. Without the change every
 * tree_count stays 0 and these checks fail.
 *
 * Compiled against mega_terrain.c directly; see mega_terrain_test_shim.h.
 */

#include "mega_terrain.h"

#include <stdint.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                            \
    }                                                                        \
  } while (0)

/* Defined in mega_terrain.c but not declared in mega_terrain.h. */
bool terrain_system_init(TerrainSystem *system, const TerrainConfig *config);
void terrain_system_shutdown(TerrainSystem *system);
void terrain_system_update(TerrainSystem *system, float dt);

/* mega_terrain.c calls these erosion functions; the test does not use them. */
bool terrain_hydraulic_erosion(TerrainData *terrain, float rate,
                               uint32_t iterations) {
  (void)terrain;
  (void)rate;
  (void)iterations;
  return true;
}

bool terrain_thermal_erosion(TerrainData *terrain, float rate,
                             uint32_t iterations) {
  (void)terrain;
  (void)rate;
  (void)iterations;
  return true;
}

/* Sample points, and the biome terrain_get_biome_at() returns for each:
 *   (0, 0)         PLAINS  in chunk (0, 0)
 *   (-1570.796, 0) SNOW    in chunk (-7, 0)
 *   (0, 1570.796)  SWAMP   in chunk (0, 6)
 */
#define PLAINS_X 0.0f
#define PLAINS_Z 0.0f
#define SNOW_X -1570.796f
#define SNOW_Z 0.0f
#define SWAMP_X 0.0f
#define SWAMP_Z 1570.796f

/* Candidate tree cells in one chunk. */
#define CELLS_PER_CHUNK                                                       \
  ((uint32_t)((TERRAIN_CHUNK_SIZE / TERRAIN_TREE_CELL_SIZE) *                 \
              (TERRAIN_CHUNK_SIZE / TERRAIN_TREE_CELL_SIZE)))

int main(void) {
  TerrainConfig config = {0};
  TerrainSystem system;
  CHECK(terrain_system_init(&system, &config));

  /* A biome that was not configured has no trees. */
  CHECK(system.biomes[BIOME_SWAMP].tree_density == 0.0f);

  /* Three chunks: two loaded under different biomes, one unloaded. */
  system.chunks[0].chunk_x = 0;
  system.chunks[0].chunk_z = 0;
  system.chunks[0].is_loaded = true;
  system.chunks[1].chunk_x = -7;
  system.chunks[1].chunk_z = 0;
  system.chunks[1].is_loaded = true;
  system.chunks[2].chunk_x = 0;
  system.chunks[2].chunk_z = 6;
  system.chunks[2].is_loaded = false;
  system.chunks[2].tree_count = 7;
  system.chunk_count = 3;

  system.biomes[BIOME_PLAINS].tree_density = 0.5f;
  system.biomes[BIOME_SNOW].tree_density = 0.25f;
  system.biomes[BIOME_SWAMP].tree_density = 0.5f;

  /* Each biome's density sets its own chunk's tree count. */
  terrain_update_biome_effects(&system, PLAINS_X, PLAINS_Z);
  CHECK(system.chunks[0].tree_count == CELLS_PER_CHUNK / 2);

  terrain_update_biome_effects(&system, SNOW_X, SNOW_Z);
  CHECK(system.chunks[1].tree_count == CELLS_PER_CHUNK / 4);
  CHECK(system.chunks[0].tree_count == CELLS_PER_CHUNK / 2);

  /* An unloaded chunk is not placed into. */
  terrain_update_biome_effects(&system, SWAMP_X, SWAMP_Z);
  CHECK(system.chunks[2].tree_count == 7);

  /* Zero density clears the count; density above 1 caps at one tree per cell. */
  system.biomes[BIOME_PLAINS].tree_density = 0.0f;
  terrain_update_biome_effects(&system, PLAINS_X, PLAINS_Z);
  CHECK(system.chunks[0].tree_count == 0);

  system.biomes[BIOME_PLAINS].tree_density = 2.0f;
  terrain_update_biome_effects(&system, PLAINS_X, PLAINS_Z);
  CHECK(system.chunks[0].tree_count == CELLS_PER_CHUNK);

  terrain_update_biome_effects(NULL, PLAINS_X, PLAINS_Z);

  terrain_system_shutdown(&system);

  /* Streaming: the camera at the origin loads chunk (0, 0) on the first
   * update, since it is one of the four nearest chunks. Loading must place
   * vegetation for it using the PLAINS density. */
  TerrainSystem streamed;
  CHECK(terrain_system_init(&streamed, &config));
  streamed.biomes[BIOME_PLAINS].tree_density = 0.5f;
  terrain_system_update(&streamed, 1.0f / 60.0f);

  int found = 0;
  for (uint32_t i = 0; i < streamed.chunk_count; i++) {
    TerrainChunk *chunk = &streamed.chunks[i];
    if (chunk->chunk_x == 0 && chunk->chunk_z == 0) {
      CHECK(chunk->is_loaded);
      CHECK(chunk->tree_count == CELLS_PER_CHUNK / 2);
      found++;
    }
  }
  CHECK(found == 1);

  terrain_system_shutdown(&streamed);

  if (failures) {
    fprintf(stderr, "test_mega_terrain_vegetation: %d check(s) failed\n",
            failures);
    return 1;
  }
  printf("test_mega_terrain_vegetation: all checks passed\n");
  return 0;
}
