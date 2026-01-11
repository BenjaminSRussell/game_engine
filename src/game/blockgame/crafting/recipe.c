// Recipe helpers and matching logic.
// TODO: Implement recipe pattern matching optimization.
// TODO: Implement recipe validation system for invalid combinations.
// TODO: Add recipe priority system for conflict resolution.
// TODO: Implement recipe substitution system for similar ingredients.
// TODO: Add recipe difficulty system affecting crafting time.
// TODO: Implement recipe unlock system through progression.
// TODO: Add recipe search and filtering system.
// TODO: Implement recipe serialization for save/load.
// TODO: Add recipe versioning for compatibility.
#include <crafting/recipe_system.h>
#include <inventory/inventory.h>
#include <stdlib.h>
#include <string.h>

#define CRAFT_GRID_WIDTH 3u
#define CRAFT_GRID_HEIGHT 3u
#define CRAFT_GRID_SLOTS (CRAFT_GRID_WIDTH * CRAFT_GRID_HEIGHT)

static bool recipe_is_shaped(const Recipe *recipe) {
  return recipe && recipe->pattern && recipe->pattern_width > 0 &&
         recipe->pattern_height > 0;
}

static void recipe_snapshot_grid(const Inventory *inventory,
                                 u32 grid[CRAFT_GRID_SLOTS]) {
  for (u32 i = 0; i < CRAFT_GRID_SLOTS; i++) {
    grid[i] = inventory->slots[i].item_id;
  }
}

static bool recipe_grid_bounds(const u32 grid[CRAFT_GRID_SLOTS], u32 *min_x,
                               u32 *min_y, u32 *max_x, u32 *max_y) {
  bool found = false;
  u32 minx = CRAFT_GRID_WIDTH;
  u32 miny = CRAFT_GRID_HEIGHT;
  u32 maxx = 0;
  u32 maxy = 0;

  for (u32 y = 0; y < CRAFT_GRID_HEIGHT; y++) {
    for (u32 x = 0; x < CRAFT_GRID_WIDTH; x++) {
      if (grid[y * CRAFT_GRID_WIDTH + x] != 0) {
        if (!found) {
          minx = x;
          maxx = x;
          miny = y;
          maxy = y;
          found = true;
        } else {
          if (x < minx)
            minx = x;
          if (x > maxx)
            maxx = x;
          if (y < miny)
            miny = y;
          if (y > maxy)
            maxy = y;
        }
      }
    }
  }

  if (!found) {
    return false;
  }

  if (min_x)
    *min_x = minx;
  if (min_y)
    *min_y = miny;
  if (max_x)
    *max_x = maxx;
  if (max_y)
    *max_y = maxy;
  return true;
}

static bool recipe_match_pattern(const Recipe *recipe,
                                 const Inventory *inventory, u32 *origin_x,
                                 u32 *origin_y) {
  if (!recipe_is_shaped(recipe) || !inventory) {
    return false;
  }

  if (recipe->pattern_width > CRAFT_GRID_WIDTH ||
      recipe->pattern_height > CRAFT_GRID_HEIGHT) {
    return false;
  }

  u32 grid[CRAFT_GRID_SLOTS];
  recipe_snapshot_grid(inventory, grid);

  u32 min_x = 0;
  u32 min_y = 0;
  u32 max_x = 0;
  u32 max_y = 0;
  if (!recipe_grid_bounds(grid, &min_x, &min_y, &max_x, &max_y)) {
    return false;
  }

  u32 width = max_x - min_x + 1;
  u32 height = max_y - min_y + 1;
  if (width != recipe->pattern_width || height != recipe->pattern_height) {
    return false;
  }

  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      u32 grid_id = grid[(min_y + y) * CRAFT_GRID_WIDTH + (min_x + x)];
      u32 pattern_id = recipe->pattern[y * recipe->pattern_width + x];
      if (grid_id != pattern_id) {
        return false;
      }
    }
  }

  if (origin_x)
    *origin_x = min_x;
  if (origin_y)
    *origin_y = min_y;
  return true;
}

static bool recipe_consume_slot(Inventory *inventory, u32 slot_index,
                                u32 expected_item_id) {
  if (!inventory || slot_index >= MAX_INVENTORY_SLOTS) {
    return false;
  }

  InventorySlot *slot = &inventory->slots[slot_index];
  if (slot->item_id != expected_item_id || slot->count == 0) {
    return false;
  }

  slot->count -= 1;
  if (inventory->total_items > 0) {
    inventory->total_items -= 1;
  }
  if (slot->count == 0) {
    slot->item_id = 0;
  }

  inventory->dirty = true;
  if (inventory->on_event) {
    inventory->on_event(inventory, INVENTORY_EVENT_REMOVE, expected_item_id, 1,
                        slot_index, inventory->user_data);
  }

  return true;
}

static void recipe_restore_slot(Inventory *inventory, u32 slot_index,
                                u32 item_id) {
  if (!inventory || slot_index >= MAX_INVENTORY_SLOTS) {
    return;
  }

  InventorySlot *slot = &inventory->slots[slot_index];
  if (slot->item_id == 0) {
    slot->item_id = item_id;
    slot->count = 1;
    inventory->total_items += 1;
    inventory->dirty = true;
    if (inventory->on_event) {
      inventory->on_event(inventory, INVENTORY_EVENT_ADD, item_id, 1,
                          slot_index, inventory->user_data);
    }
    return;
  }

  if (slot->item_id == item_id) {
    slot->count += 1;
    inventory->total_items += 1;
    inventory->dirty = true;
    if (inventory->on_event) {
      inventory->on_event(inventory, INVENTORY_EVENT_ADD, item_id, 1,
                          slot_index, inventory->user_data);
    }
    return;
  }

  inventory_add_item(inventory, item_id, 1);
}

