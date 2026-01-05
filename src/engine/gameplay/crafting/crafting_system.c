#include "gameplay/crafting/crafting.h"
#include "gameplay/inventory/item_database.h"
#include <core/hashmap.h>
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>

// Global crafting system state
static struct {
  Recipe *recipes;
  u32 recipe_count;
  u32 capacity;
  HashMap *recipe_map;
  bool initialized;
} g_crafting = {0};

// Helper functions for u32 hashmap
static u32 hash_u32(const void *key) {
  return *(const u32 *)key;
}

static bool equals_u32(const void *a, const void *b) {
  return *(const u32 *)a == *(const u32 *)b;
}

// ============================================================================
// RECIPE DATABASE
// ============================================================================

bool crafting_system_init(u32 max_recipes) {
  if (g_crafting.initialized)
    return true;

  g_crafting.capacity = max_recipes;
  g_crafting.recipes = (Recipe *)MALLOC_TAGGED(sizeof(Recipe) * max_recipes,
                                               MEMORY_TAG_GAMEPLAY);
  if (!g_crafting.recipes)
    return false;

  g_crafting.recipe_map =
      hashmap_create(1024, sizeof(u32), sizeof(u32), hash_u32, equals_u32);
  if (!g_crafting.recipe_map) {
    crafting_system_shutdown();
    return false;
  }

  g_crafting.recipe_count = 0;
  g_crafting.initialized = true;

  return true;
}

void crafting_system_shutdown(void) {
  if (!g_crafting.initialized)
    return;

  if (g_crafting.recipes) {
    // Free shapeless ingredient arrays
    for (u32 i = 0; i < g_crafting.recipe_count; i++) {
      if (g_crafting.recipes[i].type == RECIPE_TYPE_SHAPELESS &&
          g_crafting.recipes[i].data.shapeless.ingredients) {
        FREE(g_crafting.recipes[i].data.shapeless.ingredients);
      }
    }
    FREE(g_crafting.recipes);
  }
  if (g_crafting.recipe_map) {
    hashmap_destroy(g_crafting.recipe_map);
  }

  memset(&g_crafting, 0, sizeof(g_crafting));
}

bool crafting_register_recipe(const Recipe *recipe) {
  if (!g_crafting.initialized || !recipe)
    return false;
  if (g_crafting.recipe_count >= g_crafting.capacity)
    return false;

  u32 index = g_crafting.recipe_count++;
  g_crafting.recipes[index] = *recipe;

  // Deep copy shapeless ingredients
  if (recipe->type == RECIPE_TYPE_SHAPELESS) {
    u32 count = recipe->data.shapeless.ingredient_count;
    g_crafting.recipes[index].data.shapeless.ingredients =
        (RecipeIngredient *)MALLOC_TAGGED(sizeof(RecipeIngredient) * count,
                                          MEMORY_TAG_GAMEPLAY);
    memcpy(g_crafting.recipes[index].data.shapeless.ingredients,
           recipe->data.shapeless.ingredients,
           sizeof(RecipeIngredient) * count);
  }

  hashmap_set(g_crafting.recipe_map, &g_crafting.recipes[index].id, &index);

  return true;
}

bool crafting_load_recipes_from_json(const char *filepath) {
  // TODO: JSON loading
  (void)filepath;
  return false;
}

const Recipe *crafting_get_recipe(u32 recipe_id) {
  if (!g_crafting.initialized)
    return NULL;

  void *index_ptr =
      hashmap_get(g_crafting.recipe_map, &recipe_id);
  if (!index_ptr) {
    return NULL;
  }

  u32 index = *(u32 *)index_ptr;
  if (index >= g_crafting.recipe_count)
    return NULL;

  return &g_crafting.recipes[index];
}

u32 crafting_get_recipe_count(void) {
  return g_crafting.initialized ? g_crafting.recipe_count : 0;
}

// ============================================================================
// PATTERN MATCHING
// ============================================================================

static bool ingredient_matches(const RecipeIngredient *ingredient,
                               const ItemStack *stack) {
  if (!ingredient || !stack || !stack->item)
    return false;

  // Check specific item
  if (ingredient->item_id != 0) {
    return stack->item->id == ingredient->item_id &&
           stack->quantity >= ingredient->quantity;
  }

  // Check category
  return stack->item->type == ingredient->category &&
         stack->quantity >= ingredient->quantity;
}

