// include/tech/crafting.h
//
// Purpose: Defines a comprehensive technological crafting system, distinct from
// basic crafting, by introducing technology tiers and specialized workbenches.
// This header provides structures for `ItemType`s, `TechTier`s, and
// `WorkbenchType`s, along with `Recipe` definitions that include technology
// requirements and crafting times. It also manages a `RecipeRegistry` for
// technological recipes and a `TechnologyTree` for tracking player progression
// through different tech levels and unlocked recipes.
//
// Public APIs:
// - `ItemType`: Generic categorization of items.
// - `TechTier`: Enumeration defining various technological advancement levels
// (e.g., Stone, Electric).
// - `RecipeIngredient`: Structure for item and quantity requirements for a
// recipe.
// - `Recipe`: Defines a crafting recipe, specifying its result, ingredients,
// required `TechTier`,
//   `crafting_time_ms`, and the `workbench_type` needed.
// - `RecipeRegistry`: Structure for managing a collection of technological
// `Recipe`s.
// - `recipe_registry_init`, `recipe_registry_free`: Lifecycle for the
// technological recipe registry.
// - `recipe_register`: Adds a new technological recipe to the registry.
// - `recipe_find_by_result`, `recipe_find_by_ingredients`: Functions to search
// for recipes.
// - `TechnologyTree`: Structure tracking the player's current `TechTier`,
// unlocked tiers,
//   research points, and a list of `unlocked_recipes`.
// - `tech_tree_init`, `tech_tree_free`: Lifecycle for the technology tree.
// - `tech_unlock_tier`, `tech_unlock_recipe`: Functions to advance the player's
// technological capabilities.
// - `tech_can_craft`: Checks if a recipe can be crafted given the player's
// `TechnologyTree`.
// - `WorkbenchType`: Enumeration defining types of specialized crafting
// stations.
// - `Workbench`: Structure representing a specialized crafting station,
// including its type,
//   required `TechTier`, current `crafting_progress`, and input/output item
//   management.
// - `workbench_init`, `workbench_free`: Lifecycle for a `Workbench`.
// - `workbench_start_crafting`, `workbench_update`, `workbench_is_complete`,
// `workbench_collect_output`:
//   Functions for managing the crafting process on a workbench.
//
// Ownership: `RecipeRegistry` and `TechnologyTree` instances own their internal
// data structures. A `Workbench` instance owns its internal state related to
// crafting.
//
// Invariants:
// - All registries and trees must be initialized before use.
// - Recipes must be registered with their corresponding `TechTier` and
// `WorkbenchType`.
// - `crafting_time_ms` is in milliseconds.
// - Player's `TechnologyTree` must be updated to reflect unlocked tiers for
// crafting.
//
#ifndef CRAFTING_H
#define CRAFTING_H

#include "../game_common.h"

// Item types are defined in inventory.h

// Technology tier
typedef enum {
  TECH_TIER_STONE,
  TECH_TIER_COPPER,
  TECH_TIER_IRON,
  TECH_TIER_STEEL,
  TECH_TIER_ELECTRIC,
  TECH_TIER_ADVANCED,
  TECH_TIER_COUNT
} TechTier;

// Recipe ingredient
typedef struct {
  u32 item_id;
  u32 quantity;
} TechRecipeIngredient;

// Crafting recipe
typedef struct {
  u32 result_item_id;
  u32 result_quantity;
  TechRecipeIngredient *ingredients;
  u32 ingredient_count;
  TechTier required_tier;
  u32 crafting_time_ms;
  const char *workbench_type; // "crafting_table", "furnace", "anvil", etc.
} TechRecipe;

// Recipe registry
typedef struct {
  TechRecipe *recipes;
  u32 count;
  u32 capacity;
} TechRecipeRegistry;

// Initialize recipe system
void tech_recipe_registry_init(TechRecipeRegistry *registry, u32 capacity);
void tech_recipe_registry_free(TechRecipeRegistry *registry);

// Register recipe
void tech_recipe_register(TechRecipeRegistry *registry, TechRecipe recipe);

// Find recipes
TechRecipe *tech_recipe_find_by_result(TechRecipeRegistry *registry,
                                       u32 item_id);
TechRecipe *tech_recipe_find_by_ingredients(TechRecipeRegistry *registry,
                                            u32 *ingredient_ids,
                                            u32 ingredient_count);

// Technology progression
typedef struct {
  TechTier current_tier;
  bool tiers_unlocked[TECH_TIER_COUNT];
  u32 research_points;
  u32 *unlocked_recipes;
  u32 unlocked_recipe_count;
} TechnologyTree;

// Initialize technology tree
void tech_tree_init(TechnologyTree *tree);
void tech_tree_free(TechnologyTree *tree);

// Technology progression
bool tech_unlock_tier(TechnologyTree *tree, TechTier tier);
bool tech_unlock_recipe(TechnologyTree *tree, u32 recipe_id);
bool tech_can_craft(TechnologyTree *tree, TechRecipe *recipe);

// Workbench types
typedef enum {
  TECH_WORKBENCH_CRAFTING_TABLE,
  TECH_WORKBENCH_FURNACE,
  TECH_WORKBENCH_ANVIL,
  TECH_WORKBENCH_SAWMILL,
  TECH_WORKBENCH_FORGE,
  TECH_WORKBENCH_ASSEMBLER,
  TECH_WORKBENCH_COUNT
} TechWorkbenchType;

// Workbench
typedef struct {
  TechWorkbenchType type;
  TechTier tier;
  TechRecipe *current_recipe;
  f32 crafting_progress;
  bool is_crafting;
  u32 *input_items;
  u32 input_count;
  u32 *output_items;
  u32 output_count;
} TechWorkbench;

// Initialize workbench
void workbench_init(TechWorkbench *bench, TechWorkbenchType type,
                    TechTier tier);
void workbench_free(TechWorkbench *bench);

// Workbench operations
bool workbench_start_crafting(TechWorkbench *bench, TechRecipe *recipe,
                              u32 *ingredients, u32 ingredient_count);
void workbench_update(TechWorkbench *bench, f32 delta_time);
bool workbench_is_complete(TechWorkbench *bench);
void workbench_collect_output(TechWorkbench *bench, u32 *output,
                              u32 *output_count);

#endif // CRAFTING_H
