// Cave generation system for underground terrain features
#ifndef CAVE_GENERATION_H
#define CAVE_GENERATION_H

#include "../chunk/chunk.h"
#include "../game_common.h"
#include <math/vec3.h>

// Cave types
typedef enum {
  CAVE_TYPE_NOISE,     // Standard noise caves (spaghetti caves)
  CAVE_TYPE_WORM,      // Perlin worms (tunnels)
  CAVE_TYPE_CAVERN,    // Large open caverns
  CAVE_TYPE_LAKE,      // Underground water lakes
  CAVE_TYPE_LAVA_POOL, // Deep lava pools
  CAVE_TYPE_RAVINE,    // Vertical ravines
  CAVE_TYPE_COUNT
} CaveType;

// Cave biome types (separate from surface biomes)
typedef enum {
  CAVE_BIOME_NORMAL,
  CAVE_BIOME_LUSH,      // Moss, glow berries, azalea
  CAVE_BIOME_DRIPSTONE, // Stalactites, stalagmites
  CAVE_BIOME_DEEP_DARK, // Skulk sensors, warden territory
  CAVE_BIOME_CRYSTAL,   // Amethyst geodes (rare)
  CAVE_BIOME_MUSHROOM,  // Underground mushroom fields
  CAVE_BIOME_COUNT
} CaveBiomeType;

// Noise parameters for cave generation
typedef struct {
  f32 frequency;
  f32 threshold; // Density threshold for carving
  f32 lacunarity;
  f32 persistence;
  i32 octaves;
  u32 seed;
} CaveNoiseParams;

// Cave generation configuration
typedef struct {
  CaveNoiseParams noise_params;
  f32 cheese_cave_frequency; // Frequency of creating large caverns
  f32 worm_cave_frequency;   // Frequency of worm tunnels
  f32 water_level;           // Y-level for underground water
  f32 lava_level;            // Y-level for lava
  bool generate_aquifers;
} CaveGenConfig;

// Structure to track cave worms/tunnels
typedef struct {
  Vec3 position;
  Vec3 direction;
  f32 radius;
  f32 length;
  i32 segments;
} CaveWorm;

// Main cave generator system
typedef struct {
  CaveGenConfig config;
  bool initialized;
  // Pointers to other systems if needed
} CaveGenerator;

// Initialization
void cave_generator_init(CaveGenerator *gen, u32 seed);
void cave_generator_free(CaveGenerator *gen);

// Core generation functions
void cave_generator_carve_chunk(CaveGenerator *gen, Chunk *chunk, i32 chunk_x,
                                i32 chunk_y);
void cave_generator_decorate_chunk(CaveGenerator *gen, Chunk *chunk,
                                   i32 chunk_x, i32 chunk_y);

// Cave biome functions
CaveBiomeType cave_generator_get_biome(CaveGenerator *gen, f32 x, f32 y, f32 z);
void cave_generator_set_config(CaveGenerator *gen, const CaveGenConfig *config);

// Specific feature generation
void cave_generator_spawn_worm(CaveGenerator *gen, Chunk *chunk, Vec3 start_pos,
                               f32 length);
void cave_generator_create_aquifer(CaveGenerator *gen, Chunk *chunk, i32 x,
                                   i32 y, i32 z);
void cave_generator_place_stalactite(CaveGenerator *gen, Chunk *chunk, i32 x,
                                     i32 y, i32 z, bool ceiling);

// Utility
f32 cave_generator_sample_density(CaveGenerator *gen, f32 x, f32 y, f32 z);
bool cave_generator_is_cave(CaveGenerator *gen, f32 x, f32 y, f32 z);

#endif // CAVE_GENERATION_H
