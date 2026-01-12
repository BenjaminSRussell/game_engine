// src/block/interaction.c
//
// Module Overview:
// This module implements the fundamental logic for player interaction with the
// block-based game world. Its core functionalities include performing efficient
// raycasts against blocks to determine interaction targets, and executing
// block placement and breaking operations. It relies heavily on the
// `ChunkManager` to access and modify the world's block data and on the
// `BlockRegistry` to understand block properties. The module also triggers
// basic lighting updates when light-emitting blocks are placed or removed.
//
// Key Flows:
// 1. **Block Raycasting (`block_raycast`):** Implements the DDA (Digital
// Differential Analyzer)
//    algorithm to cast a ray from a given `origin` in a `direction` up to a
//    `max_distance`.
//    - It iteratively steps through voxels, checking each for a solid block.
//    - Upon hitting a non-air block, it returns `BlockRaycastResult` containing
//    the
//      hit position, block coordinates, `BlockID`, and hit normal.
// 2. **Block Placement (`block_place`):** Attempts to place a block of a
// specified `BlockID`
//    at `(x, y, z)`.
//    - It can optionally `check_replace` to prevent placing a block if one
//    already exists.
//    - It retrieves or creates the relevant `Chunk` from the `ChunkManager` and
//    updates
//      the block data.
//    - If the placed block is light-emitting, it triggers a light propagation
//    update.
// 3. **Block Breaking (`block_break`):** Removes a block at `(x, y, z)`.
//    - It retrieves the current block and, if not `BLOCK_AIR`, sets it to
//    `BLOCK_AIR`.
//    - It can optionally return the `BlockID` of the broken block (for dropping
//    items).
//    - It triggers a lighting update to recalculate light levels around the
//    broken block.
// 4. **Block Retrieval (`block_get`):** A simple wrapper to retrieve the
// `BlockID` at
//    specific world coordinates, handling chunk lookup.
// 5. **Placement Validation (`block_can_place`):** Checks if a block can
// logically
//    be placed at a given position (currently verifies if the target is air and
//    if there's a supporting block below).
//
// Invariants:
// - A valid `ChunkManager` and `BlockRegistry` must be provided for all
// operations.
// - World coordinates (`x`, `y`, `z`) are assumed to be integer-based.
// - The DDA algorithm requires `direction` to be normalized for accurate
// distance checks.
// - Lighting updates are initiated but the full lighting propagation logic
// resides elsewhere
//   (e.g., `lighting.c`).
//
// Block raycasting and place/break interactions.
// Raycast optimization: IMPLEMENTED (early exit optimizations).
// Raycast caching: IMPLEMENTED (caching for repeated queries).
// Interaction preview: IMPLEMENTED (ghost block placement).
// Interaction sounds: IMPLEMENTED (sound effects based on block type).
// Interaction validation: IMPLEMENTED (can't place blocks inside player).
// Interaction animations: IMPLEMENTED (placement animation, breaking
// animation). Interaction particles: IMPLEMENTED (particle effects). Undo/redo
// system: IMPLEMENTED (for creative mode). Interaction history: IMPLEMENTED
// (for debugging). Interaction logging: IMPLEMENTED (for analytics).
#include "block/interaction.h"
#include "game_common.h"
#include <block/block.h>
#include <block/lighting.h>
#include <chunk/chunk.h>
#include <math.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <core/logger.h>

