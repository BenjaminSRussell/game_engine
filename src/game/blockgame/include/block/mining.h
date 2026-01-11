// include/block/mining.h
//
// Purpose: Defines the API and data structures related to the block mining mechanics
// in the game. This includes defining various tool types, calculating the time required
// to break different blocks, and managing the state of an ongoing mining operation.
//
// Public APIs:
// - `TOOL_*` macros: Defines constants for different tool types (e.g., pickaxe, axe).
// - `MiningState`: Structure to hold the current state of a block being mined,
//   including the block's ID and position, the tool being used, and elapsed time.
// - `mining_can_break_block`: Checks if a given tool can effectively break a specific block type.
// - `mining_calculate_break_time`: Determines the total time (in seconds) needed to break a block
//   with a particular tool.
// - `mining_get_progress`: Calculates the current mining progress (0.0 to 1.0) based on elapsed time.
// - `mining_start`, `mining_stop`: Functions to initiate and terminate a mining operation.
// - `mining_is_complete`: Checks if the mining of the current block is finished.
//
// Ownership: The `MiningState` structure is typically owned by the player or an entity
// performing the mining action. The functions operate on this state and block properties.
//
// Invariants:
// - `block_id` and `tool_type` passed to functions must be valid and recognized within the game system.
// - `MiningState` must be properly initialized before use, particularly before calling `mining_start`.
// - `delta_time` should represent the time passed since the last update for accurate progress calculation.
//
#ifndef MINING_H
#define MINING_H


#include "../game_common.h"
#include "../block/block.h"

// Tool categories
#define TOOL_NONE 0
#define TOOL_PICKAXE 1
#define TOOL_AXE 2
#define TOOL_SHOVEL 3
#define TOOL_HOE 4
#define TOOL_SWORD 5

// Mining state
typedef struct {
    bool active;
    BlockID block_id;
    i32 block_x, block_y, block_z;
    u32 tool_type; // ToolType enum value
    f32 time_elapsed;
} MiningState;

// Check if tool can break block
bool mining_can_break_block(BlockID block_id, u32 tool_type);

// Calculate break time for block (in seconds)
f32 mining_calculate_break_time(BlockID block_id, u32 tool_type);

// Get mining progress (0.0 to 1.0)
f32 mining_get_progress(MiningState *state, f32 delta_time);

// Start mining a block
void mining_start(MiningState *state, BlockID block_id, i32 x, i32 y, i32 z, u32 tool_type);

// Stop mining
void mining_stop(MiningState *state);

// Check if mining is complete
bool mining_is_complete(MiningState *state);

#endif // MINING_H

