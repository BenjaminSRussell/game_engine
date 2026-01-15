#include "unified_logger.h"
#include "unified_memory.h"
#include "world_types_private.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// WORLD MANAGEMENT
// =============================================================================

World *world_create(const WorldConfig *config) {
  if (!config) {
    LOG_ERROR(LOG_CAT_SYSTEM, "Invalid world config");
    return NULL;
  }

  World *world = (World *)UNIFIED_ALLOC(sizeof(World));
  if (!world) {
    LOG_ERROR(LOG_CAT_SYSTEM, "Failed to allocate world");
    return NULL;
  }
  memset(world, 0, sizeof(World));

  // Copy config
  world->config = *config;
  if (!world->config.chunk_size)
    world->config.chunk_size = 16;
  if (!world->config.render_distance)
    world->config.render_distance = 8;

  // Initialize chunk storage
  world->chunk_capacity = 256;
  world->chunks =
      (Chunk **)UNIFIED_ALLOC(sizeof(Chunk *) * world->chunk_capacity);
  memset(world->chunks, 0, sizeof(Chunk *) * world->chunk_capacity);

  world->initialized = true;

  LOG_INFO(LOG_CAT_SYSTEM, "World '%s' created (seed=%u, chunk_size=%u)",
           config->name ? config->name : "Unnamed", config->seed,
           world->config.chunk_size);

  return world;
}

void world_destroy(World *world) {
  if (!world)
    return;

  LOG_INFO(LOG_CAT_SYSTEM, "Destroying world '%s'", world->config.name);

  // Unload all chunks
  for (uint32_t i = 0; i < world->chunk_count; i++) {
    if (world->chunks[i]) {
      destroy_chunk(world->chunks[i]);
    }
  }

  UNIFIED_FREE(world->chunks);
  UNIFIED_FREE(world);
}

void world_update(World *world, float delta_time) {
  if (!world)
    return;

  // TODO: Update world systems (weather, time, etc.)
  (void)delta_time; // Unused for now
}

void world_set_player_position(World *world, float x, float y, float z) {
  if (!world)
    return;

  world->player_x = x;
  world->player_y = y;
  world->player_z = z;
}

const char *world_get_name(World *world) {
  return world ? world->config.name : NULL;
}

uint32_t world_get_seed(World *world) { return world ? world->config.seed : 0; }

uint32_t world_get_loaded_chunk_count(World *world) {
  return world ? world->chunk_count : 0;
}

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

Chunk *find_chunk(World *world, int32_t chunk_x, int32_t chunk_z) {
  if (!world)
    return NULL;

  for (uint32_t i = 0; i < world->chunk_count; i++) {
    Chunk *chunk = world->chunks[i];
    if (chunk && chunk->position.x == chunk_x && chunk->position.z == chunk_z) {
      return chunk;
    }
  }

  return NULL;
}

Chunk *create_chunk(World *world, int32_t chunk_x, int32_t chunk_z) {
  if (!world)
    return NULL;

  // Check capacity
  if (world->chunk_count >= world->chunk_capacity) {
    LOG_WARN(LOG_CAT_SYSTEM,
             "Chunk capacity reached, cannot create more chunks");
    return NULL;
  }

  Chunk *chunk = (Chunk *)UNIFIED_ALLOC(sizeof(Chunk));
  if (!chunk)
    return NULL;

  memset(chunk, 0, sizeof(Chunk));
  chunk->position.x = chunk_x;
  chunk->position.z = chunk_z;

  // Allocate chunk data
  uint32_t size = world->config.chunk_size;
  chunk->blocks = (uint32_t *)UNIFIED_ALLOC(sizeof(uint32_t) * size * 256 *
                                            size); // 256 = height
  chunk->heightmap = (float *)UNIFIED_ALLOC(sizeof(float) * size * size);
  chunk->biomes = (BiomeType *)UNIFIED_ALLOC(sizeof(BiomeType) * size * size);

  if (!chunk->blocks || !chunk->heightmap || !chunk->biomes) {
    UNIFIED_FREE(chunk->blocks);
    UNIFIED_FREE(chunk->heightmap);
    UNIFIED_FREE(chunk->biomes);
    UNIFIED_FREE(chunk);
    return NULL;
  }

  memset(chunk->blocks, 0, sizeof(uint32_t) * size * 256 * size);
  memset(chunk->heightmap, 0, sizeof(float) * size * size);
  memset(chunk->biomes, 0, sizeof(BiomeType) * size * size);

  chunk->is_loaded = true;
  chunk->is_generated = false;

  // Add to world
  world->chunks[world->chunk_count++] = chunk;

  LOG_INFO(LOG_CAT_SYSTEM, "Created chunk at (%d, %d)", chunk_x, chunk_z);

  return chunk;
}

