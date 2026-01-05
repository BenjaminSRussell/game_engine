// Advanced crafting recipes and processing.
// Roadmap: docs/ADVANCED_CRAFTING_ROADMAP.md.
// TODO: Implement multi-step crafting recipes with intermediate products.
// TODO: Add crafting station upgrade system for better recipes.
// TODO: Implement crafting quality system affecting output properties.
// TODO: Add crafting failure chance system for risk/reward.
// TODO: Implement crafting automation system with machines.
// TODO: Add crafting recipe discovery through experimentation.
// TODO: Implement crafting material substitution system.
// TODO: Add crafting time system for complex recipes.
// TODO: Implement crafting skill system affecting success rates.
// TODO: Add crafting recipe book with search and filtering.
#include <crafting/advanced_crafting.h>
#include <crafting/recipe.h>
#include <inventory/inventory.h>
#include <stdlib.h>
#include <string.h>

// brewing animations, anvil hammer effects.
// sparkles, brewing bubbles, anvil smoke).
// crafting station type.

static AdvancedCraftingSystem g_advanced_crafting = {0};

static u32 ingredient_count_nonzero(const u32 *ingredients,
                                    u32 ingredient_count) {
  u32 count = 0;
  if (!ingredients) {
    return 0;
  }
  for (u32 i = 0; i < ingredient_count; i++) {
    if (ingredients[i] != 0) {
      count++;
    }
  }
  return count;
}

static u32 ingredient_count_for_item(const u32 *ingredients,
                                     u32 ingredient_count, u32 item_id) {
  u32 count = 0;
  if (!ingredients || item_id == 0) {
    return 0;
  }
  for (u32 i = 0; i < ingredient_count; i++) {
    if (ingredients[i] == item_id) {
      count++;
    }
  }
  return count;
}

static bool recipe_has_item(const Recipe *recipe, u32 item_id) {
  if (!recipe || item_id == 0) {
    return false;
  }
  for (u32 i = 0; i < recipe->ingredient_count; i++) {
    if (recipe->ingredients[i].item_id == item_id) {
      return true;
    }
  }
  return false;
}