static bool grid_matches_pattern(const ItemStack *grid,
                                 const ShapedRecipe *pattern, u8 grid_size,
                                 bool mirrored) {
  for (u8 y = 0; y < pattern->height; y++) {
    for (u8 x = 0; x < pattern->width; x++) {
      u8 grid_x = mirrored ? (pattern->width - 1 - x) : x;
      u8 grid_idx = y * grid_size + grid_x;
      u8 pattern_idx = y * pattern->width + x;

      const ItemStack *grid_slot = &grid[grid_idx];
      const RecipeIngredient *pattern_slot = &pattern->grid[pattern_idx];

      // Empty slots
      if (pattern_slot->item_id == 0 && pattern_slot->category == 0) {
        if (grid_slot->item != NULL)
          return false;
        continue;
      }

      // Match ingredient
      if (!ingredient_matches(pattern_slot, grid_slot)) {
        return false;
      }
    }
  }

  return true;
}

const Recipe *crafting_find_shaped_recipe(const ItemStack *grid, u8 grid_size) {
  if (!g_crafting.initialized || !grid)
    return NULL;

  for (u32 i = 0; i < g_crafting.recipe_count; i++) {
    Recipe *recipe = &g_crafting.recipes[i];
    if (recipe->type != RECIPE_TYPE_SHAPED)
      continue;

    const ShapedRecipe *pattern = &recipe->data.shaped;
    if (pattern->width > grid_size || pattern->height > grid_size)
      continue;

    // Try normal orientation
    if (grid_matches_pattern(grid, pattern, grid_size, false)) {
      return recipe;
    }

    // Try mirrored if allowed
    if (pattern->mirror &&
        grid_matches_pattern(grid, pattern, grid_size, true)) {
      return recipe;
    }
  }

  return NULL;
}

const Recipe *crafting_find_shapeless_recipe(const ItemStack *items,
                                             u32 item_count) {
  if (!g_crafting.initialized || !items)
    return NULL;

  for (u32 i = 0; i < g_crafting.recipe_count; i++) {
    Recipe *recipe = &g_crafting.recipes[i];
    if (recipe->type != RECIPE_TYPE_SHAPELESS)
      continue;

    const ShapelessRecipe *shapeless = &recipe->data.shapeless;
    if (shapeless->ingredient_count != item_count)
      continue;

    // Check if all ingredients match (order doesn't matter)
    bool *matched = (bool *)calloc(item_count, sizeof(bool));
    bool all_matched = true;

    for (u32 j = 0; j < shapeless->ingredient_count; j++) {
      bool found = false;
      for (u32 k = 0; k < item_count; k++) {
        if (!matched[k] &&
            ingredient_matches(&shapeless->ingredients[j], &items[k])) {
          matched[k] = true;
          found = true;
          break;
        }
      }
      if (!found) {
        all_matched = false;
        break;
      }
    }

    free(matched);

    if (all_matched) {
      return recipe;
    }
  }

  return NULL;
}

// ============================================================================
// CRAFTING OPERATIONS
// ============================================================================

CraftingResult crafting_try_craft_shaped(const ItemStack *grid, u8 grid_size) {
  CraftingResult result = {0};

  const Recipe *recipe = crafting_find_shaped_recipe(grid, grid_size);
  if (!recipe) {
    result.error_message = "No matching recipe";
    return result;
  }

  result.success = true;
  result.output = recipe->output;
  return result;
}

CraftingResult crafting_try_craft_shapeless(const ItemStack *items,
                                            u32 item_count) {
  CraftingResult result = {0};

  const Recipe *recipe = crafting_find_shapeless_recipe(items, item_count);
  if (!recipe) {
    result.error_message = "No matching recipe";
    return result;
  }

  result.success = true;
  result.output = recipe->output;
  return result;
}

bool crafting_can_craft(const Container *inventory, const Recipe *recipe) {
  return crafting_has_ingredients(inventory, recipe);
}

bool crafting_has_ingredients(const Container *inventory,
                              const Recipe *recipe) {
  if (!inventory || !recipe)
    return false;

  if (recipe->type == RECIPE_TYPE_SHAPED) {
    const ShapedRecipe *shaped = &recipe->data.shaped;
    for (u32 i = 0; i < 9; i++) {
      const RecipeIngredient *ing = &shaped->grid[i];
      if (ing->item_id == 0 && ing->category == 0)
        continue;

      if (ing->item_id != 0) {
        if (!container_has_item(inventory, ing->item_id, ing->quantity)) {
          return false;
        }
      }
    }
  } else if (recipe->type == RECIPE_TYPE_SHAPELESS) {
    const ShapelessRecipe *shapeless = &recipe->data.shapeless;
    for (u32 i = 0; i < shapeless->ingredient_count; i++) {
      const RecipeIngredient *ing = &shapeless->ingredients[i];
      if (ing->item_id != 0) {
        if (!container_has_item(inventory, ing->item_id, ing->quantity)) {
          return false;
        }
      }
    }
  }

  return true;
}