// Initialize recipe
void recipe_init(Recipe *recipe, u32 result_item, u32 result_quantity) {
  recipe->result_item = result_item;
  recipe->result_quantity = result_quantity;
  recipe->ingredient_count = 0;
  // ingredients is a fixed array, no need to set to NULL
  recipe->pattern_width = 0;
  recipe->pattern_height = 0;
  recipe->pattern = NULL;
}

// Free recipe
void recipe_free(Recipe *recipe) {
  // ingredients is a fixed array, no need to free
  if (recipe->pattern) {
    free(recipe->pattern);
    recipe->pattern = NULL;
  }
  recipe->ingredient_count = 0;
}

// Add ingredient to recipe
void recipe_add_ingredient(Recipe *recipe, u32 item_id, u32 quantity) {
  if (!recipe)
    return;

  recipe->ingredients = (RecipeIngredient *)realloc(
      recipe->ingredients,
      (recipe->ingredient_count + 1) * sizeof(RecipeIngredient));
  recipe->ingredients[recipe->ingredient_count].item_id = item_id;
  recipe->ingredients[recipe->ingredient_count].quantity = quantity;
  recipe->ingredient_count++;
}

// Set recipe pattern (for shaped recipes)
void recipe_set_pattern(Recipe *recipe, u32 width, u32 height, u32 *pattern) {
  if (!recipe)
    return;

  recipe->pattern_width = width;
  recipe->pattern_height = height;

  if (recipe->pattern) {
    free(recipe->pattern);
  }

  recipe->pattern = (u32 *)malloc(width * height * sizeof(u32));
  memcpy(recipe->pattern, pattern, width * height * sizeof(u32));
}

// Check if inventory can craft recipe
bool recipe_can_craft(Recipe *recipe, Inventory *inventory) {
  if (!recipe || !inventory)
    return false;

  if (recipe_is_shaped(recipe)) {
    return recipe_match_pattern(recipe, inventory, NULL, NULL);
  }

  // Check all ingredients
  for (u32 i = 0; i < recipe->ingredient_count; i++) {
    u32 required = recipe->ingredients[i].quantity;
    u32 available =
        inventory_get_item_count(inventory, recipe->ingredients[i].item_id);

    if (available < required) {
      return false;
    }
  }

  return true;
}

bool recipe_consume_inputs(Recipe *recipe, Inventory *inventory) {
  if (!recipe || !inventory)
    return false;

  if (recipe_is_shaped(recipe)) {
    u32 origin_x = 0;
    u32 origin_y = 0;
    if (!recipe_match_pattern(recipe, inventory, &origin_x, &origin_y)) {
      return false;
    }

    u32 consumed_slots[CRAFT_GRID_SLOTS];
    u32 consumed_items[CRAFT_GRID_SLOTS];
    u32 consumed_count = 0;

    for (u32 y = 0; y < recipe->pattern_height; y++) {
      for (u32 x = 0; x < recipe->pattern_width; x++) {
        u32 item_id = recipe->pattern[y * recipe->pattern_width + x];
        if (item_id == 0) {
          continue;
        }

        u32 slot_index = (origin_y + y) * CRAFT_GRID_WIDTH + (origin_x + x);
        if (!recipe_consume_slot(inventory, slot_index, item_id)) {
          for (u32 i = 0; i < consumed_count; i++) {
            recipe_restore_slot(inventory, consumed_slots[i],
                                consumed_items[i]);
          }
          return false;
        }

        consumed_slots[consumed_count] = slot_index;
        consumed_items[consumed_count] = item_id;
        consumed_count++;
      }
    }

    return true;
  }

  if (!recipe_can_craft(recipe, inventory)) {
    return false;
  }

  for (u32 i = 0; i < recipe->ingredient_count; i++) {
    if (!inventory_remove_item(inventory, recipe->ingredients[i].item_id,
                               recipe->ingredients[i].quantity)) {
      for (u32 j = 0; j < i; j++) {
        inventory_add_item(inventory, recipe->ingredients[j].item_id,
                           recipe->ingredients[j].quantity);
      }
      return false;
    }
  }

  return true;
}

// Craft recipe (remove ingredients, add result)
bool recipe_craft(Recipe *recipe, Inventory *inventory) {
  if (!recipe || !inventory)
    return false;

  // Check if can craft
  if (!recipe_consume_inputs(recipe, inventory)) {
    return false;
  }

  // Add result
  if (!inventory_add_item(inventory, recipe->result_item,
                          recipe->result_quantity)) {
    // Failed to add result, try to restore inputs
    if (recipe_is_shaped(recipe)) {
      for (u32 y = 0; y < recipe->pattern_height; y++) {
        for (u32 x = 0; x < recipe->pattern_width; x++) {
          u32 item_id = recipe->pattern[y * recipe->pattern_width + x];
          if (item_id == 0) {
            continue;
          }
          inventory_add_item(inventory, item_id, 1);
        }
      }
    } else {
      for (u32 i = 0; i < recipe->ingredient_count; i++) {
        inventory_add_item(inventory, recipe->ingredients[i].item_id,
                           recipe->ingredients[i].quantity);
      }
    }
    return false;
  }

  return true;
}
