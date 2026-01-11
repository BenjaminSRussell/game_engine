#include <crafting/recipe_manager.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 32

typedef struct {
  u64 fingerprint;
  Recipe *recipe;
  u32 age; /* simple LRU age counter */
} CacheEntry;

static Recipe **g_recipes = NULL;
static u32 g_recipe_count = 0;
static CacheEntry g_cache[CACHE_SIZE];
static u32 g_cache_age = 1;
static u32 g_cache_hits = 0;
static u32 g_cache_misses = 0;

static u64 inventory_fingerprint(const Inventory *inv) {
  u64 h = 1469598103934665603ULL; /* FNV offset basis */
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    u32 id = inv->slots[i].item_id;
    u32 count = inv->slots[i].count;
    h ^= id + (count << 16);
    h *= 1099511628211ULL; /* FNV prime */
  }
  return h;
}

int recipe_manager_init(void) {
  g_recipes = NULL;
  g_recipe_count = 0;
  for (int i = 0; i < CACHE_SIZE; i++) {
    g_cache[i].fingerprint = 0;
    g_cache[i].recipe = NULL;
    g_cache[i].age = 0;
  }
  g_cache_age = 1;
  g_cache_hits = 0;
  g_cache_misses = 0;
  return 0;
}

void recipe_manager_shutdown(void) {
  if (g_recipes)
    free(g_recipes);
  g_recipes = NULL;
  g_recipe_count = 0;
}

int recipe_manager_add(Recipe *recipe) {
  g_recipes =
      (Recipe **)realloc(g_recipes, (g_recipe_count + 1) * sizeof(Recipe *));
  if (!g_recipes)
    return -1;
  g_recipes[g_recipe_count++] = recipe;
  return 0;
}

Recipe *recipe_manager_find_matching(Inventory *inv) {
  if (!inv)
    return NULL;
  u64 fp = inventory_fingerprint(inv);

  /* Check cache first */
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (g_cache[i].recipe && g_cache[i].fingerprint == fp) {
      g_cache[i].age = g_cache_age++;
      g_cache_hits++;
      return g_cache[i].recipe;
    }
  }

  g_cache_misses++;

  /* No cache entry - search recipes */
  for (u32 r = 0; r < g_recipe_count; r++) {
    if (recipe_can_craft(g_recipes[r], inv)) {
      /* Insert into cache using LRU replacement */
      int oldest_idx = 0;
      u32 oldest_age = g_cache[0].age;
      for (int i = 1; i < CACHE_SIZE; i++) {
        if (g_cache[i].age == 0) {
          oldest_idx = i;
          break;
        }
        if (g_cache[i].age < oldest_age) {
          oldest_age = g_cache[i].age;
          oldest_idx = i;
        }
      }
      g_cache[oldest_idx].fingerprint = fp;
      g_cache[oldest_idx].recipe = g_recipes[r];
      g_cache[oldest_idx].age = g_cache_age++;
      return g_recipes[r];
    }
  }

  /* No matching recipe */
  return NULL;
}

void recipe_manager_get_cache_stats(u32 *hits, u32 *misses) {
  if (hits)
    *hits = g_cache_hits;
  if (misses)
    *misses = g_cache_misses;
}
