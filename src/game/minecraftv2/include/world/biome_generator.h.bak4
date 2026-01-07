// Biome generator - integrates biome system with world generation
// Assigns biomes to chunks and generates biome-specific features
#ifndef BIOME_GENERATOR_H
#define BIOME_GENERATOR_H

#include "../game_common.h"
#include <math/vec3.h>
#include <math/vec2.h>
#include "biome_features.h"
#include "../chunk/chunk.h"

// Forward declarations
typedef struct ChunkManager ChunkManager;
typedef struct WorldGenerator WorldGenerator;

// Biome noise parameters
typedef struct {
  f32 temperature_scale;
  f32 temperature_offset;
  f32 humidity_scale;
  f32 humidity_offset;
  f32 elevation_scale;
  f32 biome_size;
  u32 noise_seed;
} BiomeNoiseParams;

// Biome transition info
typedef struct {
  BiomeType primary_biome;
  BiomeType secondary_biome;
  f32 blend_factor;  // 0.0 = fully primary, 1.0 = fully secondary
} BiomeTransitionInfo;

// Biome chunk data
typedef struct {
  BiomeType biome;
  BiomeTransitionInfo transition;
  f32 temperature;
  f32 humidity;
  f32 elevation;
} BiomeChunkData;

// Biome generator system
typedef struct {
  BiomeFeaturesSystem *features;
  ChunkManager *chunk_manager;
  BiomeNoiseParams noise_params;
  u32 *biome_map;  // Pre-computed biome map for region
  u32 map_width;
  u32 map_height;
  bool use_noise;
  bool use_smooth_transitions;
  bool initialized;
} BiomeGenerator;

// Initialization
void biome_generator_init(BiomeGenerator *gen, BiomeFeaturesSystem *features,
                         ChunkManager *chunks);
void biome_generator_free(BiomeGenerator *gen);

// Biome assignment for chunks
BiomeChunkData biome_generator_get_biome_for_chunk(BiomeGenerator *gen,
                                                  i32 chunk_x, i32 chunk_y);
void biome_generator_assign_chunk_biome(BiomeGenerator *gen,
                                       Chunk *chunk, i32 chunk_x, i32 chunk_y);

// Biome map generation
void biome_generator_generate_biome_map(BiomeGenerator *gen, i32 center_x,
                                       i32 center_z, u32 width, u32 height);
BiomeType biome_generator_sample_biome_map(BiomeGenerator *gen, f32 x, f32 z);

// Noise-based biome selection
BiomeType biome_generator_select_biome_from_noise(BiomeGenerator *gen,
                                                 f32 temperature, f32 humidity,
                                                 f32 elevation);

// Biome-specific generation
void biome_generator_spawn_plants(BiomeGenerator *gen, Chunk *chunk,
                                 BiomeType biome);
void biome_generator_generate_structures(BiomeGenerator *gen, Chunk *chunk,
                                        BiomeType biome);
void biome_generator_apply_terrain_modifiers(BiomeGenerator *gen,
                                            Chunk *chunk, BiomeType biome);

// Configuration
void biome_generator_set_noise_params(BiomeGenerator *gen,
                                     const BiomeNoiseParams *params);
BiomeNoiseParams biome_generator_get_default_noise_params(void);

// Utility functions
f32 biome_generator_sample_temperature_noise(BiomeGenerator *gen, f32 x, f32 z);
f32 biome_generator_sample_humidity_noise(BiomeGenerator *gen, f32 x, f32 z);
f32 biome_generator_sample_elevation_noise(BiomeGenerator *gen, f32 x, f32 z);

// Decision tree for biome selection based on climate
BiomeType biome_generator_select_from_climate(f32 temperature, f32 humidity,
                                             f32 elevation);

// Statistics
typedef struct {
  u32 chunks_processed;
  u32 biomes_assigned;
  u32 transitions_created;
  f32 generation_time;
} BiomeGenerationStats;

BiomeGenerationStats biome_generator_get_stats(BiomeGenerator *gen);

#endif // BIOME_GENERATOR_H