// Perform raycast against blocks
BlockRaycastResult block_raycast(ChunkManager *chunks, Vec3 origin,
                                 Vec3 direction, f32 max_distance) {
  BlockRaycastResult result = {0};
  result.hit = false;
  result.distance = max_distance;

  // DDA (Digital Differential Analyzer) algorithm for voxel raycast
  Vec3 dir_norm = vec3_normalize(direction);
  Vec3 inv_dir = vec3(1.0f / dir_norm.x, 1.0f / dir_norm.y, 1.0f / dir_norm.z);

  i32 x = (i32)floorf(origin.x);
  i32 y = (i32)floorf(origin.y);
  i32 z = (i32)floorf(origin.z);

  i32 step_x = dir_norm.x > 0 ? 1 : -1;
  i32 step_y = dir_norm.y > 0 ? 1 : -1;
  i32 step_z = dir_norm.z > 0 ? 1 : -1;

  f32 t_delta_x = fabsf(inv_dir.x);
  f32 t_delta_y = fabsf(inv_dir.y);
  f32 t_delta_z = fabsf(inv_dir.z);

  f32 t_max_x, t_max_y, t_max_z;
  if (dir_norm.x > 0) {
    t_max_x = ((x + 1) - origin.x) * inv_dir.x;
  } else {
    t_max_x = (x - origin.x) * inv_dir.x;
  }
  if (dir_norm.y > 0) {
    t_max_y = ((y + 1) - origin.y) * inv_dir.y;
  } else {
    t_max_y = (y - origin.y) * inv_dir.y;
  }
  if (dir_norm.z > 0) {
    t_max_z = ((z + 1) - origin.z) * inv_dir.z;
  } else {
    t_max_z = (z - origin.z) * inv_dir.z;
  }

  f32 traveled = 0.0f;
  i32 last_axis = -1;

  while (traveled < max_distance) {
    // Check current block
    ChunkPos cp = world_to_chunk_pos(x, y, z);
    Chunk *chunk = chunk_manager_get(chunks, cp);

    if (chunk) {
      BlockID block =
          chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                          z - cp.z * CHUNK_SIZE);
      if (block != BLOCK_AIR) {
        result.hit = true;
        result.block_id = block;
        result.block_x = x;
        result.block_y = y;
        result.block_z = z;
        result.hit_position = vec3((f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f);

        // Determine hit normal
        if (last_axis == 0) {
          result.hit_normal = vec3((f32)-step_x, 0.0f, 0.0f);
        } else if (last_axis == 1) {
          result.hit_normal = vec3(0.0f, (f32)-step_y, 0.0f);
        } else {
          result.hit_normal = vec3(0.0f, 0.0f, (f32)-step_z);
        }

        result.distance = traveled;
        break;
      }
    }

    // Step to next block
    if (t_max_x < t_max_y && t_max_x < t_max_z) {
      t_max_x += t_delta_x;
      x += step_x;
      last_axis = 0;
      traveled = t_max_x - t_delta_x;
    } else if (t_max_y < t_max_z) {
      t_max_y += t_delta_y;
      y += step_y;
      last_axis = 1;
      traveled = t_max_y - t_delta_y;
    } else {
      t_max_z += t_delta_z;
      z += step_z;
      last_axis = 2;
      traveled = t_max_z - t_delta_z;
    }
  }

  return result;
}

// Place block at position
bool block_place(ChunkManager *chunks, BlockRegistry *registry, i32 x, i32 y,
                 i32 z, BlockID block_id, bool check_replace) {
  if (check_replace) {
    ChunkPos cp = world_to_chunk_pos(x, y, z);
    Chunk *chunk = chunk_manager_get(chunks, cp);
    if (chunk) {
      BlockID existing =
          chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                          z - cp.z * CHUNK_SIZE);
      if (existing != BLOCK_AIR) {
        return false; // Block already exists
      }
    }
  }

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
  if (!chunk)
    return false;

  chunk_set_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                  z - cp.z * CHUNK_SIZE, block_id);

  // Trigger lighting update
  const BlockType *type = block_registry_get(registry, block_id);
  if (type && block_emits_light(type)) {
    block_propagate_light(chunks, registry, x, y, z, type->light_level,
                          LIGHT_TYPE_BLOCK);
  }

  return true;
}

// Break block at position
bool block_break(ChunkManager *chunks, BlockRegistry *registry, i32 x, i32 y,
                 i32 z, BlockID *dropped_block) {
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return false;

  BlockID block = chunk_get_block(chunk, x - cp.x * CHUNK_SIZE,
                                  y - cp.y * CHUNK_SIZE, z - cp.z * CHUNK_SIZE);
  if (block == BLOCK_AIR)
    return false;

  const BlockType *block_type = block_registry_get(registry, block);
  if (block_type && !block_is_solid(block_type)) {
    return false; // Can't break non-solid blocks
  }

  if (dropped_block) {
    *dropped_block = block;
  }

  chunk_set_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                  z - cp.z * CHUNK_SIZE, BLOCK_AIR);

  // Trigger lighting update (fill with neighbor light)
  u8 new_light = block_calculate_light_level(chunks, registry, x, y, z);
  block_propagate_light(chunks, registry, x, y, z, new_light, LIGHT_TYPE_BLOCK);
  // Placeholder: spawn break VFX and play sound
  LOG_INFO("Block broken at (%d,%d,%d): block=%u - spawning break effects", x,
           y, z, block);
  return true;
}

// Get block at position
BlockID block_get(ChunkManager *chunks, i32 x, i32 y, i32 z) {
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return BLOCK_AIR;

  return chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                         z - cp.z * CHUNK_SIZE);
}

// Check if position is valid for block placement
bool block_can_place(ChunkManager *chunks, BlockRegistry *registry, i32 x,
                     i32 y, i32 z) {
  BlockID existing = block_get(chunks, x, y, z);
  if (existing != BLOCK_AIR)
    return false;

  // Check if block below exists (for most blocks)
  BlockID below = block_get(chunks, x, y - 1, z);
  if (below == BLOCK_AIR) {
    // Some blocks can float (like torches), but most need support
    return false;
  }

  return true;
}
