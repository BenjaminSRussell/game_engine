#include "include/gameplay/crafting/crafting.h"
#include "include/gameplay/inventory/item_database.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// Global recipe storage
static Recipe *g_recipes = NULL;
static u32 g_recipe_count = 0;
static u32 g_recipe_capacity = 0;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static bool matches_ingredient(const RecipeIngredient *ingredient, const ItemStack *item) {
  if (ingredient->item_id != 0) {
    return item->item_id == ingredient->item_id && item->quantity >= ingredient->quantity;
  }
  // TODO: Check category if item_id is 0
  return false;
}

static bool check_shaped_grid(const ShapedRecipe *recipe, const ItemStack *grid, u8 grid_size, u32 start_x, u32 start_y) {
  for (u32 y = 0; y < recipe->height; y++) {
    for (u32 x = 0; x < recipe->width; x++) {
      u32 grid_idx = (start_y + y) * grid_size + (start_x + x);
      u32 recipe_idx = y * recipe->width + x;

      const RecipeIngredient *ing = &recipe->grid[recipe_idx];
      const ItemStack *slot = &grid[grid_idx];

      if (ing->item_id == 0) {
        if (slot->item_id != 0) return false;
      } else {
        if (!matches_ingredient(ing, slot)) return false;
      }
    }
  }
  return true;
}

// ============================================================================
// RECIPE DATABASE
// ============================================================================

bool crafting_system_init(u32 max_recipes) {
  if (g_recipes) return true;

  g_recipe_capacity = max_recipes > 0 ? max_recipes : 1024;
  g_recipes = calloc(g_recipe_capacity, sizeof(Recipe));

  if (!g_recipes) {
    LOG_ERROR("Failed to allocate crafting recipes");
    return false;
  }

  g_recipe_count = 0;
  LOG_INFO("Crafting system initialized (capacity: %u)", g_recipe_capacity);
  return true;
}

void crafting_system_shutdown(void) {
  if (g_recipes) {
    // Need to free dynamic arrays in shapeless recipes if we allocated them deep
    // But currently Recipe struct assumes flat allocation or external management for shapeless ingredients array?
    // The header has `RecipeIngredient *ingredients;` for shapeless.
    // If we allocate that, we should free it. Assuming for now we just free the main array
    // as the ingredients might be static or managed elsewhere for default recipes.
    // For registered recipes we might need a strategy.
    // For now, simple free.
    free(g_recipes);
    g_recipes = NULL;
  }
  g_recipe_count = 0;
  g_recipe_capacity = 0;
}

bool crafting_register_recipe(const Recipe *recipe) {
  if (!g_recipes || g_recipe_count >= g_recipe_capacity || !recipe) return false;

  // Shallow copy for now. Deep copy would be needed for shapeless ingredients array if it's stack allocated.
  g_recipes[g_recipe_count] = *recipe;

  // Assign ID if 0
  if (g_recipes[g_recipe_count].id == 0) {
    g_recipes[g_recipe_count].id = g_recipe_count + 1;
  }

  g_recipe_count++;
  return true;
}

const Recipe *crafting_get_recipe(u32 recipe_id) {
  if (!g_recipes || recipe_id == 0) return NULL;

  for (u32 i = 0; i < g_recipe_count; i++) {
    if (g_recipes[i].id == recipe_id) {
      return &g_recipes[i];
    }
  }
  return NULL;
}

u32 crafting_get_recipe_count(void) {
  return g_recipe_count;
}

// ============================================================================
// CRAFTING OPERATIONS
// ============================================================================

const Recipe *crafting_find_shaped_recipe(const ItemStack *grid, u8 grid_size) {
  if (!g_recipes || !grid) return NULL;

  for (u32 i = 0; i < g_recipe_count; i++) {
    Recipe *r = &g_recipes[i];
    if (r->type != RECIPE_TYPE_SHAPED) continue;

    // Check dimensions fit
    if (r->data.shaped.width > grid_size || r->data.shaped.height > grid_size) continue;

    // Check all possible positions
    for (u32 y = 0; y <= grid_size - r->data.shaped.height; y++) {
      for (u32 x = 0; x <= grid_size - r->data.shaped.width; x++) {
        // Check grid match
        if (check_shaped_grid(&r->data.shaped, grid, grid_size, x, y)) {
          // Verify no other items outside recipe area
          bool extra_items = false;
          for (u32 gy = 0; gy < grid_size; gy++) {
            for (u32 gx = 0; gx < grid_size; gx++) {
              bool in_recipe = (gx >= x && gx < x + r->data.shaped.width) &&
                               (gy >= y && gy < y + r->data.shaped.height);
              if (!in_recipe && grid[gy * grid_size + gx].item_id != 0) {
                extra_items = true;
                break;
              }
            }
            if (extra_items) break;
          }
          if (!extra_items) return r;
        }
      }
    }
  }

  return NULL;
}

