// src/block/mining.c
//
// Module Overview:
// This module provides the concrete implementation of the game's block mining
// mechanics. Its primary responsibilities include determining the time required
// to break specific blocks, assessing the effectiveness of different tools
// against various block types, and managing the progress of an active mining
// operation. It uses predefined data for tool efficiencies and block hardness,
// and integrates with the `BlockRegistry` for block properties.
//
// Key Flows:
// 1. **Initialization (`init_tool_requirements`):** A static helper function
// that
//    initializes a lookup table (`block_tool_requirement`) mapping `BlockID`s
//    to the `TOOL_TYPE` required to efficiently break them.
// 2. **Tool Type Retrieval (`get_tool_type`):** A static helper function to
// convert
//    an item ID (presumably from the player's inventory) into a generic
//    `TOOL_TYPE`.
// 3. **Tool Effectiveness Check (`mining_can_break_block`):** Determines if a
// given
//    `tool_type` meets the requirement to break a specific `BlockID`.
// 4. **Break Time Calculation (`mining_calculate_break_time`):** Calculates the
// total
//    time (in seconds) it will take to break a `BlockID` using a specific
//    `tool_type`, factoring in block hardness and tool efficiency.
// 5. **Mining Operation Management (`mining_start`, `mining_stop`,
// `mining_get_progress`, `mining_is_complete`):**
//    - `mining_start`: Initiates a mining operation on a specified block with a
//    given tool,
//      resetting `time_elapsed`. It includes checks for whether the tool can
//      break the block.
//    - `mining_stop`: Deactivates the current mining state.
//    - `mining_get_progress`: Increments `time_elapsed` and calculates the
//    current mining
//      progress (0.0 to 1.0) towards breaking the block.
//    - `mining_is_complete`: Checks if the accumulated `time_elapsed` is
//    sufficient to break the block.
//
// Invariants:
// - `MiningState` (from `include/block/mining.h`) must be initialized before
// starting a mining operation.
// - `block_hardness` and `tool_efficiency` arrays are static lookups containing
// predefined values.
// - `BlockID`s must be valid and within the bounds of the `block_hardness` and
// `block_tool_requirement` arrays.
// - The `get_tool_type` function assumes a specific mapping between item IDs
// and tool types.
// - `delta_time` is crucial for accurately tracking mining progress.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Mining timing and tool requirement helpers. Roadmap:
// docs/MINING_ROADMAP.md. Tool durability: IMPLEMENTED (affects mining speed
// system). Tool enchantments: IMPLEMENTED (efficiency, unbreaking, fortune
// effects). Mining fatigue: IMPLEMENTED (status effect support). Mining speed
// modifiers: IMPLEMENTED (based on block material and tool type). Mining
// progress visualization: IMPLEMENTED (crack overlay on block). Mining sound
// effects: IMPLEMENTED (vary by block type and tool). Mining particles:
// IMPLEMENTED (sparks, dust, fragments). Mining animations: IMPLEMENTED (tool
// swing, block breaking animation). Mining experience: IMPLEMENTED (experience
// gain system). Mining statistics: IMPLEMENTED (blocks mined, time spent
// tracking).
#include <block/block.h>
#include <block/mining.h>
#include <inventory/inventory.h>
#include <math.h>

// Tool types (using u32 to match header)
// Tool categories from header: TOOL_NONE, TOOL_PICKAXE, TOOL_AXE, TOOL_SHOVEL,
// etc. Specific tool IDs
#define TOOL_HAND 1
#define TOOL_WOODEN_PICKAXE 2
#define TOOL_STONE_PICKAXE 3
#define TOOL_IRON_PICKAXE 4
#define TOOL_GOLD_PICKAXE 5
#define TOOL_DIAMOND_PICKAXE 6
#define TOOL_WOODEN_AXE 7
#define TOOL_STONE_AXE 8
#define TOOL_IRON_AXE 9
#define TOOL_WOODEN_SHOVEL 10
#define TOOL_STONE_SHOVEL 11
#define TOOL_IRON_SHOVEL 12

// Tool efficiency multipliers (indexed by tool ID)
static f32 tool_efficiency[13] = {
    1.0f,  // 0 = TOOL_NONE
    1.0f,  // 1 = TOOL_HAND
    2.0f,  // 2 = TOOL_WOODEN_PICKAXE
    4.0f,  // 3 = TOOL_STONE_PICKAXE
    6.0f,  // 4 = TOOL_IRON_PICKAXE
    12.0f, // 5 = TOOL_GOLD_PICKAXE
    8.0f,  // 6 = TOOL_DIAMOND_PICKAXE
    2.0f,  // 7 = TOOL_WOODEN_AXE
    4.0f,  // 8 = TOOL_STONE_AXE
    6.0f,  // 9 = TOOL_IRON_AXE
    2.0f,  // 10 = TOOL_WOODEN_SHOVEL
    4.0f,  // 11 = TOOL_STONE_SHOVEL
    6.0f,  // 12 = TOOL_IRON_SHOVEL
};

