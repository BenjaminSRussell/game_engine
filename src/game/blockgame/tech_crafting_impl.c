#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <tech/crafting.h>

// -----------------------------------------------------------------------------
// Tech Recipe Registry
// -----------------------------------------------------------------------------

void tech_recipe_registry_init(TechRecipeRegistry *registry, u32 capacity) {
  if (!registry)
    return;
  registry->recipes = (TechRecipe *)calloc(capacity, sizeof(TechRecipe));
  registry->capacity = capacity;
  registry->count = 0;
  LOG_INFO("Initialized TechRecipeRegistry with capacity %u", capacity);
}

void tech_recipe_registry_free(TechRecipeRegistry *registry) {
  if (!registry)
    return;
  if (registry->recipes) {
    for (u32 i = 0; i < registry->count; i++) {
      if (registry->recipes[i].ingredients) {
        free(registry->recipes[i].ingredients);
      }
    }
    free(registry->recipes);
    registry->recipes = NULL;
  }
  registry->count = 0;
  registry->capacity = 0;
}

void tech_recipe_register(TechRecipeRegistry *registry, TechRecipe recipe) {
  if (!registry || registry->count >= registry->capacity)
    return;
  // Deep copy ingredients if needed, but for now just struct copy
  // Ideally we should allocate ingredients array here.
  // Assuming ownership transfer for this simplified implementation.
  registry->recipes[registry->count++] = recipe;
}

// -----------------------------------------------------------------------------
// Technology Tree
// -----------------------------------------------------------------------------

void tech_tree_init(TechnologyTree *tree) {
  if (!tree)
    return;
  tree->current_tier = TECH_TIER_STONE;
  tree->research_points = 0;
  tree->unlocked_recipes = NULL;
  tree->unlocked_recipe_count = 0;
  for (int i = 0; i < TECH_TIER_COUNT; i++) {
    tree->tiers_unlocked[i] = (i == TECH_TIER_STONE);
  }
  LOG_INFO("Initialized TechnologyTree");
}

void tech_tree_free(TechnologyTree *tree) {
  if (!tree)
    return;
  if (tree->unlocked_recipes) {
    free(tree->unlocked_recipes);
    tree->unlocked_recipes = NULL;
  }
  tree->unlocked_recipe_count = 0;
}

bool tech_unlock_tier(TechnologyTree *tree, TechTier tier) {
  if (!tree || tier >= TECH_TIER_COUNT)
    return false;
  if (!tree->tiers_unlocked[tier]) {
    tree->tiers_unlocked[tier] = true;
    LOG_INFO("Unlocked Tech Tier: %d", tier);
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Workbench
// -----------------------------------------------------------------------------

void workbench_init(TechWorkbench *bench, TechWorkbenchType type,
                    TechTier tier) {
  if (!bench)
    return;
  memset(bench, 0, sizeof(TechWorkbench));
  bench->type = type;
  bench->tier = tier;
}

void workbench_free(TechWorkbench *bench) {
  if (!bench)
    return;
  // Free internal buffers if allocated
}
