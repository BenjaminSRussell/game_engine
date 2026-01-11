// include/block/block.h
//
// Purpose: Defines the fundamental structures and API for managing block types
// within the game engine. This includes enumerations for block faces and
// properties, the `BlockType` structure which describes individual block
// characteristics, and the `BlockRegistry` for central management of all
// registered block types. It also provides common block IDs and utility
// functions for querying block properties.
//
// Public APIs:
// - `BlockFace`, `BlockFlags`: Enumerations for block rendering and behavioral
// properties.
// - `BlockType`: Structure defining a block's ID, flags, physical properties
// (hardness, opacity),
//   light emission, and textures for each face.
// - `BlockRegistry`: Structure and functions (`block_registry_init`,
// `block_registry_free`,
//   `block_registry_register`, `block_registry_get`) for dynamic management of
//   block types.
// - `BLOCK_*` macros: Predefined identifiers for common in-game blocks.
// - `block_is_solid`, `block_is_transparent`, etc.: Inline utility functions
// for efficient
//   querying of block flags.
// - `block_registry_init_defaults`: Initializes the registry with a standard
// set of blocks.
//
// Ownership: The `BlockRegistry` owns the `BlockType` data, managing its
// allocation and deallocation. Individual `BlockType` instances are conceptual
// definitions stored within the registry.
//
// Invariants:
// - `BlockRegistry` must be initialized before use and freed afterwards.
// - `BlockID`s are unique identifiers for each block type, with `BLOCK_AIR`
// reserved as 0.
// - `BlockFlags` are bitfields allowing for efficient storage and querying of
// block properties.
// - `texture_indices` array must contain 6 valid texture indices for proper
// rendering.
//
#ifndef BLOCK_H
#define BLOCK_H

#include "../game_common.h"

// Block face directions (for culling and meshing)
typedef enum {
  FACE_NONE = 0,
  FACE_POS_X = 1 << 0,
  FACE_NEG_X = 1 << 1,
  FACE_POS_Y = 1 << 2,
  FACE_NEG_Y = 1 << 3,
  FACE_POS_Z = 1 << 4,
  FACE_NEG_Z = 1 << 5,
  FACE_ALL = 0x3F
} BlockFace;

// Block properties flags (bitfield for efficiency)
typedef enum {
  BLOCK_SOLID = 1 << 0,
  BLOCK_TRANSPARENT = 1 << 1,
  BLOCK_LIQUID = 1 << 2,
  BLOCK_GRAVITY = 1 << 3,
  BLOCK_BREAKABLE = 1 << 4,
  BLOCK_PLACEABLE = 1 << 5,
  BLOCK_LIGHT_EMITTING = 1 << 6,
  BLOCK_ANIMATED = 1 << 7,
  BLOCK_PLANT = 1 << 8,      // Plant - breaks if no solid block below
  BLOCK_WATER_PLANT = 1 << 9 // Water plant - requires water below
} BlockFlags;

// Block type definition
typedef struct {
  BlockID id;
  u16 flags;
  u8 hardness;           // 0-255, affects break time
  u8 light_level;        // 0-15, light emission
  u8 opacity;            // 0-15, light blocking
  u8 texture_indices[6]; // One per face (pos_x, neg_x, pos_y, neg_y, pos_z,
                         // neg_z)
} BlockType;

// Block registry (global, initialized at startup)
typedef struct BlockRegistry {
  BlockType *types;
  u32 count;
  u32 capacity;
} BlockRegistry;

