// src/block/liquid_blocks.c
//
// Module Overview:
// This module implements a basic fluid simulation system for liquid blocks,
// specifically water and lava. Its core responsibility is to simulate the
// natural flow of these liquids, enabling them to spread from source blocks
// downwards due to gravity and horizontally into adjacent empty spaces.
// The module directly interacts with the `ChunkManager` to access the current
// block state of the world and to apply changes as liquids flow.
// TODO: Implement realistic fluid pressure system for accurate water flow.
// TODO: Add fluid volume conservation to prevent water duplication.
// TODO: Implement fluid viscosity system for different liquid types.
// TODO: Add fluid temperature system affecting flow rate and behavior.
// TODO: Implement fluid mixing system for different liquid combinations.
// TODO: Add fluid evaporation system for water in hot biomes.
// TODO: Implement fluid freezing system for water in cold biomes.
// TODO: Add fluid source block detection and management.
// TODO: Implement fluid flow optimization with update batching.
// TODO: Add fluid visual effects for flowing animations.
//
// Key Flows:
// 1. **Water Flow Update (`block_update_water_flow`):**
//    - Takes world coordinates `(x, y, z)` of a water block.
//    - Checks the block below: if it's `BLOCK_AIR`, the water block will flow
//    downwards,
//      setting the block below to `BLOCK_WATER`.
//    - Checks adjacent horizontal blocks: if any are `BLOCK_AIR`, the water
//    block
//      will spread horizontally into those air blocks.
//    - Placeholder `LOG_INFO` calls indicate where particle effects or sound
//    effects
//      would typically be triggered.
// 2. **Lava Flow Update (`block_update_lava_flow`):**
//    - Currently, this function is a placeholder that simply calls
//    `block_update_water_flow`.
//    - In a complete implementation, this would contain distinct logic for
//    lava's
//      slower flow rate and other unique properties (e.g., burning blocks).
//
// Invariants:
// - A valid `ChunkManager` must be provided for all liquid flow operations.
// - World coordinates (`x`, `y`, `z`) must be integer-based.
// - Flow logic assumes `BLOCK_AIR` as the primary target for liquid spread.
// - The fluid simulation is currently a simple, iterative spread without
// advanced
//   properties like fluid pressure, accurate volume conservation, or complex
//   interaction with other block types (e.g., flowing over non-solid blocks).
// - Functions `block_update_water_flow` and `block_update_lava_flow` are
// declared
//   in `include/block/block_states.h`, indicating they are part of the block
//   state update pipeline.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Water/lava state updates and simple fluid flow.
// Roadmap: docs/LIQUID_BLOCKS_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math/vec3.h>
#include <stdlib.h>

// Liquid block state
typedef struct {
  i32 x, y, z;
  u8 level; // 0-15, liquid level
  bool is_source;
} LiquidBlock;

// Update water flow
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  if (!chunk_manager)
    return;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;

  BlockID block = chunk_get_block(chunk, x - cp.x * CHUNK_SIZE,
                                  y - cp.y * CHUNK_SIZE, z - cp.z * CHUNK_SIZE);

  if (block != BLOCK_WATER)
    return;

  // Check neighbors for flow
  BlockID below =
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, (y - 1) - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE);

  // Flow down if air below
  if (below == BLOCK_AIR) {
    /* Only flow down if the location below does not already have many water
       neighbors. This reduces simple duplication when multiple sources try to
       write the same block in the same tick. */
    int below_x = x - cp.x * CHUNK_SIZE;
    int below_y = (y - 1) - cp.y * CHUNK_SIZE;
    int below_z = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    /* Count adjacent water blocks around the target down cell */
    if (chunk_get_block(chunk, below_x + 1, below_y, below_z) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x - 1, below_y, below_z) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x, below_y, below_z + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, below_x, below_y, below_z - 1) == BLOCK_WATER)
      adj++;

    if (adj < 2) {
      chunk_set_block(chunk, below_x, below_y, below_z, BLOCK_WATER);
      // Placeholder: spawn water drip particles and play water flow sound
      LOG_INFO("Water flowed down at (%d,%d,%d)", x, y - 1, z);
    } else {
      LOG_INFO(
          "Skipped flowing down to (%d,%d,%d) due to existing water neighbors",
          x, y - 1, z);
    }
  }

  // Flow horizontally

  BlockID neighbors[4] = {
      chunk_get_block(chunk, (x + 1) - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, (x - 1) - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      z - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      (z + 1) - cp.z * CHUNK_SIZE),
      chunk_get_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                      (z - 1) - cp.z * CHUNK_SIZE)};

  // Simple horizontal spread into adjacent air blocks.
  if (neighbors[0] == BLOCK_AIR) {
    int tx = (x + 1) - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x + 1, y, z);
    }
  }
  if (neighbors[1] == BLOCK_AIR) {
    int tx = (x - 1) - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = z - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x - 1, y, z);
    }
  }
  if (neighbors[2] == BLOCK_AIR) {
    int tx = x - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = (z + 1) - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x, y, z + 1);
    }
  }
  if (neighbors[3] == BLOCK_AIR) {
    int tx = x - cp.x * CHUNK_SIZE;
    int ty = y - cp.y * CHUNK_SIZE;
    int tz = (z - 1) - cp.z * CHUNK_SIZE;
    int adj = 0;
    if (chunk_get_block(chunk, tx + 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx - 1, ty, tz) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz + 1) == BLOCK_WATER)
      adj++;
    if (chunk_get_block(chunk, tx, ty, tz - 1) == BLOCK_WATER)
      adj++;
    if (adj < 2) {
      chunk_set_block(chunk, tx, ty, tz, BLOCK_WATER);
      // Placeholder: spawn water splash/puddle particles and play gentle flow
      // sound
      LOG_INFO("Water spread to (%d,%d,%d)", x, y, z - 1);
    }
  }
}

// Update lava flow (similar to water but slower)
void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  if (!chunk_manager)
    return;

  // Similar to water but with different flow rate
  block_update_water_flow(chunk_manager, x, y, z);
}
