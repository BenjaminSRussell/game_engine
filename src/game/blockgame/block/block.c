// src/block/block.c
//
// Module Overview:
// This module provides the concrete implementation of the `BlockRegistry`
// system, which serves as the central repository for all defined block types in
// the game. Its primary responsibilities include initializing and managing the
// memory for block definitions, allowing for dynamic registration of new
// blocks, and providing efficient lookup mechanisms to retrieve `BlockType`
// data using their unique `BlockID`s. It also contains the hardcoded
// definitions for a comprehensive set of default blocks used in world
// generation and gameplay.
//
// Key Flows:
// 1. **Initialization (`block_registry_init`):** Allocates memory for the
// internal
//    array of `BlockType`s based on a specified capacity.
// 2. **Default Block Population (`block_registry_init_defaults`):** Registers a
// predefined
//    set of common game blocks (e.g., Air, Stone, Grass, Dirt, various Ores,
//    Plants) with their respective properties like `BlockFlags`, `hardness`,
//    `light_level`, `opacity`, and `texture_indices`. This function manually
//    defines each block's characteristics.
// 3. **Registration (`block_registry_register`):** Adds a new `BlockType`
// definition
//    to the registry, assigning it a unique `BlockID`. It includes dynamic
//    resizing logic if the registry's capacity is exceeded.
// 4. **Retrieval (`block_registry_get`):** Provides read-only access to a
// `BlockType`
//    definition given its `BlockID`.
// 5. **Cleanup (`block_registry_free`):** Deallocates all memory used by the
// `BlockRegistry`.
//
// Invariants:
// - A `BlockRegistry` must be initialized before any `block_registry_register`
// or `block_registry_get` calls.
// - `BlockID`s are assigned sequentially starting from 0, with predefined
// macros (`BLOCK_AIR`, etc.)
//   ensuring consistency for common blocks.
// - Each `BlockType` entry contains `BlockFlags` which are bitfields
// representing various
//   physical and rendering properties.
// - `texture_indices` array must contain valid indices into the game's texture
// atlas.
// - The module notes that the default definitions are hardcoded and mentions
// future
//   support for mods and localization.
//
// (Additional comments from the file indicating external dependencies and
// generation processes are preserved.) Block registry setup and default block
// definitions. Roadmap: docs/BLOCK_DEFS_ROADMAP.md.
// Block registry serialization/deserialization for mod support
// Block property inheritance system for block variants
// Block registry search/filter functionality
// Block registry versioning for save compatibility
// Block registry hot-reload support for development
// Block registry compression for memory efficiency
#include "engine/include/core/logger.h"
#include <core/types.h>
#include <block/block.h>
#include <block/lighting.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void block_registry_init(BlockRegistry *registry, u32 capacity) {
  registry->types = (BlockType *)malloc(sizeof(BlockType) * capacity);
  registry->count = 0;
  registry->capacity = capacity;
  memset(registry->types, 0, sizeof(BlockType) * capacity);
}

void block_registry_free(BlockRegistry *registry) {
  if (registry->types) {
    free(registry->types);
    registry->types = NULL;
  }
  registry->count = 0;
  registry->capacity = 0;
}

BlockID block_registry_register(BlockRegistry *registry, BlockType type) {
  if (registry->count >= registry->capacity) {
    // Resize (shouldn't happen often)
    u32 new_capacity = registry->capacity * 2;
    registry->types =
        (BlockType *)realloc(registry->types, sizeof(BlockType) * new_capacity);
    registry->capacity = new_capacity;
  }

  registry->types[registry->count] = type;
  return registry->count++;
}

const BlockType *block_registry_get(const BlockRegistry *registry, BlockID id) {
  if (!registry || id >= registry->count) {
    return NULL;
  }
  return &registry->types[id];
}

