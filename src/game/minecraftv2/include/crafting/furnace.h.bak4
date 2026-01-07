// include/crafting/furnace.h
//
// Purpose: Defines the API and data structures for managing a furnace, a processing block
// in the game that smelts or cooks items using fuel. This header outlines the structure
// of the `FurnaceState`, detailing its slots for input, fuel, and output, as well as
// variables for tracking smelting progress and fuel consumption.
//
// Public APIs:
// - `FurnaceState`: Structure representing the current state of a furnace, including
//   its active status, the items in its input, fuel, and output slots, current recipe,
//   smelting progress, and fuel remaining.
// - `furnace_init`: Initializes a `FurnaceState` instance.
// - `furnace_update`: Updates the furnace state each frame, handling fuel consumption,
//   smelting progress, and item transfer to the output slot when complete.
// - `furnace_get_progress`: Returns the current smelting progress as a value between 0.0 and 1.0.
// - `furnace_is_active`: Checks if the furnace is currently operating (smelting an item).
// - `furnace_get_burn_time`: Returns the remaining burn time for the current fuel.
//
// Ownership: A `FurnaceState` instance manages its internal state variables. It interacts
// with an `Inventory` instance (typically a block inventory) but does not own it.
//
// Invariants:
// - A `FurnaceState` must be initialized with `furnace_init` before use.
// - `furnace_update` should be called regularly (e.g., every game tick) to ensure proper
//   smelting progression and fuel management.
// - The `Inventory` structure (defined in `inventory.h`) is assumed to be correctly defined.
// - Fuel and smeltable items are expected to be recognized by the underlying crafting/recipe system.
//
#ifndef FURNACE_H
#define FURNACE_H


#include "../game_common.h"
#include "../inventory/inventory.h"

// Furnace state
typedef struct {
    bool active;
    u32 input_slot;
    u32 fuel_slot;
    u32 output_slot;
    u32 current_recipe;
    f32 smelt_progress;
    f32 fuel_remaining;
    f32 burn_time;
    f32 xp_stored; // Experience accumulated from smelting operations
} FurnaceState;

// Initialize furnace
// TODO: Set default values for burn time and progress.
void furnace_init(FurnaceState *furnace);

// Update furnace (call every frame)
// TODO: Implement the main furnace logic loop:
// 1. Decrease burn_time if > 0.
// 2. If can_smelt() and burn_time == 0, consume fuel (reset burn_time).
// 3. If burning and can_smelt(), increase smelt_progress.
// 4. If smelt_progress >= 1.0, produce output, add XP to xp_stored, and reset progress.
void furnace_update(FurnaceState *furnace, Inventory *inventory, f32 delta_time);

// Get smelting progress (0.0 to 1.0)
f32 furnace_get_progress(FurnaceState *furnace);

// Check if furnace is active
bool furnace_is_active(FurnaceState *furnace);

// Get burn time remaining
f32 furnace_get_burn_time(FurnaceState *furnace);

// Extract stored experience
// TODO: Call this when player takes items from the output slot.
f32 furnace_take_xp(FurnaceState *furnace);

// Helpers for UI and automation
bool furnace_is_fuel(u32 item_id);
bool furnace_is_smeltable(u32 item_id);

#endif // FURNACE_H
