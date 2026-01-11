// src/block/block_states.c
// TODO: Implement block state transition system with state machines.
// TODO: Add block state persistence across chunk unloads.
// TODO: Implement block state validation to prevent invalid states.
// TODO: Add block state compression for memory efficiency.
// TODO: Implement block state serialization for save/load.
// TODO: Add block state change notifications for other systems.
// TODO: Implement block state caching to reduce lookups.
// TODO: Add block state versioning for compatibility.
// TODO: Implement block state migration system for updates.
// TODO: Add block state statistics and profiling.
#include <block/block.h>
#include <block/block_states.h>
#include <block/water_physics.h>
#include <chunk/chunk.h>
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

// Initialize block state manager
void block_state_manager_init(BlockStateManager *manager, u32 capacity) {
  if (!manager)
    return;
  manager->states = (BlockState *)calloc(capacity, sizeof(BlockState));
  manager->count = 0;
  manager->capacity = capacity;
}

// Free block state manager
void block_state_manager_free(BlockStateManager *manager) {
  if (!manager)
    return;
  if (manager->states) {
    free(manager->states);
    manager->states = NULL;
  }
  manager->count = 0;
  manager->capacity = 0;
}

void block_state_manager_add(BlockStateManager *manager, i32 x, i32 y, i32 z,
                             BlockID block_id) {
  if (!manager || !manager->states)
    return;

  // Check if limits reached
  if (manager->count >= manager->capacity) {
    // Simple drop policy for now
    return;
  }

  // Check availability (linear scan optimization pending)
  for (u32 i = 0; i < manager->count; i++) {
    if (manager->states[i].x == x && manager->states[i].y == y &&
        manager->states[i].z == z) {
      manager->states[i].needs_update = true;
      manager->states[i].block_id = block_id;
      return;
    }
  }

  BlockState *state = &manager->states[manager->count++];
  state->x = x;
  state->y = y;
  state->z = z;
  state->block_id = block_id;
  state->needs_update = true;
}

// Update block states (for water flow, etc.)
void block_state_manager_update(BlockStateManager *manager,
                                ChunkManager *chunk_manager, f32 delta_time) {
  if (!manager || !chunk_manager)
    return;

  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    if (!state->needs_update)
      continue;

    // Dispatch based on block type
    if (state->block_id == BLOCK_WATER) {
      bool changed =
          water_update_tick(chunk_manager, state->x, state->y, state->z);
      if (!changed) {
        // If stabilized, stop updating
        state->needs_update = false;
      }
    } else {
      state->needs_update = false;
    }
  }
  (void)delta_time;
}

// Legacy placeholders implementation
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  water_update_tick(chunk_manager, x, y, z);
}

void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  (void)chunk_manager;
  (void)x;
  (void)y;
  (void)z;
}
