// include/world/settlement_generation.h
//
// Purpose: Defines the public API for the procedural generation of settlements
// directly within the game world's chunks. This header provides lower-level
// functions responsible for modifying block data in `ChunkManager` and spawning
// NPCs via `NPCSystem` to construct settlements. It complements the higher-level
// settlement management defined in `include/world/settlement.h` by focusing on
// the concrete world manipulation aspects of settlement generation.
//
// Public APIs:
// - `settlement_generate`: Generates a settlement structure at a specific world coordinate
//   (`x`, `y`, `z`) using a given `seed`, applying block changes to the `ChunkManager`
//   and spawning NPCs via the `NPCSystem`. Returns `true` on successful generation.
// - `settlement_generate_in_chunk`: Populates a specified `Chunk` with settlement
//   structures or features and associated NPCs during chunk generation or loading.
//
// Ownership: These functions operate directly on `ChunkManager` and `NPCSystem` instances
// (which are owned elsewhere). They do not own any persistent settlement data structures
// themselves but rather modify the block state of the world and create NPC entities.
//
// Invariants:
// - Valid `ChunkManager` and `NPCSystem` instances must be provided.
// - `seed` ensures reproducible settlement layouts.
// - World coordinates (`x`, `y`, `z`) must be valid for the `ChunkManager`.
// - The block manipulation and NPC spawning performed by these functions must be
//   consistent with the game's block, chunk, and NPC systems.
//
#ifndef SETTLEMENT_GENERATION_H
#define SETTLEMENT_GENERATION_H


#include "../game_common.h"
#include "../chunk/chunk.h"
#include "../npc/npc.h"

// Generate settlement at position
bool settlement_generate(ChunkManager *chunks, NPCSystem *npc_system, i32 x, i32 y, i32 z, u32 seed);

// Generate settlements in chunk
void settlement_generate_in_chunk(ChunkManager *chunks, NPCSystem *npc_system, Chunk *chunk, u32 seed);

#endif // SETTLEMENT_GENERATION_H