// hardcoded; support mods and localization.
// invariants.
void block_registry_init_defaults(BlockRegistry *registry) {
  // Air
  BlockType air = {.id = BLOCK_AIR,
                   .flags = 0,
                   .hardness = 0,
                   .light_level = 0,
                   .opacity = 0,
                   .texture_indices = {0, 0, 0, 0, 0, 0}};
  block_registry_register(registry, air);

  // Stone
  BlockType stone = {.id = BLOCK_STONE,
                     .flags = BLOCK_SOLID | BLOCK_BREAKABLE,
                     .hardness = 50,
                     .light_level = 0,
                     .opacity = 15,
                     .texture_indices = {1, 1, 1, 1, 1, 1}};
  block_registry_register(registry, stone);

  // Grass
  BlockType grass = {
      .id = BLOCK_GRASS,
      .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
      .hardness = 20,
      .light_level = 0,
      .opacity = 15,
      .texture_indices = {2, 3, 2, 3, 2, 2}
      // Top: grass, bottom: dirt, sides: grass side
  };
  block_registry_register(registry, grass);

  // Dirt
  BlockType dirt = {.id = BLOCK_DIRT,
                    .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                    .hardness = 15,
                    .light_level = 0,
                    .opacity = 15,
                    .texture_indices = {3, 3, 3, 3, 3, 3}};
  block_registry_register(registry, dirt);

  // Cobblestone
  BlockType cobblestone = {.id = BLOCK_COBBLESTONE,
                           .flags =
                               BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                           .hardness = 40,
                           .light_level = 0,
                           .opacity = 15,
                           .texture_indices = {4, 4, 4, 4, 4, 4}};
  block_registry_register(registry, cobblestone);

  // Wood
  BlockType wood = {
      .id = BLOCK_WOOD,
      .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
      .hardness = 25,
      .light_level = 0,
      .opacity = 15,
      .texture_indices = {5, 5, 6, 6, 5, 5}
      // Top/bottom: wood rings, sides: wood bark
  };
  block_registry_register(registry, wood);

  // Leaves
  BlockType leaves = {.id = BLOCK_LEAVES,
                      .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE |
                               BLOCK_TRANSPARENT,
                      .hardness = 5,
                      .light_level = 0,
                      .opacity = 1,
                      .texture_indices = {7, 7, 7, 7, 7, 7}};
  block_registry_register(registry, leaves);

  // Sand
  BlockType sand = {.id = BLOCK_SAND,
                    .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE |
                             BLOCK_GRAVITY,
                    .hardness = 10,
                    .light_level = 0,
                    .opacity = 15,
                    .texture_indices = {8, 8, 8, 8, 8, 8}};
  block_registry_register(registry, sand);

  // Gravel
  BlockType gravel = {.id = BLOCK_GRAVEL,
                      .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE |
                               BLOCK_GRAVITY,
                      .hardness = 12,
                      .light_level = 0,
                      .opacity = 15,
                      .texture_indices = {9, 9, 9, 9, 9, 9}};
  block_registry_register(registry, gravel);

  // Water
  BlockType water = {.id = BLOCK_WATER,
                     .flags = BLOCK_LIQUID | BLOCK_TRANSPARENT,
                     .hardness = 0,
                     .light_level = 0,
                     .opacity = 1,
                     .texture_indices = {10, 10, 10, 10, 10, 10}};
  block_registry_register(registry, water);

  // Lava
  BlockType lava = {.id = BLOCK_LAVA,
                    .flags = BLOCK_LIQUID | BLOCK_LIGHT_EMITTING,
                    .hardness = 0,
                    .light_level = 15,
                    .opacity = 15,
                    .texture_indices = {11, 11, 11, 11, 11, 11}};
  block_registry_register(registry, lava);

  // Glass
  BlockType glass = {.id = BLOCK_GLASS,
                     .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE |
                              BLOCK_TRANSPARENT,
                     .hardness = 5,
                     .light_level = 0,
                     .opacity = 0,
                     .texture_indices = {12, 12, 12, 12, 12, 12}};
  block_registry_register(registry, glass);

  // Bedrock
  BlockType bedrock = {.id = BLOCK_BEDROCK,
                       .flags = BLOCK_SOLID,
                       .hardness = 255,
                       .light_level = 0,
                       .opacity = 15,
                       .texture_indices = {13, 13, 13, 13, 13, 13}};
  block_registry_register(registry, bedrock);

  // Furnace
  BlockType furnace = {
      .id = BLOCK_FURNACE,
      .flags = BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
      .hardness = 35,
      .light_level = 0,
      .opacity = 15,
      .texture_indices = {14, 14, 15, 14, 14, 14} // Front has special texture
  };
  block_registry_register(registry, furnace);

  // Solar Panel
  BlockType solar_panel = {.id = BLOCK_SOLAR_PANEL,
                           .flags =
                               BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                           .hardness = 20,
                           .light_level = 0,
                           .opacity = 15,
                           .texture_indices = {16, 16, 16, 16, 16, 16}};
  block_registry_register(registry, solar_panel);

  // Rubber Wood
  BlockType rubber_wood = {.id = BLOCK_RUBBER_WOOD,
                           .flags =
                               BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                           .hardness = 25,
                           .light_level = 0,
                           .opacity = 15,
                           .texture_indices = {17, 17, 18, 18, 17, 17}};
  block_registry_register(registry, rubber_wood);

  // Rubber Leaves
  BlockType rubber_leaves = {.id = BLOCK_RUBBER_LEAVES,
                             .flags = BLOCK_SOLID | BLOCK_BREAKABLE |
                                      BLOCK_PLACEABLE | BLOCK_TRANSPARENT,
                             .hardness = 5,
                             .light_level = 0,
                             .opacity = 1,
                             .texture_indices = {19, 19, 19, 19, 19, 19}};
  block_registry_register(registry, rubber_leaves);

  // Iron Ore
  BlockType iron_ore = {.id = BLOCK_IRON_ORE,
                        .flags =
                            BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                        .hardness = 45,
                        .light_level = 0,
                        .opacity = 15,
                        .texture_indices = {20, 20, 20, 20, 20, 20}};
  block_registry_register(registry, iron_ore);

  // Gold Ore
  BlockType gold_ore = {.id = BLOCK_GOLD_ORE,
                        .flags =
                            BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                        .hardness = 50,
                        .light_level = 0,
                        .opacity = 15,
                        .texture_indices = {21, 21, 21, 21, 21, 21}};
  block_registry_register(registry, gold_ore);

  // Diamond Ore
  BlockType diamond_ore = {.id = BLOCK_DIAMOND_ORE,
                           .flags =
                               BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                           .hardness = 60,
                           .light_level = 0,
                           .opacity = 15,
                           .texture_indices = {22, 22, 22, 22, 22, 22}};
  block_registry_register(registry, diamond_ore);

  // Coal Ore
  BlockType coal_ore = {.id = BLOCK_COAL_ORE,
                        .flags =
                            BLOCK_SOLID | BLOCK_BREAKABLE | BLOCK_PLACEABLE,
                        .hardness = 30,
                        .light_level = 0,
                        .opacity = 15,
                        .texture_indices = {23, 23, 23, 23, 23, 23}};
  block_registry_register(registry, coal_ore);

  // Plant blocks (decorative, transparent, no collision)
  // Tall Grass
  BlockType tall_grass = {.id = BLOCK_TALL_GRASS,
                          .flags =
                              BLOCK_TRANSPARENT | BLOCK_BREAKABLE | BLOCK_PLANT,
                          .hardness = 0,
                          .light_level = 0,
                          .opacity = 0,
                          .texture_indices = {80, 80, 80, 80, 80, 80}};
  block_registry_register(registry, tall_grass);

  // Short Grass
  BlockType short_grass = {.id = BLOCK_SHORT_GRASS,
                           .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                    BLOCK_PLANT,
                           .hardness = 0,
                           .light_level = 0,
                           .opacity = 0,
                           .texture_indices = {81, 81, 81, 81, 81, 81}};
  block_registry_register(registry, short_grass);

  // Red Flower
  BlockType flower_red = {.id = BLOCK_FLOWER_RED,
                          .flags =
                              BLOCK_TRANSPARENT | BLOCK_BREAKABLE | BLOCK_PLANT,
                          .hardness = 0,
                          .light_level = 0,
                          .opacity = 0,
                          .texture_indices = {82, 82, 82, 82, 82, 82}};
  block_registry_register(registry, flower_red);

  // Yellow Flower
  BlockType flower_yellow = {.id = BLOCK_FLOWER_YELLOW,
                             .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                      BLOCK_PLANT,
                             .hardness = 0,
                             .light_level = 0,
                             .opacity = 0,
                             .texture_indices = {83, 83, 83, 83, 83, 83}};
  block_registry_register(registry, flower_yellow);

  // Blue Flower
  BlockType flower_blue = {.id = BLOCK_FLOWER_BLUE,
                           .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                    BLOCK_PLANT,
                           .hardness = 0,
                           .light_level = 0,
                           .opacity = 0,
                           .texture_indices = {84, 84, 84, 84, 84, 84}};
  block_registry_register(registry, flower_blue);

  // White Flower
  BlockType flower_white = {.id = BLOCK_FLOWER_WHITE,
                            .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                     BLOCK_PLANT,
                            .hardness = 0,
                            .light_level = 0,
                            .opacity = 0,
                            .texture_indices = {85, 85, 85, 85, 85, 85}};
  block_registry_register(registry, flower_white);

  // Brown Mushroom
  BlockType mushroom_brown = {.id = BLOCK_MUSHROOM_BROWN,
                              .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                       BLOCK_PLANT,
                              .hardness = 0,
                              .light_level = 1, // Slight glow
                              .opacity = 0,
                              .texture_indices = {86, 86, 86, 86, 86, 86}};
  block_registry_register(registry, mushroom_brown);

  // Red Mushroom
  BlockType mushroom_red = {.id = BLOCK_MUSHROOM_RED,
                            .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                     BLOCK_PLANT,
                            .hardness = 0,
                            .light_level = 1, // Slight glow
                            .opacity = 0,
                            .texture_indices = {87, 87, 87, 87, 87, 87}};
  block_registry_register(registry, mushroom_red);

  // Lily Pad
  BlockType lily_pad = {.id = BLOCK_LILY_PAD,
                        .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE |
                                 BLOCK_WATER_PLANT,
                        .hardness = 0,
                        .light_level = 0,
                        .opacity = 0,
                        .texture_indices = {88, 88, 88, 88, 88, 88}};
  block_registry_register(registry, lily_pad);

  // Dead Bush
  BlockType dead_bush = {.id = BLOCK_DEAD_BUSH,
                         .flags =
                             BLOCK_TRANSPARENT | BLOCK_BREAKABLE | BLOCK_PLANT,
                         .hardness = 0,
                         .light_level = 0,
                         .opacity = 0,
                         .texture_indices = {89, 89, 89, 89, 89, 89}};
  block_registry_register(registry, dead_bush);

  // Fern
  BlockType fern = {.id = BLOCK_FERN,
                    .flags = BLOCK_TRANSPARENT | BLOCK_BREAKABLE | BLOCK_PLANT,
                    .hardness = 0,
                    .light_level = 0,
                    .opacity = 0,
                    .texture_indices = {90, 90, 90, 90, 90, 90}};
  block_registry_register(registry, fern);

  // Cactus
  BlockType cactus = {.id = BLOCK_CACTUS,
                      .flags = BLOCK_SOLID | BLOCK_BREAKABLE |
                               BLOCK_PLANT, // Solid for collision
                      .hardness = 5,
                      .light_level = 0,
                      .opacity = 0,
                      .texture_indices = {91, 91, 91, 91, 91, 91}};
  block_registry_register(registry, cactus);
}

