// Ore distribution system configuration and API
#ifndef ORE_DISTRIBUTION_H
#define ORE_DISTRIBUTION_H

#include "../block/block.h"
#include "../game_common.h"

// Ore types for internal tracking
typedef enum {
  ORE_TYPE_COAL,
  ORE_TYPE_IRON,
  ORE_TYPE_COPPER,
  ORE_TYPE_GOLD,
  ORE_TYPE_REDSTONE,
  ORE_TYPE_EMERALD,
  ORE_TYPE_LAPIS,
  ORE_TYPE_DIAMOND,
  ORE_TYPE_QUARTZ,      // Nether
  ORE_TYPE_NETHER_GOLD, // Nether
  ORE_TYPE_DEBRIS,      // Ancient debris
  ORE_TYPE_COUNT
} OreInternalType;

// Configuration for a single ore type's generation
typedef struct {
  OreInternalType type;
  BlockID block_id;
  u32 veins_per_chunk;  // Attempts per chunk
  i32 min_y;            // Minimum generation height
  i32 max_y;            // Maximum generation height
  u32 vein_size;        // Max blocks per vein
  f32 discard_chance;   // Chance to fail generation (0.0-1.0)
  bool triangular_dist; // Use triangular distribution (more centered)
} OreConfig;

// Biome specific ore modifiers
typedef struct {
  OreInternalType type;
  f32 count_multiplier; // 2.0 = double ores
  f32 size_multiplier;  // 1.5 = 50% larger veins
  i32 height_offset;    // Raise/lower generation band
} BiomeOreModifier;

// Initialize ore system
void ore_distribution_init(u32 seed);

// Get default configuration for an ore type
OreConfig ore_create_default_config(OreInternalType type);

// Generate single vein at position
void ore_generate_vein(void *chunk_ptr, const OreConfig *config, i32 x, i32 y,
                       i32 z);

#endif // ORE_DISTRIBUTION_H
