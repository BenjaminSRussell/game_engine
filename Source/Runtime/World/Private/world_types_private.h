#pragma once

#include "../Public/unified_world.h"

// Chunk structure
struct Chunk {
  ChunkPos position;
  uint32_t *blocks;  // 3D block data
  float *heightmap;  // 2D height data
  BiomeType *biomes; // 2D biome data
  bool is_loaded;
  bool is_generated;
  uint64_t last_access_time;
};

// World structure
struct World {
  WorldConfig config;
  bool initialized;

  // Player tracking (for chunk loading)
  float player_x;
  float player_y;
  float player_z;

  // Chunk storage
  Chunk **chunks;
  uint32_t chunk_count;
  uint32_t chunk_capacity;

  // Statistics
  uint64_t total_chunks_generated;
  uint64_t total_blocks_modified;
};

// Internal helpers
Chunk *find_chunk(World *world, int32_t chunk_x, int32_t chunk_z);
Chunk *create_chunk(World *world, int32_t chunk_x, int32_t chunk_z);
void destroy_chunk(Chunk *chunk);
void generate_chunk_terrain(World *world, Chunk *chunk);