// Block registry validation system to detect invalid block definitions
bool block_registry_validate(const BlockRegistry *registry) {
  if (!registry || !registry->types) {
    LOG_ERROR("Invalid block registry: null pointer");
    return false;
  }

  if (registry->count > registry->capacity) {
    LOG_ERROR("Invalid block registry: count (%u) exceeds capacity (%u)",
              registry->count, registry->capacity);
    return false;
  }

  for (u32 i = 0; i < registry->count; i++) {
    const BlockType *block = &registry->types[i];

    // Validate block ID
    if (block->id >= BLOCK_COUNT) {
      LOG_ERROR("Invalid block ID %u at index %u (max: %d)", block->id, i,
                BLOCK_COUNT);
      return false;
    }

    // Validate hardness
    if (block->hardness < 0 || block->hardness > 100) {
      LOG_ERROR("Invalid hardness %.1f for block %u", (double)block->hardness,
                block->id);
      return false;
    }

    // Validate light level
    if (block->light_level < 0 || block->light_level > 15) {
      LOG_ERROR("Invalid light level %u for block %u", block->light_level,
                block->id);
      return false;
    }

    // Validate opacity
    if (block->opacity > 15) {
      LOG_ERROR("Invalid opacity %u for block %u", block->opacity, block->id);
      return false;
    }
  }

  LOG_INFO("Block registry validation passed for %u blocks", registry->count);
  return true;
}