void destroy_chunk(Chunk *chunk) {
  if (!chunk)
    return;

  UNIFIED_FREE(chunk->blocks);
  UNIFIED_FREE(chunk->heightmap);
  UNIFIED_FREE(chunk->biomes);
  UNIFIED_FREE(chunk);
}

void generate_chunk_terrain(World *world, Chunk *chunk) {
  if (!world || !chunk || chunk->is_generated)
    return;

  // Simple procedural generation
  uint32_t size = world->config.chunk_size;
  uint32_t seed = world->config.seed;

  for (uint32_t x = 0; x < size; x++) {
    for (uint32_t z = 0; z < size; z++) {
      // Simple height generation using seed
      float world_x = (float)(chunk->position.x * size + x);
      float world_z = (float)(chunk->position.z * size + z);

      // Basic noise function (simplified)
      float height = 64.0f + sinf(world_x * 0.1f + seed) * 16.0f +
                     cosf(world_z * 0.1f + seed) * 16.0f;

      chunk->heightmap[z * size + x] = height;

      // Simple biome assignment
      if (height > 90.0f)
        chunk->biomes[z * size + x] = BIOME_MOUNTAINS;
      else if (height < 50.0f)
        chunk->biomes[z * size + x] = BIOME_OCEAN;
      else
        chunk->biomes[z * size + x] = BIOME_PLAINS;
    }
  }

  chunk->is_generated = true;
  world->total_chunks_generated++;
}

// =============================================================================
// CHUNK MANAGEMENT
// =============================================================================

Chunk *world_get_chunk(World *world, int32_t chunk_x, int32_t chunk_z) {
  if (!world)
    return NULL;
  return find_chunk(world, chunk_x, chunk_z);
}

Chunk *world_get_chunk_at_world_pos(World *world, float x, float z) {
  if (!world)
    return NULL;

  int32_t chunk_x, chunk_z;
  world_to_chunk_pos(x, z, world->config.chunk_size, &chunk_x, &chunk_z);
  return find_chunk(world, chunk_x, chunk_z);
}

void world_load_chunk(World *world, int32_t chunk_x, int32_t chunk_z) {
  if (!world)
    return;

  // Check if already loaded
  Chunk *existing = find_chunk(world, chunk_x, chunk_z);
  if (existing)
    return;

  // Create and generate
  Chunk *chunk = create_chunk(world, chunk_x, chunk_z);
  if (chunk && world->config.enable_terrain_generation) {
    generate_chunk_terrain(world, chunk);
  }
}

void world_unload_chunk(World *world, int32_t chunk_x, int32_t chunk_z) {
  if (!world)
    return;

  for (uint32_t i = 0; i < world->chunk_count; i++) {
    Chunk *chunk = world->chunks[i];
    if (chunk && chunk->position.x == chunk_x && chunk->position.z == chunk_z) {
      destroy_chunk(chunk);

      // Remove from array
      for (uint32_t j = i; j < world->chunk_count - 1; j++) {
        world->chunks[j] = world->chunks[j + 1];
      }
      world->chunk_count--;

      LOG_INFO(LOG_CAT_SYSTEM, "Unloaded chunk at (%d, %d)", chunk_x, chunk_z);
      return;
    }
  }
}

void world_load_chunks_around(World *world, float center_x, float center_z,
                              uint32_t radius) {
  if (!world)
    return;

  int32_t center_chunk_x, center_chunk_z;
  world_to_chunk_pos(center_x, center_z, world->config.chunk_size,
                     &center_chunk_x, &center_chunk_z);

  for (int32_t dx = -(int32_t)radius; dx <= (int32_t)radius; dx++) {
    for (int32_t dz = -(int32_t)radius; dz <= (int32_t)radius; dz++) {
      world_load_chunk(world, center_chunk_x + dx, center_chunk_z + dz);
    }
  }
}

bool world_is_chunk_loaded(World *world, int32_t chunk_x, int32_t chunk_z) {
  return find_chunk(world, chunk_x, chunk_z) != NULL;
}

