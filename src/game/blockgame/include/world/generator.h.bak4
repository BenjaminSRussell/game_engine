// include/world/generator.h
//
// Purpose: Defines the public API and data structures for the game's procedural
// world generation system. This comprehensive header provides the tools and
// framework to create diverse and dynamic game worlds, encompassing terrain
// shaping, cave systems, ore distribution, biome definition, and structural
// placement. It orchestrates various generation stages using different noise
// functions and biome definitions.
//
// Public APIs:
// - `NoiseType`: Enumeration defining various types of noise algorithms
// (Perlin, Simplex, FBM).
// - `NoiseGenerator`: Structure encapsulating parameters for a specific noise
// function,
//   including seed, frequency, amplitude, octaves, lacunarity, and persistence.
// - `BiomeType`: Enumeration defining different types of biomes (e.g., Ocean,
// Plains, Desert, Forest).
// - `Biome`: Structure defining the properties of a specific biome, such as its
//   surface/subsurface/filler blocks, temperature, humidity, and height
//   characteristics.
// - `GenerationContext`: Structure providing context for generation,
// particularly access to `ChunkManager`.
// - `WorldGenerator`: The main structure managing all aspects of world
// generation,
//   including multiple `NoiseGenerator` instances for different features
//   (height, temp, humidity, etc.), a collection of `Biome` definitions, and a
//   global `seed`.
// - `world_generator_init`: Initializes the world generator with a global seed
// and generation context.
// - `world_generator_free`: Frees resources held by the world generator.
// - `world_generator_generate_chunk`: The primary entry point for generating
// all features within a given `Chunk`.
// - Modular generation pipeline stages: `world_generator_generate_terrain`,
// `world_generator_generate_caves`,
//   `world_generator_generate_ores`,
//   `world_generator_generate_biome_decorations`,
//   `world_generator_generate_structures` for executing specific parts of the
//   generation process.
// - Noise functions: `noise_perlin`, `noise_simplex`, `noise_fbm`,
// `noise_ridged`, `noise_billow`
//   for generating various types of procedural noise.
// - Biome functions: `world_generator_get_biome`,
// `world_generator_get_biome_def` for querying biome information.
//
// Ownership: The `WorldGenerator` instance owns its `NoiseGenerator` instances
// and `Biome` array. It holds a reference to a `GenerationContext` but does not
// own it.
//
// Invariants:
// - A `WorldGenerator` must be initialized with `world_generator_init` before
// use.
// - `seed` ensures reproducible world generation.
// - `Chunk` and `BlockRegistry` (from their respective headers) are assumed to
// be correctly defined.
// - Noise parameters (frequency, amplitude, octaves, etc.) significantly affect
// the generated terrain.
//
#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include "../block/block.h"
#include "../chunk/chunk.h"
#include "../game_common.h"

// Noise function types
typedef enum {
  NOISE_PERLIN,
  NOISE_SIMPLEX,
  NOISE_FBM,    // Fractional Brownian Motion
  NOISE_RIDGED, // Ridged multi-fractal (mountains)
  NOISE_BILLOW, // Billow noise (clouds, rolling hills)
  NOISE_WORLEY  // Worley/Cellular noise
} NoiseType;

// Noise generator
typedef struct {
  NoiseType type;
  u32 seed;
  f32 frequency;
  f32 amplitude;
  u32 octaves;
  f32 lacunarity;
  f32 persistence;
} NoiseGenerator;

#include "biome_features.h"

// Note: BiomeType enum is now defined in biome_features.h

// Biome definition
typedef struct {
  BiomeType type;
  BlockID surface_block;
  BlockID subsurface_block;
  BlockID filler_block;
  f32 temperature;
  f32 humidity;
  f32 height_min;
  f32 height_max;
} Biome;

// Generation context (replaces global state)
typedef struct {
  struct ChunkManager *chunk_manager; // For structure generation across chunks
  struct NPCSystem *npc_system;       // For procedural NPC spawning
} GenerationContext;

// World generator
typedef struct WorldGenerator {
  NoiseGenerator height_noise;
  NoiseGenerator temperature_noise;
  NoiseGenerator humidity_noise;
  NoiseGenerator cave_noise;
  NoiseGenerator ore_noise;
  Biome *biomes;
  u32 biome_count;
  u32 seed;
  GenerationContext *context; // Context for chunk manager access
} WorldGenerator;

// Initialize world generator
void world_generator_init(WorldGenerator *gen, u32 seed,
                          GenerationContext *context);
void world_generator_free(WorldGenerator *gen);

// Main generation entry point (calls modular pipeline)
void world_generator_generate_chunk(WorldGenerator *gen, Chunk *chunk);

// Modular generation pipeline stages
void world_generator_generate_terrain(WorldGenerator *gen, Chunk *chunk);
void world_generator_generate_caves(WorldGenerator *gen, Chunk *chunk);
void world_generator_generate_ores(WorldGenerator *gen, Chunk *chunk);
void world_generator_generate_biome_decorations(WorldGenerator *gen,
                                                Chunk *chunk);
void world_generator_generate_structures(WorldGenerator *gen, Chunk *chunk);

// Legacy/compatibility functions
void world_generator_generate_chunk_biomes(WorldGenerator *gen, Chunk *chunk);
void world_generator_generate_chunk_with_transitions(WorldGenerator *gen,
                                                     Chunk *chunk);
void world_generator_generate_tree(WorldGenerator *gen, Chunk *chunk, i32 x,
                                   i32 y, i32 z);

// Noise functions
f32 noise_perlin(NoiseGenerator *gen, f32 x, f32 y, f32 z);
f32 noise_simplex(NoiseGenerator *gen, f32 x, f32 y, f32 z);
f32 noise_fbm(NoiseGenerator *gen, f32 x, f32 y, f32 z);
f32 noise_ridged(NoiseGenerator *gen, f32 x, f32 y, f32 z);
f32 noise_billow(NoiseGenerator *gen, f32 x, f32 y, f32 z);
f32 noise_worley(NoiseGenerator *gen, f32 x, f32 y, f32 z);

// Biome functions
BiomeType world_generator_get_biome(WorldGenerator *gen, i32 x, i32 y, i32 z);
const Biome *world_generator_get_biome_def(WorldGenerator *gen, BiomeType type);

#endif // WORLD_GENERATOR_H
