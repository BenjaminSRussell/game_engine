// include/world/dungeon_generation.h
//
// Purpose: Defines the public API for the procedural generation of dungeons
// directly within the game world's chunks. This header provides lower-level
// functions responsible for modifying block data in `ChunkManager` and `Chunk`
// instances to construct dungeons, complementing the higher-level dungeon
// management defined in `include/world/dungeon.h`.
//
// Public APIs:
// - `dungeon_generate`: Generates a dungeon structure at a specific world coordinate
//   (`x`, `y`, `z`) using a given `seed`, and applies these changes to the `ChunkManager`.
//   Returns `true` on successful generation.
// - `dungeon_generate_in_chunk`: Populates a specified `Chunk` with dungeon structures
//   or features, often used during chunk generation or loading.
//
// Ownership: These functions operate directly on `ChunkManager` and `Chunk` instances
// (which are owned by the world or chunk management systems). They do not own any
// persistent dungeon data structures themselves but rather modify the block state of the world.
//
// Invariants:
// - A valid `ChunkManager` and `Chunk` (if applicable) must be provided.
// - `seed` ensures reproducible dungeon layouts.
// - World coordinates (`x`, `y`, `z`) must be valid for the `ChunkManager`.
// - The block manipulation performed by these functions must be consistent with
//   the game's block and chunk systems.
//
#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H


#include "../game_common.h"
#include "../chunk/chunk.h"

// Generate dungeon at position
bool dungeon_generate(ChunkManager *chunks, i32 x, i32 y, i32 z, u32 seed);

// Generate dungeons in chunk
void dungeon_generate_in_chunk(ChunkManager *chunks, Chunk *chunk, u32 seed);

#endif // DUNGEON_GENERATION_H