// Block IDs (predefined common blocks)
#define BLOCK_AIR 0
#define BLOCK_STONE 1
#define BLOCK_GRASS 2
#define BLOCK_DIRT 3
#define BLOCK_COBBLESTONE 4
#define BLOCK_WOOD 5
#define BLOCK_LEAVES 6
#define BLOCK_SAND 7
#define BLOCK_GRAVEL 8
#define BLOCK_WATER 9
#define BLOCK_LAVA 10
#define BLOCK_GLASS 11
#define BLOCK_BEDROCK 12
#define BLOCK_FURNACE 13
#define BLOCK_SOLAR_PANEL 14
#define BLOCK_RUBBER_WOOD 15
#define BLOCK_RUBBER_LEAVES 16
#define BLOCK_OAK_LOG 17
#define BLOCK_OAK_LEAVES 18
#define BLOCK_BIRCH_LOG 19
#define BLOCK_BIRCH_LEAVES 20
#define BLOCK_SPRUCE_LOG 21
#define BLOCK_SPRUCE_LEAVES 22
#define BLOCK_IRON_ORE 30
#define BLOCK_GOLD_ORE 31
#define BLOCK_EMERALD_ORE 32
#define BLOCK_DIAMOND_ORE 22
#define BLOCK_COAL_ORE 23
#define BLOCK_RAIL 50
#define BLOCK_POWERED_RAIL 51
#define BLOCK_DETECTOR_RAIL 52
#define BLOCK_LADDER 53
#define BLOCK_BOOKSHELF 47
#define BLOCK_ICE 54
#define BLOCK_SNOW 55
#define BLOCK_SOUL_SAND 56
#define BLOCK_VINE 57
#define BLOCK_CRAFTING_TABLE 60
#define BLOCK_MAGMA_BLOCK 100
#define BLOCK_COUNT 256

// Plant blocks (decorative, no collision)
#define BLOCK_TALL_GRASS 80
#define BLOCK_SHORT_GRASS 81
#define BLOCK_FLOWER_RED 82
#define BLOCK_FLOWER_YELLOW 83
#define BLOCK_FLOWER_BLUE 84
#define BLOCK_FLOWER_WHITE 85
#define BLOCK_MUSHROOM_BROWN 86
#define BLOCK_MUSHROOM_RED 87
#define BLOCK_LILY_PAD 88
#define BLOCK_DEAD_BUSH 89
#define BLOCK_FERN 90
#define BLOCK_CACTUS 91

// Block registry functions
void block_registry_init(BlockRegistry *registry, u32 capacity);
void block_registry_free(BlockRegistry *registry);
BlockID block_registry_register(BlockRegistry *registry, BlockType type);
const BlockType *block_registry_get(const BlockRegistry *registry, BlockID id);

// Block property queries (inline for performance)
INLINE bool block_is_solid(const BlockType *block) {
  return block && (block->flags & BLOCK_SOLID) != 0;
}

INLINE bool block_is_transparent(const BlockType *block) {
  return block && (block->flags & BLOCK_TRANSPARENT) != 0;
}

INLINE bool block_is_liquid(const BlockType *block) {
  return block && (block->flags & BLOCK_LIQUID) != 0;
}

INLINE bool block_has_gravity(const BlockType *block) {
  return block && (block->flags & BLOCK_GRAVITY) != 0;
}

INLINE bool block_emits_light(const BlockType *block) {
  return block && (block->flags & BLOCK_LIGHT_EMITTING) != 0;
}

INLINE bool block_is_plant(const BlockType *block) {
  return block && (block->flags & BLOCK_PLANT) != 0;
}

INLINE bool block_is_water_plant(const BlockType *block) {
  return block && (block->flags & BLOCK_WATER_PLANT) != 0;
}

// Initialize default blocks
void block_registry_init_defaults(BlockRegistry *registry);

// Block registry validation system
bool block_registry_validate(const BlockRegistry *registry);

// Block registry search/filter functionality
const BlockType *block_registry_find_by_id(const BlockRegistry *registry,
                                           BlockID id);
u32 block_registry_find_by_flags(const BlockRegistry *registry,
                                 BlockFlags flags, BlockType *results,
                                 u32 max_results);

// Block registry serialization/deserialization for mod support
void block_registry_serialize(const BlockRegistry *registry, FILE *file);
bool block_registry_deserialize(BlockRegistry *registry, FILE *file);

#endif // BLOCK_H