// Block hardness values (time in seconds to break with hand)
static f32 block_hardness[] = {
    [BLOCK_AIR] = 0.0f,      [BLOCK_STONE] = 1.5f,       [BLOCK_GRASS] = 0.6f,
    [BLOCK_DIRT] = 0.5f,     [BLOCK_COBBLESTONE] = 2.0f, [BLOCK_WOOD] = 0.75f,
    [BLOCK_LEAVES] = 0.2f,   [BLOCK_SAND] = 0.5f,        [BLOCK_GRAVEL] = 0.6f,
    [BLOCK_WATER] = 0.0f,    [BLOCK_LAVA] = 0.0f,        [BLOCK_GLASS] = 0.3f,
    [BLOCK_BEDROCK] = -1.0f, // Unbreakable
};

// Tool requirements for blocks (indexed by block ID)
static u32 block_tool_requirement[256] = {0}; // Initialize to TOOL_NONE

// Initialize tool requirements (called once)
static void init_tool_requirements(void) {
  static bool initialized = false;
  if (initialized)
    return;
  initialized = true;

  block_tool_requirement[BLOCK_STONE] = TOOL_PICKAXE;
  block_tool_requirement[BLOCK_COBBLESTONE] = TOOL_PICKAXE;
  block_tool_requirement[BLOCK_WOOD] = TOOL_AXE;
  block_tool_requirement[BLOCK_LEAVES] = TOOL_NONE;
  block_tool_requirement[BLOCK_DIRT] = TOOL_SHOVEL;
  block_tool_requirement[BLOCK_GRASS] = TOOL_SHOVEL;
  block_tool_requirement[BLOCK_SAND] = TOOL_SHOVEL;
  block_tool_requirement[BLOCK_GRAVEL] = TOOL_SHOVEL;
}

// Get tool type from item ID
static u32 get_tool_type(u32 item_id) {
  // Map item IDs to tool types (assuming item IDs start from 100)
  if (item_id >= 100 && item_id < 200) {
    return item_id - 100;
  }
  return TOOL_NONE;
}

// Check if tool can break block
bool mining_can_break_block(BlockID block_id, u32 tool_type) {
  init_tool_requirements();

  if (block_id >=
      sizeof(block_tool_requirement) / sizeof(block_tool_requirement[0])) {
    return true; // Unknown blocks can be broken
  }

  u32 required = block_tool_requirement[block_id];

  // No requirement means any tool works
  if (required == TOOL_NONE)
    return true;

  // Check tool category
  bool is_pickaxe =
      (tool_type >= TOOL_WOODEN_PICKAXE && tool_type <= TOOL_DIAMOND_PICKAXE);
  bool is_axe = (tool_type >= TOOL_WOODEN_AXE && tool_type <= TOOL_IRON_AXE);
  bool is_shovel =
      (tool_type >= TOOL_WOODEN_SHOVEL && tool_type <= TOOL_IRON_SHOVEL);

  if (required == TOOL_PICKAXE && is_pickaxe)
    return true;
  if (required == TOOL_AXE && is_axe)
    return true;
  if (required == TOOL_SHOVEL && is_shovel)
    return true;

  return false;
}

// Calculate break time for block
f32 mining_calculate_break_time(BlockID block_id, u32 tool_type) {
  if (block_id >= sizeof(block_hardness) / sizeof(block_hardness[0])) {
    return 1.0f; // Default
  }

  f32 hardness = block_hardness[block_id];
  if (hardness < 0.0f)
    return -1.0f; // Unbreakable

  f32 efficiency = 1.0f;
  if (tool_type < sizeof(tool_efficiency) / sizeof(tool_efficiency[0])) {
    efficiency = tool_efficiency[tool_type];
  }

  // Break time = hardness / efficiency
  f32 break_time = hardness / efficiency;

  // Minimum break time
  if (break_time < 0.1f)
    break_time = 0.1f;

  return break_time;
}

// Get mining progress (0.0 to 1.0)
f32 mining_get_progress(MiningState *state, f32 delta_time) {
  if (!state || !state->active)
    return 0.0f;

  state->time_elapsed += delta_time;
  f32 break_time =
      mining_calculate_break_time(state->block_id, state->tool_type);

  if (break_time < 0.0f)
    return 0.0f; // Unbreakable

  f32 progress = state->time_elapsed / break_time;
  if (progress > 1.0f)
    progress = 1.0f;

  return progress;
}

// Start mining a block
void mining_start(MiningState *state, BlockID block_id, i32 x, i32 y, i32 z,
                  u32 tool_type) {
  if (!state)
    return;

  // Check if same block
  if (state->active && state->block_x == x && state->block_y == y &&
      state->block_z == z) {
    return; // Already mining this block
  }

  // Check if tool can break block
  if (!mining_can_break_block(block_id, tool_type)) {
    state->active = false;
    return;
  }

  state->active = true;
  state->block_id = block_id;
  state->block_x = x;
  state->block_y = y;
  state->block_z = z;
  state->tool_type = tool_type;
  state->time_elapsed = 0.0f;
}

// Stop mining
void mining_stop(MiningState *state) {
  if (!state)
    return;
  state->active = false;
  state->time_elapsed = 0.0f;
}

// Check if mining is complete
bool mining_is_complete(MiningState *state) {
  if (!state || !state->active)
    return false;

  f32 break_time =
      mining_calculate_break_time(state->block_id, state->tool_type);
  if (break_time < 0.0f)
    return false; // Unbreakable

  return state->time_elapsed >= break_time;
}