const Recipe *crafting_find_shapeless_recipe(const ItemStack *items, u32 item_count) {
  if (!g_recipes || !items) return NULL;

  for (u32 i = 0; i < g_recipe_count; i++) {
    Recipe *r = &g_recipes[i];
    if (r->type != RECIPE_TYPE_SHAPELESS) continue;

    // Quick count check
    u32 total_items_input = 0;
    for (u32 j = 0; j < item_count; j++) {
      if (items[j].item_id != 0) total_items_input++;
    }

    if (total_items_input != r->data.shapeless.ingredient_count) continue;

    // Check ingredients
    bool match = true;
    bool used[9] = {0}; // Assuming max 9 input items
    if (item_count > 9) continue; // Safety

    for (u32 ing_idx = 0; ing_idx < r->data.shapeless.ingredient_count; ing_idx++) {
      bool found = false;
      RecipeIngredient *ing = &r->data.shapeless.ingredients[ing_idx];

      for (u32 input_idx = 0; input_idx < item_count; input_idx++) {
        if (used[input_idx]) continue;
        if (matches_ingredient(ing, &items[input_idx])) {
          used[input_idx] = true;
          found = true;
          break;
        }
      }

      if (!found) {
        match = false;
        break;
      }
    }

    if (match) return r;
  }

  return NULL;
}

CraftingResult crafting_try_craft_shaped(const ItemStack *grid, u8 grid_size) {
  CraftingResult result = {0};
  const Recipe *recipe = crafting_find_shaped_recipe(grid, grid_size);

  if (recipe) {
    result.success = true;
    result.output = item_stack_clone(&recipe->output);
  } else {
    result.error_message = "No matching recipe";
  }
  return result;
}

CraftingResult crafting_try_craft_shapeless(const ItemStack *items, u32 item_count) {
  CraftingResult result = {0};
  const Recipe *recipe = crafting_find_shapeless_recipe(items, item_count);

  if (recipe) {
    result.success = true;
    result.output = item_stack_clone(&recipe->output);
  } else {
    result.error_message = "No matching recipe";
  }
  return result;
}

bool crafting_can_craft(const Container *inventory, const Recipe *recipe) {
  if (!inventory || !recipe) return false;
  return crafting_has_ingredients(inventory, recipe);
}

bool crafting_has_ingredients(const Container *inventory, const Recipe *recipe) {
  if (!inventory || !recipe) return false;

  // Aggregate requirements by item ID to prevent double counting availability
  // Since we don't have a hash map, we'll use a simple static array for unique items found in recipe
  // Assuming a reasonable max number of unique ingredients per recipe (e.g., 9 for 3x3 grid)

  typedef struct {
    u32 item_id;
    u32 total_quantity;
  } Requirement;

  Requirement reqs[9];
  u32 req_count = 0;

  // Initialize requirements
  for (u32 i = 0; i < 9; i++) {
    reqs[i].item_id = 0;
    reqs[i].total_quantity = 0;
  }

  // Helper logic inline since C99 doesn't support nested functions cleanly across all compilers
  if (recipe->type == RECIPE_TYPE_SHAPED) {
    for (u32 i = 0; i < recipe->data.shaped.width * recipe->data.shaped.height; i++) {
      const RecipeIngredient *ing = &recipe->data.shaped.grid[i];
      if (ing->item_id != 0) {
        bool found = false;
        for (u32 k = 0; k < req_count; k++) {
          if (reqs[k].item_id == ing->item_id) {
            reqs[k].total_quantity += ing->quantity;
            found = true;
            break;
          }
        }
        if (!found && req_count < 9) {
          reqs[req_count].item_id = ing->item_id;
          reqs[req_count].total_quantity = ing->quantity;
          req_count++;
        }
      }
    }
  } else if (recipe->type == RECIPE_TYPE_SHAPELESS) {
    for (u32 i = 0; i < recipe->data.shapeless.ingredient_count; i++) {
      const RecipeIngredient *ing = &recipe->data.shapeless.ingredients[i];
      if (ing->item_id != 0) {
        bool found = false;
        for (u32 k = 0; k < req_count; k++) {
          if (reqs[k].item_id == ing->item_id) {
            reqs[k].total_quantity += ing->quantity;
            found = true;
            break;
          }
        }
        if (!found && req_count < 9) {
          reqs[req_count].item_id = ing->item_id;
          reqs[req_count].total_quantity = ing->quantity;
          req_count++;
        }
      }
    }
  }

  // Verify inventory has enough of each unique item
  for (u32 i = 0; i < req_count; i++) {
    if (!container_has_item(inventory, reqs[i].item_id, reqs[i].total_quantity)) {
      return false;
    }
  }

  return true;
}

