// include/world/ore.h
//
// Purpose: Defines the public API and data structures for the procedural
// generation of ores within the game world. This header provides the framework
// for defining different `OreType`s, configuring their generation parameters
// (rarity, depth, vein size), and orchestrating the placement of ore veins
// into chunks during world generation.
//
// Public APIs:
// - `OreType`: Enumeration defining various types of mineable ores (e.g., Coal, Iron, Diamond).
// - `OreVein`: Structure representing a cluster of ore blocks, including its `type`,
//   `position`, `size`, `block_count`, and generation status.
// - `OreConfig`: Structure to define the generation parameters for a specific `OreType`,
//   such as its `block_id`, `rarity`, vertical distribution (`min_y`, `max_y`),
//   `min_vein_size`, `max_vein_size`, and `cluster_count` per chunk.
// - `OreGenerator`: The main structure managing all ore generation configurations and a seed.
// - `ore_generator_init`: Initializes the ore generator with a global seed.
// - `ore_generator_free`: Frees resources held by the ore generator.
// - `ore_configure`: Sets up the generation parameters for a specific `OreType`.
// - `ore_generate_in_chunk`: The primary function for generating all configured ores
//   within a specified `Chunk`.
// - `ore_generate_vein`: Generates a single `OreVein` instance based on type and position.
// - `ore_place_vein_in_chunk`: Applies the blocks of a generated `OreVein` to a `Chunk`.
//
// Ownership: The `OreGenerator` owns its `OreConfig` array.
// `OreVein`s are temporary structures during generation or managed by the chunk system once placed.
//
// Invariants:
// - An `OreGenerator` must be initialized with `ore_generator_init` before ore configuration or generation.
// - `seed` ensures reproducible ore distribution.
// - `BlockID` (from `block.h`) and `Chunk` (from `chunk.h`) structures are assumed to be correctly defined.
// - `rarity` is a normalized float value, and `min_y`/`max_y` define vertical bounds within the world.
//
#ifndef ORE_H
#define ORE_H


#include "../game_common.h"
#include "../block/block.h"
#include "../chunk/chunk.h"

// Ore types
typedef enum {
    ORE_TYPE_COAL,
    ORE_TYPE_IRON,
    ORE_TYPE_COPPER,
    ORE_TYPE_GOLD,
    ORE_TYPE_SILVER,
    ORE_TYPE_DIAMOND,
    ORE_TYPE_EMERALD,
    ORE_TYPE_RUBY,
    ORE_TYPE_URANIUM,
    ORE_TYPE_COUNT
} OreType;

// Ore vein
typedef struct {
    OreType type;
    Vec3 position;
    f32 size;
    u32 block_count;
    bool generated;
} OreVein;

// Ore generation parameters
typedef struct {
    OreType type;
    BlockID block_id;
    f32 rarity;        // 0.0 to 1.0
    i32 min_y;
    i32 max_y;
    u32 min_vein_size;
    u32 max_vein_size;
    u32 cluster_count; // Per chunk
} OreConfig;

// Ore generator
typedef struct {
    OreConfig configs[ORE_TYPE_COUNT];
    u32 seed;
} OreGenerator;

// Initialize ore generator
void ore_generator_init(OreGenerator *gen, u32 seed);
void ore_generator_free(OreGenerator *gen);

// Configure ore type
void ore_configure(OreGenerator *gen, OreType type, BlockID block_id, f32 rarity, i32 min_y, i32 max_y, u32 min_size, u32 max_size, u32 clusters);

// Generate ores in chunk
void ore_generate_in_chunk(OreGenerator *gen, Chunk *chunk, ChunkPos pos);

// Generate ore vein
OreVein ore_generate_vein(OreGenerator *gen, OreType type, Vec3 position, u32 seed);

// Place ore in chunk
void ore_place_vein_in_chunk(OreVein *vein, Chunk *chunk);

#endif // ORE_H

