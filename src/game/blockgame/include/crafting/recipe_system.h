// include/crafting/recipe_system.h
//
// Purpose: Defines the unified crafting and recipe system for the game.
// This file consolidates previous logic from recipe.h, recipe_registry.h,
// recipe_manager.h, and advanced_crafting.h into a single cohesive API.
//
// Public APIs:
// - `RecipeType`, `WorkbenchType`: Enumerations for crafting categories.
// - `Recipe`: Unified structure for shaped, shapeless, and machine recipes.
// - `RecipeRegistry`: Central storage for all game recipes.
// - Registry management: `init`, `free`, `add`, `get`.
// - Matching logic: `recipe_find_match` (grid-based), `recipe_can_craft`.
// - Execution: `recipe_craft` (transactional).
//
// Ownership:
// - `RecipeRegistry` owns the `Recipe` data.
//
// Invariants:
// - `RecipeRegistry` must be initialized before use.
//
// Invariants:
// - The `RecipeRegistry` must be initialized before adding or retrieving
// recipes.
// - `MAX_RECIPES`, `MAX_RECIPE_INGREDIENTS`, `MAX_RECIPE_OUTPUT` define static
// capacities.
// - `item_id` values for ingredients and outputs must correspond to valid game
// items.
// - The `Inventory` structure (from `inventory.h`) is assumed to be correctly
// defined.
//
#ifndef RECIPE_SYSTEM_H
#define RECIPE_SYSTEM_H

#include "../game_common.h"
#include "../inventory/inventory.h"

#define MAX_RECIPES 2048
#define MAX_RECIPE_INGREDIENTS 9
#define MAX_RECIPE_OUTPUT 4
#define INVALID_RECIPE_ID 0xFFFFFFFF

// Workbench types (consolidated from advanced_crafting.h)
typedef enum {
  WORKBENCH_NONE = 0,
  WORKBENCH_CRAFTING_TABLE,
  WORKBENCH_FURNACE,
  WORKBENCH_ANVIL,
  WORKBENCH_ENCHANTING_TABLE,
  WORKBENCH_BREWING_STAND,
  WORKBENCH_SMITHING_TABLE,
  WORKBENCH_STONECUTTER,
  WORKBENCH_COUNT
} WorkbenchType;

typedef enum {
  RECIPE_TYPE_SHAPELESS,
  RECIPE_TYPE_SHAPED,
  RECIPE_TYPE_FURNACE,
  RECIPE_TYPE_BREWING,
  RECIPE_TYPE_SMITHING,
  RECIPE_TYPE_STONECUTTING
} RecipeType;

typedef struct {
  u32 item_id;
  u16 quantity;
  // TODO: Add support for item tags (e.g. "logs", "stone_tool") to allow
  // generic ingredients. bool is_tag; char tag_name[32];
} RecipeIngredient;

typedef struct {
  WorkbenchType type;
  u32 block_id;
  char name[32];
  u32 recipe_slots;
  bool supports_shaped;
  bool supports_shapeless;
} Workbench;

typedef struct {
  u32 item_id;
  u16 quantity;
} RecipeOutput;

typedef struct {
  u32 id;
  RecipeType type;
  WorkbenchType required_workbench;

  char group[32]; // UI grouping (e.g. "bed", "boat") for the recipe book
  // Pattern dimensions for shaped recipes (e.g., 3x3, 2x2)
  u8 width;
  u8 height;

  // Ingredients (max 9 for 3x3 grid)
  RecipeIngredient ingredients[MAX_RECIPE_INGREDIENTS];
  u32 ingredient_count;

  RecipeOutput outputs[MAX_RECIPE_OUTPUT];
  u32 output_count;

  f32 craft_time; // For machines
  bool unlocked;

  // Legacy fields for compatibility with recipe.c
  u32 pattern_width;
  u32 pattern_height;
  u32 *pattern; // Dynamically allocated pattern array
  u32 result_item;
  u32 result_quantity;
} Recipe;

typedef struct {
  Recipe *recipes;
  u32 recipe_capacity;
  u32 recipe_count;
  Workbench *workbenches;
  u32 workbench_count;
} AdvancedCraftingSystem;

#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

typedef struct {
  Recipe recipes[MAX_RECIPES];
  u32 recipe_count;
  // TODO: Add a hash map or lookup table for fast recipe retrieval by output
  // ID.
  // TODO: Add a lookup tree (Trie) for efficient shaped recipe pattern
  // matching.

#ifndef PLATFORM_WEB
  pthread_mutex_t mutex;
#endif
} RecipeRegistry;

// --- Registry Management ---

// Initialize the global recipe registry
void recipe_system_init(void);
void recipe_registry_init(RecipeRegistry *registry);

// Free resources
void recipe_registry_free(RecipeRegistry *registry);

// Add a recipe. Returns the recipe ID.
u32 recipe_registry_add(RecipeRegistry *registry, Recipe *recipe);

// Get a recipe by ID
Recipe *recipe_registry_get(RecipeRegistry *registry, u32 recipe_id);

// Find a recipe that produces a specific item ID
Recipe *recipe_registry_find_by_result(RecipeRegistry *registry, u32 item_id);

// Populate with defaults
void recipe_registry_add_defaults(RecipeRegistry *registry);

// Load recipes from data directory
// TODO: Parse JSON files from assets/data/recipes/ to populate registry.
void recipe_registry_load_from_disk(RecipeRegistry *registry, const char *path);

// --- Matching & Crafting ---

// Find a matching recipe for a given grid/inventory
// 'items' is an array representing the crafting grid (e.g. 9 items for 3x3).
// Returns NULL if no match found.
Recipe *recipe_find_match(RecipeRegistry *registry, RecipeType type,
                          InventorySlot *grid_items, u32 grid_width,
                          u32 grid_height);

// Check if inventory has ingredients for a specific recipe (shapeless check
// usually)
bool recipe_can_craft(Recipe *recipe, Inventory *inventory);

// Consume ingredients and produce output
bool recipe_craft(Recipe *recipe, Inventory *inventory);

// Get remaining items after crafting (e.g. buckets from milk, bottles from
// honey) Populates 'remainder' array which must be same size as input grid.
// Returns true if any remainder items were generated.
bool recipe_get_remainder(Recipe *recipe, InventorySlot *grid_items,
                          u32 grid_size, InventorySlot *out_remainder);

// --- Recipe Book / Unlocking ---

// TODO: Implement persistent storage for unlocked recipes per player.
bool recipe_is_unlocked(RecipeRegistry *registry, u32 recipe_id);

#endif
