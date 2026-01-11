// include/npc/pathfinding.h
//
// Purpose: Defines the public API for NPC pathfinding functionality, enabling
// non-player characters to intelligently navigate the game world. This header
// provides functions to calculate a path between a starting point and a goal,
// taking into account the world's block geometry and traversable areas. It
// integrates with the ECS, chunk management, and block registry systems.
//
// Public APIs:
// - `npc_find_path`: The primary pathfinding function. It calculates a sequence
//   of `Vec3` points from a `start` position to a `goal` position for a given
//   entity, considering the current `ChunkManager` and `BlockRegistry` for world
//   information. The resulting path is stored in the provided `path` array,
//   with its length returned in `path_length`.
// - `npc_is_walkable`: A helper function that determines if a specific block
//   coordinate (`x`, `y`, `z`) is traversable by an NPC, based on block properties.
//
// Ownership: These functions operate on existing data structures like `ECSWorld`,
// `ChunkManager`, and `BlockRegistry` (which are owned elsewhere). The `path` array
// passed to `npc_find_path` is owned by the caller.
//
// Invariants:
// - Valid `ECSWorld`, `ChunkManager`, and `BlockRegistry` instances must be provided.
// - `start` and `goal` `Vec3` positions should be valid world coordinates.
// - `path` must be a pre-allocated array capable of holding `max_path_length` `Vec3` points.
// - Pathfinding algorithm's success depends on the complexity of the terrain and
//   the `max_path_length` constraint.
//
// Roadmap: docs/NPC_PATHFINDING_ROADMAP.md.
//
#ifndef PATHFINDING_H
#define PATHFINDING_H


#include "../game_common.h"
#include "../chunk/chunk.h"
#include <math/vec3.h>
#include "../block/block.h"

// Forward declarations
struct ECSWorld;

// Pathfinding functions
bool npc_find_path(struct ECSWorld *ecs, EntityID entity, ChunkManager *chunk_manager, BlockRegistry *block_registry, Vec3 start, Vec3 goal, Vec3 *path, u32 *path_length, u32 max_path_length);
bool npc_is_walkable(ChunkManager *chunk_manager, BlockRegistry *block_registry, i32 x, i32 y, i32 z);

#endif // PATHFINDING_H
