// Recipe registry storage and lookup.
#include <block/block.h>
#include <crafting/recipe.h>
#include <crafting/recipe_manager.h>
#include <crafting/recipe_registry.h>
#include <stdlib.h>
#include <string.h>

// Initialize recipe registry
void recipe_registry_init(RecipeRegistry *registry, u32 capacity) {
  registry->recipes = (Recipe *)calloc(capacity, sizeof(Recipe));
  registry->capacity = capacity;
  registry->count = 0;
  recipe_manager_init();
}

// Free recipe registry
void recipe_registry_free(RecipeRegistry *registry) {
  if (!registry)
    return;

  for (u32 i = 0; i < registry->count; i++) {
    recipe_free(&registry->recipes[i]);
  }

  if (registry->recipes) {
    free(registry->recipes);
    registry->recipes = NULL;
  }

  registry->capacity = 0;
  registry->count = 0;
  recipe_manager_shutdown();
}

// Register recipe
bool recipe_registry_register(RecipeRegistry *registry, Recipe *recipe) {
  if (!registry || !recipe || registry->count >= registry->capacity) {
    return false;
  }

  // Copy recipe
  u32 index = registry->count;
  memcpy(&registry->recipes[index], recipe, sizeof(Recipe));
  registry->count++;
  recipe_manager_add(&registry->recipes[index]);

  return true;
}

// Find recipe by result
Recipe *recipe_registry_find_by_result(RecipeRegistry *registry, u32 item_id) {
  if (!registry)
    return NULL;

  for (u32 i = 0; i < registry->count; i++) {
    if (registry->recipes[i].result_item == item_id) {
      return &registry->recipes[i];
    }
  }

  return NULL;
}

// Initialize default recipes
void recipe_registry_init_defaults(RecipeRegistry *registry) {
  if (!registry)
    return;

  // Example: Wooden Planks from Logs
  Recipe planks_recipe;
  recipe_init(&planks_recipe, 2, 4);           // Assuming BLOCK_PLANKS = 2
  recipe_add_ingredient(&planks_recipe, 1, 1); // Assuming BLOCK_LOG = 1
  recipe_registry_register(registry, &planks_recipe);

  // Example: Sticks from Planks (shapeless)
  Recipe sticks_recipe;
  recipe_init(&sticks_recipe, 100, 4);         // Assuming ITEM_STICK = 100
  recipe_add_ingredient(&sticks_recipe, 2, 2); // Assuming BLOCK_PLANKS = 2
  recipe_registry_register(registry, &sticks_recipe);

  // Example: Crafting Table (shaped)
  Recipe crafting_table_recipe;
  recipe_init(&crafting_table_recipe, 3,
              1);        // Assuming BLOCK_CRAFTING_TABLE = 3
  u32 pattern[] = {2, 2, // Assuming BLOCK_PLANKS = 2
                   2, 2};
  recipe_set_pattern(&crafting_table_recipe, 2, 2, pattern);
  recipe_registry_register(registry, &crafting_table_recipe);
}
