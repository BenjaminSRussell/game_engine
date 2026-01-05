// include/crafting/crafting_table.h
//
// Purpose: Defines the API and data structures for interacting with a crafting
// table, a dedicated station for crafting items from a 3x3 grid of ingredients.
// This header outlines the structure of the `CraftingTable` itself, including
// its input and output inventories, and declares functions for its lifecycle
// management, recipe checking, and item retrieval.
//
// Public APIs:
// - `CraftingTable`: Structure representing a crafting table, encompassing a
// reference
//   to the `RecipeRegistry`, its 3x3 `input_inventory` (crafting grid), a
//   single `output_inventory` slot for the crafted item, and a flag indicating
//   if it's currently in use (`is_open`).
// - `crafting_table_init`: Initializes a `CraftingTable` instance, linking it
// to a `RecipeRegistry`.
// - `crafting_table_free`: Frees resources associated with the `CraftingTable`.
// - `crafting_table_update`: Periodically checks the `input_inventory` against
// registered recipes
//   to determine if an item can be crafted and updates the `output_inventory`.
// - `crafting_table_take_output`: Transfers the crafted item from the
// `output_inventory`
//   to the player's inventory, if successful.
//
// Ownership: A `CraftingTable` instance owns its `input_inventory` and
// `output_inventory`. It holds a reference to a `RecipeRegistry` but does not
// own it.
//
// Invariants:
// - A `CraftingTable` must be initialized with `crafting_table_init` before use
// and freed.
// - The `RecipeRegistry` and `Inventory` structures (defined in their
// respective headers)
//   are assumed to be correctly defined and initialized.
// - `crafting_table_update` should be called regularly to process crafting
// attempts.
// - `player_inventory` passed to `crafting_table_take_output` must be a valid,
//   initialized inventory belonging to the player.
//
#ifndef CRAFTING_TABLE_H
#define CRAFTING_TABLE_H

#include "../game_common.h"
#include "../inventory/inventory.h"
#include "recipe_system.h"
#include <math/vec3.h>

// Forward declarations (types provided via included headers)

// Crafting table structure
typedef struct {
  RecipeRegistry *registry;
  Inventory input_inventory;  // 3x3 crafting grid
  Inventory output_inventory; // Output slot
  bool is_open;
} CraftingTable;

// Initialize/free crafting table
void crafting_table_init(CraftingTable *table, RecipeRegistry *registry);
void crafting_table_free(CraftingTable *table);

// Update crafting table (check recipes)
// TODO: Scan the 3x3 grid and construct a pattern/ingredient list.
// TODO: Query RecipeRegistry for a match.
// TODO: Update output_inventory with result or clear it.
void crafting_table_update(CraftingTable *table);

// Take output
// TODO: If output taken, decrement ingredients in input_inventory.
// TODO: Handle item container returns (e.g., bucket from milk).
bool crafting_table_take_output(CraftingTable *table,
                                Inventory *player_inventory);

// Drop contents into the world
// TODO: Call this when the block is broken to prevent item loss.
void crafting_table_drop_contents(CraftingTable *table, Vec3 position);

#endif // CRAFTING_TABLE_H