// Block registry search/filter functionality
const BlockType *block_registry_find_by_id(const BlockRegistry *registry,
                                           BlockID id) {
  if (!registry || !registry->types)
    return NULL;

  for (u32 i = 0; i < registry->count; i++) {
    if (registry->types[i].id == id) {
      return &registry->types[i];
    }
  }

  return NULL;
}

// Find blocks by flags (filter functionality)
u32 block_registry_find_by_flags(const BlockRegistry *registry,
                                 BlockFlags flags, BlockType *results,
                                 u32 max_results) {
  if (!registry || !registry->types || !results || max_results == 0)
    return 0;

  u32 found = 0;
  for (u32 i = 0; i < registry->count && found < max_results; i++) {
    if ((registry->types[i].flags & flags) == flags) {
      results[found++] = registry->types[i];
    }
  }

  return found;
}

// Block registry versioning for save compatibility
#define BLOCK_REGISTRY_VERSION 1

typedef struct {
  u32 version;
  u32 block_count;
  u32 capacity;
} BlockRegistryHeader;

void block_registry_serialize(const BlockRegistry *registry, FILE *file) {
  if (!registry || !file)
    return;

  BlockRegistryHeader header = {.version = BLOCK_REGISTRY_VERSION,
                                .block_count = registry->count,
                                .capacity = registry->capacity};

  fwrite(&header, sizeof(header), 1, file);
  fwrite(registry->types, sizeof(BlockType), registry->count, file);

  LOG_INFO("Serialized %u blocks to file (version %u)", registry->count,
           header.version);
}

bool block_registry_deserialize(BlockRegistry *registry, FILE *file) {
  if (!registry || !file)
    return false;

  BlockRegistryHeader header;
  if (fread(&header, sizeof(header), 1, file) != 1) {
    LOG_ERROR("Failed to read block registry header");
    return false;
  }

  if (header.version != BLOCK_REGISTRY_VERSION) {
    LOG_WARN("Block registry version mismatch: expected %u, got %u",
             BLOCK_REGISTRY_VERSION, header.version);
    // Could implement version conversion here
  }

  // Ensure registry has enough capacity
  if (registry->capacity < header.block_count) {
    LOG_ERROR("Registry capacity too small: need %u, have %u",
              header.block_count, registry->capacity);
    return false;
  }

  if (fread(registry->types, sizeof(BlockType), header.block_count, file) !=
      header.block_count) {
    LOG_ERROR("Failed to read block data");
    return false;
  }

  registry->count = header.block_count;

  LOG_INFO("Deserialized %u blocks from file (version %u)", registry->count,
           header.version);
  return true;
}