static bool pattern_bounds(const u32 *pattern, u32 width, u32 height,
                           u32 *min_x, u32 *min_y, u32 *max_x, u32 *max_y) {
  bool found = false;
  u32 minx = width;
  u32 miny = height;
  u32 maxx = 0;
  u32 maxy = 0;

  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      if (pattern[y * width + x] != 0) {
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

// Initialize advanced crafting system
void advanced_crafting_init(AdvancedCraftingSystem *system, u32 max_recipes) {
  if (!system)
    return;

  system->recipes = (Recipe *)calloc(max_recipes, sizeof(Recipe));
  system->recipe_capacity = max_recipes;
  system->recipe_count = 0;

  system->workbenches = (Workbench *)calloc(WORKBENCH_COUNT, sizeof(Workbench));
  system->workbench_count = WORKBENCH_COUNT;

  // Initialize workbenches
  system->workbenches[WORKBENCH_CRAFTING_TABLE] =
      (Workbench){.type = WORKBENCH_CRAFTING_TABLE,
                  .block_id = 100, // Placeholder
                  .name = "Crafting Table",
                  .recipe_slots = 9,
                  .supports_shaped = true,
                  .supports_shapeless = true};

  system->workbenches[WORKBENCH_FURNACE] =
      (Workbench){.type = WORKBENCH_FURNACE,
                  .block_id = BLOCK_FURNACE,
                  .name = "Furnace",
                  .recipe_slots = 1,
                  .supports_shaped = false,
                  .supports_shapeless = true};

  system->workbenches[WORKBENCH_ANVIL] =
      (Workbench){.type = WORKBENCH_ANVIL,
                  .block_id = 101, // Placeholder
                  .name = "Anvil",
                  .recipe_slots = 3,
                  .supports_shaped = false,
                  .supports_shapeless = true};
}

// Register recipe
bool advanced_crafting_register_recipe(AdvancedCraftingSystem *system,
                                       Recipe *recipe) {
  if (!system || !recipe || system->recipe_count >= system->recipe_capacity) {
    return false;
  }

  memcpy(&system->recipes[system->recipe_count], recipe, sizeof(Recipe));
  system->recipe_count++;
  return true;
}

// Find recipe by ingredients (shapeless)
Recipe *advanced_crafting_find_shapeless(AdvancedCraftingSystem *system,
                                         u32 *ingredients,
                                         u32 ingredient_count) {
  if (!system || !ingredients)
    return NULL;

  u32 input_count = ingredient_count_nonzero(ingredients, ingredient_count);
  if (input_count == 0) {
    return NULL;
  }

  for (u32 i = 0; i < system->recipe_count; i++) {
    Recipe *recipe = &system->recipes[i];

    if (recipe->ingredient_count == 0) {
      continue;
    }

    u32 required_total = 0;
    for (u32 j = 0; j < recipe->ingredient_count; j++) {
      required_total += recipe->ingredients[j].quantity;
    }
    if (required_total != input_count) {
      continue;
    }

    // Check if all ingredients are present (order doesn't matter)
    bool match = true;
    for (u32 j = 0; j < recipe->ingredient_count; j++) {
      u32 item_id = recipe->ingredients[j].item_id;
      bool seen = false;
      for (u32 k = 0; k < j; k++) {
        if (recipe->ingredients[k].item_id == item_id) {
          seen = true;
          break;
        }
      }
      if (seen) {
        continue;
      }

      u32 needed = 0;
      for (u32 k = j; k < recipe->ingredient_count; k++) {
        if (recipe->ingredients[k].item_id == item_id) {
          needed += recipe->ingredients[k].quantity;
        }
      }
      u32 have =
          ingredient_count_for_item(ingredients, ingredient_count, item_id);
      if (have < needed) {
        match = false;
        break;
      }
    }

    if (match) {
      for (u32 j = 0; j < ingredient_count; j++) {
        if (ingredients[j] == 0) {
          continue;
        }
        if (!recipe_has_item(recipe, ingredients[j])) {
          match = false;
          break;
        }
      }
    }

    if (match)
      return recipe;
  }

  return NULL;
}

// Find recipe by pattern (shaped)
Recipe *advanced_crafting_find_shaped(AdvancedCraftingSystem *system,
                                      u32 *pattern, u32 width, u32 height) {
  if (!system || !pattern)
    return NULL;

  if (width == 0 || height == 0) {
    return NULL;
  }

  u32 min_x = 0;
  u32 min_y = 0;
  u32 max_x = 0;
  u32 max_y = 0;
  if (!pattern_bounds(pattern, width, height, &min_x, &min_y, &max_x, &max_y)) {
    return NULL;
  }

  u32 input_width = max_x - min_x + 1;
  u32 input_height = max_y - min_y + 1;

  for (u32 i = 0; i < system->recipe_count; i++) {
    Recipe *recipe = &system->recipes[i];

    if (!recipe->pattern)
      continue; // Not a shaped recipe

    // Check dimensions
    if (recipe->pattern_width != input_width ||
        recipe->pattern_height != input_height)
      continue;

    // Check pattern match
    bool match = true;
    for (u32 y = 0; y < input_height; y++) {
      for (u32 x = 0; x < input_width; x++) {
        u32 idx = (min_y + y) * width + (min_x + x);
        u32 recipe_idx = y * recipe->pattern_width + x;

        if (pattern[idx] != recipe->pattern[recipe_idx]) {
          match = false;
          break;
        }
      }
      if (!match)
        break;
    }

    if (match)
      return recipe;
  }

  return NULL;
}

// Get workbench by block ID
Workbench *advanced_crafting_get_workbench(AdvancedCraftingSystem *system,
                                           u32 block_id) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->workbench_count; i++) {
    if (system->workbenches[i].block_id == block_id) {
      return &system->workbenches[i];
    }
  }

  return NULL;
}