void world_get_loaded_chunks(World *world, ChunkPos **out_chunks,
                             uint32_t *out_count) {
  if (!world || !out_chunks || !out_count)
    return;

  *out_count = world->chunk_count;
  *out_chunks =
      (ChunkPos *)UNIFIED_ALLOC(sizeof(ChunkPos) * world->chunk_count);

  for (uint32_t i = 0; i < world->chunk_count; i++) {
    (*out_chunks)[i] = world->chunks[i]->position;
  }
}

// =============================================================================
// TERRAIN QUERIES
// =============================================================================

float world_get_height(World *world, float x, float z) {
  if (!world)
    return 0.0f;

  Chunk *chunk = world_get_chunk_at_world_pos(world, x, z);
  if (!chunk || !chunk->is_generated)
    return 64.0f; // Default sea level

  uint32_t size = world->config.chunk_size;
  int32_t local_x = (int32_t)x - chunk->position.x * size;
  int32_t local_z = (int32_t)z - chunk->position.z * size;

  if (local_x < 0 || local_x >= (int32_t)size || local_z < 0 ||
      local_z >= (int32_t)size) {
    return 64.0f;
  }

  return chunk->heightmap[local_z * size + local_x];
}

BiomeType world_get_biome(World *world, float x, float z) {
  if (!world)
    return BIOME_PLAINS;

  Chunk *chunk = world_get_chunk_at_world_pos(world, x, z);
  if (!chunk || !chunk->is_generated)
    return BIOME_PLAINS;

  uint32_t size = world->config.chunk_size;
  int32_t local_x = (int32_t)x - chunk->position.x * size;
  int32_t local_z = (int32_t)z - chunk->position.z * size;

  if (local_x < 0 || local_x >= (int32_t)size || local_z < 0 ||
      local_z >= (int32_t)size) {
    return BIOME_PLAINS;
  }

  return chunk->biomes[local_z * size + local_x];
}

const char *world_get_biome_name(BiomeType biome) {
  switch (biome) {
  case BIOME_PLAINS:
    return "Plains";
  case BIOME_FOREST:
    return "Forest";
  case BIOME_DESERT:
    return "Desert";
  case BIOME_MOUNTAINS:
    return "Mountains";
  case BIOME_OCEAN:
    return "Ocean";
  case BIOME_SNOW:
    return "Snow";
  case BIOME_JUNGLE:
    return "Jungle";
  case BIOME_SWAMP:
    return "Swamp";
  default:
    return "Unknown";
  }
}

uint32_t world_get_block(World *world, int32_t x, int32_t y, int32_t z) {
  if (!world || y < 0 || y >= 256)
    return 0;

  Chunk *chunk = world_get_chunk_at_world_pos(world, (float)x, (float)z);
  if (!chunk)
    return 0;

  uint32_t size = world->config.chunk_size;
  int32_t local_x = x - chunk->position.x * size;
  int32_t local_z = z - chunk->position.z * size;

  if (local_x < 0 || local_x >= (int32_t)size || local_z < 0 ||
      local_z >= (int32_t)size) {
    return 0;
  }

  uint32_t index =
      (uint32_t)y * size * size + (uint32_t)local_z * size + (uint32_t)local_x;
  return chunk->blocks[index];
}

void world_set_block(World *world, int32_t x, int32_t y, int32_t z,
                     uint32_t block_id) {
  if (!world || y < 0 || y >= 256)
    return;

  Chunk *chunk = world_get_chunk_at_world_pos(world, (float)x, (float)z);
  if (!chunk)
    return;

  uint32_t size = world->config.chunk_size;
  int32_t local_x = x - chunk->position.x * size;
  int32_t local_z = z - chunk->position.z * size;

  if (local_x < 0 || local_x >= (int32_t)size || local_z < 0 ||
      local_z >= (int32_t)size) {
    return;
  }

  uint32_t index =
      (uint32_t)y * size * size + (uint32_t)local_z * size + (uint32_t)local_x;
  chunk->blocks[index] = block_id;
  world->total_blocks_modified++;
}

// =============================================================================
// UTILITY
// =============================================================================

void world_to_chunk_pos(float x, float z, uint32_t chunk_size,
                        int32_t *out_chunk_x, int32_t *out_chunk_z) {
  *out_chunk_x = (int32_t)floorf(x / (float)chunk_size);
  *out_chunk_z = (int32_t)floorf(z / (float)chunk_size);
}

void chunk_to_world_pos(int32_t chunk_x, int32_t chunk_z, uint32_t chunk_size,
                        float *out_x, float *out_z) {
  *out_x = (float)(chunk_x * chunk_size);
  *out_z = (float)(chunk_z * chunk_size);
}
