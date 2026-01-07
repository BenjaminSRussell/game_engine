// src/block/falling_blocks.c
//
// Module Overview:
// This module is responsible for simulating the physical behavior of
// gravity-affected blocks, such as sand and gravel. Its primary function is to
// detect when these blocks lose support from below and to then move them
// downwards, replacing their original position with air. It integrates with the
// `ChunkManager` to access and modify block data within the world and with the
// `BlockRegistry` to determine which blocks are affected by gravity.
//
// Key Flows:
// 1. **Detection and Movement (`block_update_falling`):** This function is
// called
//    for a specific block at `(x, y, z)`.
//    - It first retrieves the block's `BlockType` from the `BlockRegistry` to
//    check
//      if it has the `BLOCK_GRAVITY` flag set.
//    - It then inspects the block immediately below the current block. If the
//    block
//      below is `BLOCK_AIR` or a liquid, the current block is moved to the
//      position below it, and its original position is set to `BLOCK_AIR`.
//    - After modification, the affected `Chunk` is marked as dirty to trigger a
//    mesh update.
//
// Invariants:
// - A valid `ChunkManager` and `BlockRegistry` must be provided.
// - The `physics_world` parameter is currently unused, indicating that this
// module
//   might evolve to integrate more directly with the physics engine (as noted
//   in `include/physics/physics.h`).
// - Block updates are performed directly by modifying chunk data.
// - Chunks affected by falling blocks must be marked dirty to ensure visual
// updates.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Falling block updates when unsupported
// (sand/gravel). Roadmap: docs/FALLING_BLOCKS_ROADMAP.md.
// TODO: Implement falling block entity system for smooth physics-based falling.
// TODO: Add falling block collision detection with other entities.
// TODO: Implement falling block particle effects during fall.
// TODO: Add falling block sound effects for impact and movement.
// TODO: Implement falling block stacking optimization to reduce updates.
// TODO: Add falling block velocity system for realistic acceleration.
// TODO: Implement falling block damage system for entities below.
// TODO: Add falling block conversion system (sand to sandstone on impact).
// TODO: Implement falling block prediction to prevent lag spikes.
// TODO: Add falling block batching for performance optimization.
#include <block/block.h>
#include <chunk/chunk.h>
#include <physics/physics.h>
#include <stdlib.h>

// Update falling blocks (sand, gravel)
void block_update_falling(ChunkManager *chunk_manager,
                          BlockRegistry *block_registry,
                          PhysicsWorld *physics_world, i32 x, i32 y, i32 z) {
  (void)physics_world; // Unused for now
  if (!chunk_manager || !block_registry)
    return;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;

  i32 local_x = x - cp.x * CHUNK_SIZE;
  i32 local_y = y - cp.y * CHUNK_SIZE;
  i32 local_z = z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, local_x, local_y, local_z);

  // Check if block has gravity
  const BlockType *block_type = block_registry_get(block_registry, block);
  if (!block_type || !block_has_gravity(block_type)) {
    return;
  }

  // Check block below
  BlockID below = chunk_get_block(chunk, local_x, local_y - 1, local_z);
  const BlockType *below_type = block_registry_get(block_registry, below);

  // If air or liquid below, fall
  if (below == BLOCK_AIR || (below_type && block_is_liquid(below_type))) {
    // Move block down
    chunk_set_block(chunk, local_x, local_y - 1, local_z, block);
    chunk_set_block(chunk, local_x, local_y, local_z, BLOCK_AIR);

    // Mark chunk for mesh update
    chunk_mark_mesh_dirty(chunk);
  }
}
