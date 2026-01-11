// include/world/tree_varieties.h
//
// Purpose: Defines the public API for the procedural generation of various
// tree types within the game world. This header provides an enumeration for
// different `TreeType`s and a collection of functions responsible for generating
// specific tree geometries (logs and leaves) at given coordinates within a `Chunk`.
// It is a core component of the world generation system for populating landscapes.
//
// Public APIs:
// - `TreeType`: Enumeration defining various species of trees (e.g., `TREE_OAK`, `TREE_BIRCH`, `TREE_JUNGLE`).
// - `tree_generate`: A generalized function to generate a tree of a specified `TreeType`
//   at world coordinates (`x`, `y`, `z`) within a `Chunk`, using a given `seed` for variations.
// - Individual tree generators: `tree_generate_oak`, `tree_generate_birch`, `tree_generate_spruce`,
//   `tree_generate_rubber`, `tree_generate_jungle` provide specialized logic for each tree type.
//
// Ownership: These functions operate directly on `Chunk` instances (which are owned by
// the chunk management system). They do not own any persistent tree data structures
// themselves but rather modify the block state of the world to create trees.
//
// Invariants:
// - A valid `Chunk` must be provided, and `x`, `y`, `z` coordinates must be within its bounds.
// - `seed` ensures reproducible tree generation patterns.
// - The block manipulation performed by these functions must be consistent with the
//   game's block and chunk systems.
// - Assumes `Chunk` (from `chunk.h`) is correctly defined.
//
#ifndef TREE_VARIETIES_H
#define TREE_VARIETIES_H


#include "../game_common.h"
#include "../chunk/chunk.h"

// Tree types
typedef enum {
    TREE_OAK = 0,
    TREE_BIRCH = 1,
    TREE_SPRUCE = 2,
    TREE_RUBBER = 3,
    TREE_JUNGLE = 4,
    TREE_COUNT = 5
} TreeType;

// Generate specific tree type
void tree_generate(Chunk *chunk, i32 x, i32 y, i32 z, TreeType type, u32 seed);

// Individual tree generators
void tree_generate_oak(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed);
void tree_generate_birch(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed);
void tree_generate_spruce(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed);
void tree_generate_rubber(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed);
void tree_generate_jungle(Chunk *chunk, i32 x, i32 y, i32 z, u32 seed);

#endif // TREE_VARIETIES_H