bool crafting_craft(Container *inventory, const Recipe *recipe,
                    ItemStack *out_result) {
  if (!crafting_has_ingredients(inventory, recipe))
    return false;

  // Remove ingredients
  if (recipe->type == RECIPE_TYPE_SHAPED) {
    const ShapedRecipe *shaped = &recipe->data.shaped;
    for (u32 i = 0; i < 9; i++) {
      const RecipeIngredient *ing = &shaped->grid[i];
      if (ing->item_id != 0) {
        container_remove_item_by_id(inventory, ing->item_id, ing->quantity);
      }
    }
  }

  // Add output
  if (out_result) {
    *out_result = recipe->output;
  }

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
  if (!station)
    return;

  const Recipe *recipe =
      crafting_find_shaped_recipe(station->grid, station->grid_size);
  if (recipe) {
    station->result = recipe->output;
    station->active_recipe = recipe;
  } else {
    memset(&station->result, 0, sizeof(ItemStack));
    station->active_recipe = NULL;
  }
}

bool crafting_station_craft(CraftingStation *station, Container *inventory) {
  if (!station || !station->active_recipe)
    return false;

  // Consume items from grid
  for (u32 i = 0; i < 9; i++) {
    if (station->grid[i].item) {
      station->grid[i].quantity--;
      if (station->grid[i].quantity == 0) {
        memset(&station->grid[i], 0, sizeof(ItemStack));
      }
    }
  }

  // Add output to inventory
  container_add_item(inventory, station->result, NULL);

  // Update preview
  crafting_station_update(station);

  return true;
}

// ============================================================================
// FURNACE/SMELTING
// ============================================================================

Furnace furnace_create(void) {
  Furnace furnace = {0};
  return furnace;
}

void furnace_update(Furnace *furnace, f32 delta_time) {
  if (!furnace || !furnace->active_recipe)
    return;

  // Consume fuel
  if (furnace->fuel_remaining > 0.0f) {
    furnace->fuel_remaining -= delta_time;

    // Update progress
    furnace->progress +=
        delta_time / furnace->active_recipe->data.smelting.cook_time;

    // Complete
    if (furnace->progress >= 1.0f) {
      furnace->output = furnace->active_recipe->output;
      furnace->input.quantity--;
      if (furnace->input.quantity == 0) {
        memset(&furnace->input, 0, sizeof(ItemStack));
      }
      furnace->progress = 0.0f;
      furnace->active_recipe = NULL;
    }
  }
}

bool furnace_add_input(Furnace *furnace, ItemStack item) {
  if (!furnace || furnace->input.item)
    return false;
  furnace->input = item;
  return true;
}

bool furnace_add_fuel(Furnace *furnace, ItemStack item) {
  if (!furnace || furnace->fuel.item)
    return false;
  furnace->fuel = item;
  furnace->fuel_remaining = 10.0f; // TODO: fuel value from item
  return true;
}

ItemStack furnace_take_output(Furnace *furnace) {
  if (!furnace) {
    ItemStack empty = {0};
    return empty;
  }

  ItemStack output = furnace->output;
  memset(&furnace->output, 0, sizeof(ItemStack));
  return output;
}

// ============================================================================
// DEFAULT RECIPES
// ============================================================================

void crafting_register_default_recipes(void) {
  // Planks from wood (shapeless)
  Recipe planks = {
      .id = 1,
      .name = "Wood Planks",
      .type = RECIPE_TYPE_SHAPELESS,
      .output = item_create_stack(3002, 4), // 4 planks
  };
  RecipeIngredient wood_ing = {.item_id = 3001, .quantity = 1}; // Wood
  planks.data.shapeless.ingredients = &wood_ing;
  planks.data.shapeless.ingredient_count = 1;
  crafting_register_recipe(&planks);

  // Stick from planks (shaped 2x2)
  Recipe stick = {
      .id = 2,
      .name = "Stick",
      .type = RECIPE_TYPE_SHAPED,
      .output = item_create_stack(3003, 4), // 4 sticks
  };
  stick.data.shaped.width = 1;
  stick.data.shaped.height = 2;
  stick.data.shaped.grid[0] =
      (RecipeIngredient){.item_id = 3002, .quantity = 1}; // Plank
  stick.data.shaped.grid[1] =
      (RecipeIngredient){.item_id = 3002, .quantity = 1}; // Plank
  crafting_register_recipe(&stick);
}
