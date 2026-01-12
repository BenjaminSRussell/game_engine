#include <block/block.h>
#include <crafting/recipe_system.h>
#include <stdlib.h>
#include <string.h>

// Initialize recipe registry
void recipe_registry_init(RecipeRegistry *registry) {
  if (!registry)
    return;
  memset(registry, 0, sizeof(RecipeRegistry));
  // Mutex init if needed
}

// Free recipe registry
void recipe_registry_free(RecipeRegistry *registry) {
  if (!registry)
    return;
  // Static array, nothing to free unless patterns are dynamic
  for (u32 i = 0; i < registry->recipe_count; i++) {
    if (registry->recipes[i].pattern) {
      free(registry->recipes[i].pattern);
      registry->recipes[i].pattern = NULL;
    }
  }
  registry->recipe_count = 0;
}

// Register recipe
u32 recipe_registry_add(RecipeRegistry *registry, Recipe *recipe) {
  if (!registry || !recipe || registry->recipe_count >= MAX_RECIPES) {
    return INVALID_RECIPE_ID;
  }

  u32 index = registry->recipe_count;
  registry->recipes[index] = *recipe; // Copy struct

  // If pattern is dynamic, we need to deep copy it or ensure ownership
  // transfer. Assuming ownership transfer for now or static usage. But legacy
  // code allocs pattern. We'll handle it if pattern is set.
  if (recipe->pattern) {
    registry->recipes[index].pattern = (u32 *)malloc(
        sizeof(u32) * recipe->pattern_width * recipe->pattern_height);
    memcpy(registry->recipes[index].pattern, recipe->pattern,
           sizeof(u32) * recipe->pattern_width * recipe->pattern_height);
  }

  registry->recipes[index].id = index + 1; // ID 0 is invalid usually
  registry->recipe_count++;

  return registry->recipes[index].id;
}

Recipe *recipe_registry_get(RecipeRegistry *registry, u32 recipe_id) {
  if (!registry || recipe_id == 0 || recipe_id > registry->recipe_count)
    return NULL;
  return &registry->recipes[recipe_id - 1];
}

// Initialize default recipes
void recipe_registry_add_defaults(RecipeRegistry *registry) {
  if (!registry)
    return;

  // Example: Wooden Planks from Logs
  Recipe planks;
  memset(&planks, 0, sizeof(Recipe));
  planks.type = RECIPE_TYPE_SHAPELESS;
  planks.result_item = 2; // BLOCK_PLANKS
  planks.result_quantity = 4;
  planks.ingredient_count = 1;
  planks.ingredients[0].item_id = 1; // BLOCK_LOG
  planks.ingredients[0].quantity = 1;
  recipe_registry_add(registry, &planks);

  // Example: Sticks from Planks
  Recipe sticks;
  memset(&sticks, 0, sizeof(Recipe));
  sticks.type = RECIPE_TYPE_SHAPELESS;
  sticks.result_item = 100; // ITEM_STICK
  sticks.result_quantity = 4;
  sticks.ingredient_count = 1;
  sticks.ingredients[0].item_id = 2; // BLOCK_PLANKS
  sticks.ingredients[0].quantity = 2;
  recipe_registry_add(registry, &sticks);

  // Example: Crafting Table (shaped)
  Recipe table;
  memset(&table, 0, sizeof(Recipe));
  table.type = RECIPE_TYPE_SHAPED;
  table.result_item = 3; // BLOCK_CRAFTING_TABLE
  table.result_quantity = 1;
  table.pattern_width = 2;
  table.pattern_height = 2;
  u32 table_pattern[] = {2, 2, 2, 2};
  table.pattern = table_pattern; // Temporary pointer, add() will copy
  recipe_registry_add(registry, &table);

  // Example: Bread (3 Wheat horizontal)
  Recipe bread;
  memset(&bread, 0, sizeof(Recipe));
  bread.type = RECIPE_TYPE_SHAPED;
  bread.result_item = 642; // ITEM_BREAD
  bread.result_quantity = 1;
  bread.pattern_width = 3;
  bread.pattern_height = 1;
  u32 bread_pattern[] = {50, 50, 50}; // ITEM_WHEAT
  bread.pattern = bread_pattern;
  recipe_registry_add(registry, &bread);
}
