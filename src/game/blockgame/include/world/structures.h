// include/world/structures.h
//
// Purpose: Defines the public API for the placement and procedural generation
// of various predefined structures within the game world. This header provides
// the necessary functions to incorporate static or procedurally generated
// builds like houses, towers, and dungeons into `Chunk`s, contributing to world
// diversity.
//
// Public APIs:
// - `StructureType`: Enumeration defining different types of structures that
// can
//   be placed or generated (e.g., `STRUCTURE_HOUSE`, `STRUCTURE_DUNGEON`,
//   `STRUCTURE_TOWER`).
// - `structure_place`: Places a specific `StructureType` at given world
// coordinates (`x`, `y`, `z`)
//   by modifying the blocks within the `ChunkManager`. Returns `true` on
//   successful placement.
// - `structure_generate_in_chunk`: Integrates procedural structure generation
// into the
//   world generation pipeline, populating a `Chunk` with appropriate structures
//   based on a `seed`.
//
// Ownership: These functions operate directly on `ChunkManager` and `Chunk`
// instances (which are owned by the world or chunk management systems). They do
// not own any persistent structure data structures themselves but rather modify
// the block state of the world.
//
// Invariants:
// - A valid `ChunkManager` and `Chunk` (if applicable) must be provided.
// - `seed` ensures reproducible structure generation.
// - World coordinates (`x`, `y`, `z`) must be valid for the `ChunkManager`.
// - The block manipulation performed by these functions must be consistent with
//   the game's block and chunk systems.
//
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "../chunk/chunk.h"
#include "../game_common.h"

// Structure types
typedef enum {
  STRUCTURE_HOUSE,
  STRUCTURE_HUT,
  STRUCTURE_TOWER,
  STRUCTURE_DUNGEON,
  STRUCTURE_VILLAGE_PLAINS,
  STRUCTURE_VILLAGE_DESERT,
  STRUCTURE_VILLAGE_SNOW,
  STRUCTURE_TEMPLE_JUNGLE,
  STRUCTURE_TEMPLE_DESERT,
  STRUCTURE_MINESHAFT,
  STRUCTURE_STRONGHOLD,
  STRUCTURE_OCEAN_RUIN,
  STRUCTURE_SHIPWRECK,
  STRUCTURE_IGLOO,
  STRUCTURE_WITCH_HUT,
  STRUCTURE_DESERT_WELL,
  STRUCTURE_FOSSIL,
  STRUCTURE_RUINED_PORTAL,
  STRUCTURE_PILLAGER_OUTPOST,
  STRUCTURE_ANCIENT_CITY,
  STRUCTURE_COUNT
} StructureType;

// Place structure in world
bool structure_place(ChunkManager *chunks, StructureType type, i32 x, i32 y,
                     i32 z);

// Generate structures in chunk
void structure_generate_in_chunk(ChunkManager *chunks, Chunk *chunk, u32 seed);

#endif // STRUCTURES_H