bool crafting_craft(Container *inventory, const Recipe *recipe, ItemStack *out_result) {
  if (!inventory || !recipe || !out_result) return false;

  // This function assumes 'recipe' was found based on 'inventory' contents which acts as the grid?
  // Actually, 'crafting_craft' usually takes the crafting grid container, not the player inventory directly if it's shaped.
  // But the header says 'Container *inventory'.
  // If it's a station craft, we use crafting_station_craft.
  // If this is auto-crafting from inventory (like "Quick Craft"), we remove ingredients.

  if (!crafting_has_ingredients(inventory, recipe)) return false;

  // Remove ingredients
  if (recipe->type == RECIPE_TYPE_SHAPED) {
    for (u32 i = 0; i < recipe->data.shaped.width * recipe->data.shaped.height; i++) {
      const RecipeIngredient *ing = &recipe->data.shaped.grid[i];
      if (ing->item_id != 0) {
        container_remove_item_by_id(inventory, ing->item_id, ing->quantity);
      }
    }
  } else if (recipe->type == RECIPE_TYPE_SHAPELESS) {
    for (u32 i = 0; i < recipe->data.shapeless.ingredient_count; i++) {
      const RecipeIngredient *ing = &recipe->data.shapeless.ingredients[i];
      if (ing->item_id != 0) {
        container_remove_item_by_id(inventory, ing->item_id, ing->quantity);
      }
    }
  }

  *out_result = item_stack_clone(&recipe->output);
  return true;
}

// ============================================================================
// CRAFTING STATION
// ============================================================================

CraftingStation crafting_station_create(u8 grid_size) {
  CraftingStation station = {0};
  station.grid_size = grid_size;
  return station;
}

void crafting_station_update(CraftingStation *station) {
  if (!station) return;

  CraftingResult result = crafting_try_craft_shaped(station->grid, station->grid_size);
  if (result.success) {
    station->result = result.output;
    station->active_recipe = crafting_find_shaped_recipe(station->grid, station->grid_size);
  } else {
    station->result = (ItemStack){0};
    station->active_recipe = NULL;
  }
}

bool crafting_station_craft(CraftingStation *station, Container *inventory) {
  if (!station || !inventory || !station->active_recipe) return false;

  // Add result to inventory
  u32 slot;
  if (!container_add_item(inventory, station->result, &slot)) {
    return false; // Inventory full
  }

  // Consume ingredients from grid
  for (u32 i = 0; i < station->grid_size * station->grid_size; i++) {
    if (station->grid[i].item_id != 0) {
      station->grid[i].quantity--;
      if (station->grid[i].quantity == 0) {
        station->grid[i] = (ItemStack){0};
      }
    }
  }

  // Update result for next craft
  crafting_station_update(station);
  return true;
}

// ============================================================================
// FURNACE
// ============================================================================

Furnace furnace_create(void) {
  Furnace furnace = {0};
  return furnace;
}

bool furnace_add_input(Furnace *furnace, ItemStack item) {
  if (!furnace) return false;
  if (furnace->input.item_id == 0) {
    furnace->input = item;
    return true;
  } else if (item_can_stack_with(&furnace->input, &item)) {
    return item_stack_merge(&furnace->input, &item) > 0;
  }
  return false;
}

bool furnace_add_fuel(Furnace *furnace, ItemStack item) {
  if (!furnace) return false;
  if (furnace->fuel.item_id == 0) {
    furnace->fuel = item;
    return true;
  } else if (item_can_stack_with(&furnace->fuel, &item)) {
    return item_stack_merge(&furnace->fuel, &item) > 0;
  }
  return false;
}

ItemStack furnace_take_output(Furnace *furnace) {
  ItemStack output = {0};
  if (furnace && furnace->output.item_id != 0) {
    output = furnace->output;
    furnace->output = (ItemStack){0};
  }
  return output;
}

// ============================================================================
// DEFAULT RECIPES
// ============================================================================

void crafting_register_default_recipes(void) {
  // Test recipe: 1 Wood (ID 1) -> 4 Planks (ID 2)
  Recipe planks = {
    .name = "Planks",
    .type = RECIPE_TYPE_SHAPELESS,
    .output = item_create_stack(2, 4), // Requires item 2 to exist
    .required_level = 0,
    .discovered = true
  };

  // Need to allocate ingredients for shapeless
  static RecipeIngredient planks_ingredients[1] = {
    { .item_id = 1, .quantity = 1 }
  };
  planks.data.shapeless.ingredients = planks_ingredients;
  planks.data.shapeless.ingredient_count = 1;

  crafting_register_recipe(&planks);

  // Test recipe: 4 Planks (ID 2) in 2x2 -> 1 Crafting Table (ID 3)
  Recipe table = {
    .name = "Crafting Table",
    .type = RECIPE_TYPE_SHAPED,
    .output = item_create_stack(3, 1),
    .required_level = 0,
    .discovered = true
  };

  table.data.shaped.width = 2;
  table.data.shaped.height = 2;
  // Index 0, 1, 2, 3
  table.data.shaped.grid[0] = (RecipeIngredient){ .item_id = 2, .quantity = 1 };
  table.data.shaped.grid[1] = (RecipeIngredient){ .item_id = 2, .quantity = 1 };
  table.data.shaped.grid[2] = (RecipeIngredient){ .item_id = 2, .quantity = 1 };
  table.data.shaped.grid[3] = (RecipeIngredient){ .item_id = 2, .quantity = 1 };

  crafting_register_recipe(&table);
}
