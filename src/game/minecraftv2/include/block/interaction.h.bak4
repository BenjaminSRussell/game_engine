// include/block/interaction.h
//
// Purpose: Declares the API for player and entity interaction with blocks in the game world.
// This includes functions for performing raycasts to detect blocks, placing new blocks,
// breaking existing blocks, and querying block properties at specific coordinates.
//
// Public APIs:
// - `BlockRaycastResult`: Structure to hold detailed information about a block hit by a raycast.
// - `block_raycast`: Performs a raycast from an origin in a given direction to find the first block hit.
// - `block_place`: Attempts to place a block of a specified type at given coordinates.
// - `block_break`: Attempts to break a block at given coordinates, optionally returning the dropped block ID.
// - `block_get`: Retrieves the BlockID of the block at specified coordinates.
// - `block_can_place`: Checks if a block can be placed at a given position, considering existing blocks.
//
// Ownership: These functions operate on `ChunkManager` and `BlockRegistry` data,
// modifying the game world's block state. The `BlockRaycastResult` is a temporary
// data structure returned by the raycast function.
//
// Invariants:
// - `ChunkManager` and `BlockRegistry` instances must be valid and initialized.
// - Coordinates (x, y, z) for placement/breaking/getting must be within the valid game world bounds.
// - `block_raycast` relies on accurate collision geometry for blocks.
//
#ifndef BLOCK_INTERACTION_H
#define BLOCK_INTERACTION_H


#include "../game_common.h"
#include "../chunk/chunk.h"
#include "../block/block.h"
#include <math/vec3.h>

// Block raycast result
typedef struct {
    bool hit;
    Vec3 hit_position;
    Vec3 hit_normal;
    BlockID block_id;
    i32 block_x, block_y, block_z;
    f32 distance;
} BlockRaycastResult;

// Perform raycast against blocks
BlockRaycastResult block_raycast(ChunkManager *chunks, Vec3 origin, Vec3 direction, f32 max_distance);

// Place block at position
bool block_place(ChunkManager *chunks, BlockRegistry *registry, i32 x, i32 y, i32 z, BlockID block_id, bool check_replace);

// Break block at position
bool block_break(ChunkManager *chunks, BlockRegistry *registry, i32 x, i32 y, i32 z, BlockID *dropped_block);

// Get block at position
BlockID block_get(ChunkManager *chunks, i32 x, i32 y, i32 z);

// Check if position is valid for block placement
bool block_can_place(ChunkManager *chunks, BlockRegistry *registry, i32 x, i32 y, i32 z);

#endif // BLOCK_INTERACTION_H