// Initialize default advanced recipes
void advanced_crafting_init_defaults(AdvancedCraftingSystem *system) {
  if (!system)
    return;

  // Tool recipes
  Recipe wooden_pickaxe;
  recipe_init(&wooden_pickaxe, 200, 1); // ITEM_WOODEN_PICKAXE
  u32 pickaxe_pattern[] = {5, 5,  5,    // Wood
                           0, 29, 0,    // Stick (placeholder)
                           0, 29, 0};
  recipe_set_pattern(&wooden_pickaxe, 3, 3, pickaxe_pattern);
  advanced_crafting_register_recipe(system, &wooden_pickaxe);

  Recipe stone_pickaxe;
  recipe_init(&stone_pickaxe, 201, 1);     // ITEM_STONE_PICKAXE
  u32 stone_pickaxe_pattern[] = {1, 1,  1, // Stone
                                 0, 29, 0, 0, 29, 0};
  recipe_set_pattern(&stone_pickaxe, 3, 3, stone_pickaxe_pattern);
  advanced_crafting_register_recipe(system, &stone_pickaxe);

  Recipe iron_pickaxe;
  recipe_init(&iron_pickaxe, 202, 1);       // ITEM_IRON_PICKAXE
  u32 iron_pickaxe_pattern[] = {27, 27, 27, // Iron Ingot (placeholder)
                                0,  29, 0,  0, 29, 0};
  recipe_set_pattern(&iron_pickaxe, 3, 3, iron_pickaxe_pattern);
  advanced_crafting_register_recipe(system, &iron_pickaxe);

  // Armor recipes
  Recipe iron_helmet;
  recipe_init(&iron_helmet, 300, 1); // ITEM_IRON_HELMET
  u32 helmet_pattern[] = {27, 27, 27, 27, 0, 27, 0, 0, 0};
  recipe_set_pattern(&iron_helmet, 3, 3, helmet_pattern);
  advanced_crafting_register_recipe(system, &iron_helmet);

  Recipe iron_chestplate;
  recipe_init(&iron_chestplate, 301, 1); // ITEM_IRON_CHESTPLATE
  u32 chestplate_pattern[] = {27, 0, 27, 27, 27, 27, 27, 27, 27};
  recipe_set_pattern(&iron_chestplate, 3, 3, chestplate_pattern);
  advanced_crafting_register_recipe(system, &iron_chestplate);

  // Weapon recipes
  Recipe wooden_sword;
  recipe_init(&wooden_sword, 400, 1); // ITEM_WOODEN_SWORD
  u32 sword_pattern[] = {0, 5, 0, 0, 5, 0, 0, 29, 0};
  recipe_set_pattern(&wooden_sword, 3, 3, sword_pattern);
  advanced_crafting_register_recipe(system, &wooden_sword);

  Recipe iron_sword;
  recipe_init(&iron_sword, 401, 1); // ITEM_IRON_SWORD
  u32 iron_sword_pattern[] = {0, 27, 0, 0, 27, 0, 0, 29, 0};
  recipe_set_pattern(&iron_sword, 3, 3, iron_sword_pattern);
  advanced_crafting_register_recipe(system, &iron_sword);

  // Block recipes
  Recipe planks;
  recipe_init(&planks, 500, 4);         // BLOCK_PLANKS
  recipe_add_ingredient(&planks, 5, 1); // Wood
  advanced_crafting_register_recipe(system, &planks);

  Recipe sticks;
  recipe_init(&sticks, 29, 4);            // ITEM_STICK
  recipe_add_ingredient(&sticks, 500, 2); // Planks
  advanced_crafting_register_recipe(system, &sticks);
}

// Free advanced crafting system
void advanced_crafting_free(AdvancedCraftingSystem *system) {
  if (!system)
    return;

  if (system->recipes) {
    for (u32 i = 0; i < system->recipe_count; i++) {
      recipe_free(&system->recipes[i]);
    }
    free(system->recipes);
    system->recipes = NULL;
  }

  if (system->workbenches) {
    free(system->workbenches);
    system->workbenches = NULL;
  }

  system->recipe_count = 0;
  system->recipe_capacity = 0;
  system->workbench_count = 0;
}
