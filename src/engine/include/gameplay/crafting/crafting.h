// include/gameplay/crafting/crafting.h
//
// Purpose: Crafting system for combining items to create new items. Supports
// shaped recipes (grid pattern), shapeless recipes (any order), and furnace
// smelting.
//
#ifndef CRAFTING_H
#define CRAFTING_H

#include "gameplay/inventory/inventory.h"
#include "gameplay/inventory/item.h"
#include <common.h>

// Recipe types
typedef enum {
  RECIPE_TYPE_SHAPED,    // Grid pattern (2x2 or 3x3)
  RECIPE_TYPE_SHAPELESS, // Any order
  RECIPE_TYPE_SMELTING,  // Furnace/smelting
  RECIPE_TYPE_COUNT
} RecipeType;

// Crafting grid sizes
typedef enum {
  GRID_SIZE_2X2 = 2,
  GRID_SIZE_3X3 = 3,
} GridSize;

// Recipe ingredient (can be specific item or category)
typedef struct {
  u32 item_id;       // Specific item (0 = any from category)
  ItemType category; // Item category if item_id is 0
  u32 quantity;      // Required quantity
} RecipeIngredient;

// Shaped recipe (grid pattern)
typedef struct {
  RecipeIngredient grid[9]; // Max 3x3 grid (3x3=9, 2x2 uses first 4)
  u8 width;                 // Grid width (2 or 3)
  u8 height;                // Grid height (2 or 3)
  bool mirror;              // Can be flipped horizontally
} ShapedRecipe;

// Shapeless recipe (any order)
typedef struct {
  RecipeIngredient *ingredients;
  u32 ingredient_count;
} ShapelessRecipe;

// Smelting recipe
typedef struct {
  u32 input_item_id;
  f32 cook_time;     // Seconds to smelt
  u32 fuel_required; // Fuel units needed
} SmeltingRecipe;

// Recipe definition
typedef struct {
  u32 id;           // Unique recipe ID
  const char *name; // Recipe name
  RecipeType type;

  // Output
  ItemStack output;

  // Recipe data
  union {
    ShapedRecipe shaped;
    ShapelessRecipe shapeless;
    SmeltingRecipe smelting;
  } data;

  // Requirements
  u32 required_level; // Crafting level required
  bool discovered;    // Has player discovered this recipe?
} Recipe;

// Crafting result
typedef struct {
  bool success;
  ItemStack output;
  const char *error_message;
} CraftingResult;

// ============================================================================
// RECIPE DATABASE
// ============================================================================

// Initialize crafting system
bool crafting_system_init(u32 max_recipes);
void crafting_system_shutdown(void);

// Register recipes
bool crafting_register_recipe(const Recipe *recipe);
bool crafting_load_recipes_from_json(const char *filepath);

// Get recipe
const Recipe *crafting_get_recipe(u32 recipe_id);
u32 crafting_get_recipe_count(void);

// ============================================================================
// CRAFTING OPERATIONS
// ============================================================================

// Try to craft from grid
CraftingResult crafting_try_craft_shaped(const ItemStack *grid, u8 grid_size);
CraftingResult crafting_try_craft_shapeless(const ItemStack *items,
                                            u32 item_count);

// Find matching recipe
const Recipe *crafting_find_shaped_recipe(const ItemStack *grid, u8 grid_size);
const Recipe *crafting_find_shapeless_recipe(const ItemStack *items,
                                             u32 item_count);

// Validate crafting
bool crafting_can_craft(const Container *inventory, const Recipe *recipe);
bool crafting_has_ingredients(const Container *inventory, const Recipe *recipe);

// Perform craft (consumes items)
bool crafting_craft(Container *inventory, const Recipe *recipe,
                    ItemStack *out_result);

// ============================================================================
// CRAFTING STATION
// ============================================================================

// Crafting station component
typedef struct {
  ItemStack grid[9]; // Crafting grid (3x3)
  u8 grid_size;      // Active grid size (2 or 3)
  ItemStack result;  // Preview of result
  const Recipe *active_recipe;
} CraftingStation;

// Create crafting station
CraftingStation crafting_station_create(u8 grid_size);

// Update result preview
void crafting_station_update(CraftingStation *station);

// Execute craft
bool crafting_station_craft(CraftingStation *station, Container *inventory);

// ============================================================================
// SMELTING/FURNACE
// ============================================================================

typedef struct {
  ItemStack input;
  ItemStack fuel;
  ItemStack output;
  f32 progress;       // 0.0 to 1.0
  f32 fuel_remaining; // Time left on current fuel
  const Recipe *active_recipe;
} Furnace;

// Furnace operations
Furnace furnace_create(void);
// void furnace_update(Furnace *furnace, f32 delta_time); // Conflicts with furnace.h - commented out
bool furnace_add_input(Furnace *furnace, ItemStack item);
bool furnace_add_fuel(Furnace *furnace, ItemStack item);
ItemStack furnace_take_output(Furnace *furnace);

// ============================================================================
// DEFAULT RECIPES
// ============================================================================

void crafting_register_default_recipes(void);

#endif // CRAFTING_H
