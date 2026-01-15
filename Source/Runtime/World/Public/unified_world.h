#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct World World;
typedef struct Chunk Chunk;
typedef struct WorldConfig WorldConfig;

// World coordinates
typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} WorldPos;

typedef struct {
  int32_t x;
  int32_t z;
} ChunkPos;

// Biome types
typedef enum {
  BIOME_PLAINS,
  BIOME_FOREST,
  BIOME_DESERT,
  BIOME_MOUNTAINS,
  BIOME_OCEAN,
  BIOME_SNOW,
  BIOME_JUNGLE,
  BIOME_SWAMP,
  BIOME_COUNT
} BiomeType;

// World configuration
struct WorldConfig {
  const char *name;
  uint32_t seed;
  uint32_t chunk_size;
  uint32_t render_distance;
  bool enable_terrain_generation;
  bool enable_biomes;
};

// =============================================================================
// WORLD MANAGEMENT
// =============================================================================

// Create/destroy world
World *world_create(const WorldConfig *config);
void world_destroy(World *world);

// World update
void world_update(World *world, float delta_time);
void world_set_player_position(World *world, float x, float y, float z);

// World info
const char *world_get_name(World *world);
uint32_t world_get_seed(World *world);
uint32_t world_get_loaded_chunk_count(World *world);

// =============================================================================
// CHUNK MANAGEMENT
// =============================================================================

// Chunk access
Chunk *world_get_chunk(World *world, int32_t chunk_x, int32_t chunk_z);
Chunk *world_get_chunk_at_world_pos(World *world, float x, float z);

// Chunk loading
void world_load_chunk(World *world, int32_t chunk_x, int32_t chunk_z);
void world_unload_chunk(World *world, int32_t chunk_x, int32_t chunk_z);
void world_load_chunks_around(World *world, float center_x, float center_z,
                              uint32_t radius);

// Chunk queries
bool world_is_chunk_loaded(World *world, int32_t chunk_x, int32_t chunk_z);
void world_get_loaded_chunks(World *world, ChunkPos **out_chunks,
                             uint32_t *out_count);

// =============================================================================
// TERRAIN QUERIES
// =============================================================================

// Height queries
float world_get_height(World *world, float x, float z);
float world_get_height_at_chunk(World *world, Chunk *chunk, uint32_t local_x,
                                uint32_t local_z);

// Block access
uint32_t world_get_block(World *world, int32_t x, int32_t y, int32_t z);
void world_set_block(World *world, int32_t x, int32_t y, int32_t z,
                     uint32_t block_id);

// Biome access
BiomeType world_get_biome(World *world, float x, float z);
const char *world_get_biome_name(BiomeType biome);

// =============================================================================
// UTILITY
// =============================================================================

// Coordinate conversion
void world_to_chunk_pos(float x, float z, uint32_t chunk_size,
                        int32_t *out_chunk_x, int32_t *out_chunk_z);
void chunk_to_world_pos(int32_t chunk_x, int32_t chunk_z, uint32_t chunk_size,
                        float *out_x, float *out_z);
