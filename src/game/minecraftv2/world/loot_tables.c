// Loot table system implementation for randomized item generation.
// Roadmap: docs/WORLD_ROADMAP.md
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/loot_tables.h>

// Forward declarations
static void loot_tables_initialize_defaults(void);
static u32 loot_random(u32 *seed);
static f32 loot_random_f32(u32 *seed);

// Dungeon common loot entries
static LootEntry g_dungeon_common_loot[] = {
    {.item_id = 100,
     .min_quantity = 1,
     .max_quantity = 3,
     .weight = 30.0f,
     .rarity = LOOT_RARITY_COMMON}, // Bread
    {.item_id = 101,
     .min_quantity = 1,
     .max_quantity = 5,
     .weight = 25.0f,
     .rarity = LOOT_RARITY_COMMON}, // Arrow
    {.item_id = 102,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 20.0f,
     .rarity = LOOT_RARITY_COMMON}, // Torch
    {.item_id = 103,
     .min_quantity = 2,
     .max_quantity = 8,
     .weight = 15.0f,
     .rarity = LOOT_RARITY_UNCOMMON}, // Gold coin
    {.item_id = 104,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 8.0f,
     .rarity = LOOT_RARITY_RARE}, // Iron sword
    {.item_id = 105,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 2.0f,
     .rarity = LOOT_RARITY_EPIC}, // Diamond
};

// Dungeon rare loot entries
static LootEntry g_dungeon_rare_loot[] = {
    {.item_id = 200,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 20.0f,
     .rarity = LOOT_RARITY_RARE}, // Enchanted sword
    {.item_id = 201,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 18.0f,
     .rarity = LOOT_RARITY_RARE}, // Enchanted armor
    {.item_id = 202,
     .min_quantity = 5,
     .max_quantity = 15,
     .weight = 15.0f,
     .rarity = LOOT_RARITY_UNCOMMON}, // Gems
    {.item_id = 203,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 12.0f,
     .rarity = LOOT_RARITY_EPIC}, // Magic scroll
    {.item_id = 204,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 5.0f,
     .rarity = LOOT_RARITY_LEGENDARY}, // Artifact
};

// Dungeon boss loot entries
static LootEntry g_dungeon_boss_loot[] = {
    {.item_id = 300,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 100.0f,
     .rarity = LOOT_RARITY_LEGENDARY,
     .always_include = true}, // Boss key
    {.item_id = 301,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 40.0f,
     .rarity = LOOT_RARITY_EPIC}, // Legendary weapon
    {.item_id = 302,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 35.0f,
     .rarity = LOOT_RARITY_EPIC}, // Legendary armor
    {.item_id = 303,
     .min_quantity = 10,
     .max_quantity = 50,
     .weight = 25.0f,
     .rarity = LOOT_RARITY_RARE}, // Gold bars
};

// Wooden chest loot
static LootEntry g_chest_wooden_loot[] = {
    {.item_id = 100,
     .min_quantity = 1,
     .max_quantity = 5,
     .weight = 40.0f,
     .rarity = LOOT_RARITY_COMMON}, // Bread
    {.item_id = 102,
     .min_quantity = 2,
     .max_quantity = 8,
     .weight = 35.0f,
     .rarity = LOOT_RARITY_COMMON}, // Torch
    {.item_id = 103,
     .min_quantity = 1,
     .max_quantity = 3,
     .weight = 20.0f,
     .rarity = LOOT_RARITY_UNCOMMON}, // Gold coin
    {.item_id = 400,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 5.0f,
     .rarity = LOOT_RARITY_RARE}, // Map fragment
};

// Iron chest loot
static LootEntry g_chest_iron_loot[] = {
    {.item_id = 103,
     .min_quantity = 5,
     .max_quantity = 15,
     .weight = 30.0f,
     .rarity = LOOT_RARITY_UNCOMMON}, // Gold coin
    {.item_id = 104,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 25.0f,
     .rarity = LOOT_RARITY_RARE}, // Iron sword
    {.item_id = 202,
     .min_quantity = 2,
     .max_quantity = 8,
     .weight = 20.0f,
     .rarity = LOOT_RARITY_UNCOMMON}, // Gems
    {.item_id = 203,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 15.0f,
     .rarity = LOOT_RARITY_EPIC}, // Magic scroll
    {.item_id = 105,
     .min_quantity = 1,
     .max_quantity = 3,
     .weight = 10.0f,
     .rarity = LOOT_RARITY_EPIC}, // Diamond
};

// Golden chest loot
static LootEntry g_chest_golden_loot[] = {
    {.item_id = 203,
     .min_quantity = 1,
     .max_quantity = 2,
     .weight = 30.0f,
     .rarity = LOOT_RARITY_EPIC}, // Magic scroll
    {.item_id = 105,
     .min_quantity = 2,
     .max_quantity = 5,
     .weight = 25.0f,
     .rarity = LOOT_RARITY_EPIC}, // Diamond
    {.item_id = 301,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 20.0f,
     .rarity = LOOT_RARITY_LEGENDARY}, // Legendary weapon
    {.item_id = 302,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 15.0f,
     .rarity = LOOT_RARITY_LEGENDARY}, // Legendary armor
    {.item_id = 500,
     .min_quantity = 1,
     .max_quantity = 1,
     .weight = 10.0f,
     .rarity = LOOT_RARITY_LEGENDARY}, // Unique artifact
};

// Loot tables array
static LootTable g_loot_tables[LOOT_TABLE_COUNT];
static bool g_initialized = false;

static u32 loot_random(u32 *seed) {
  *seed = *seed * 1103515245 + 12345;
  return *seed;
}

static f32 loot_random_f32(u32 *seed) {
  return (f32)loot_random(seed) / (f32)UINT32_MAX;
}

static void loot_tables_initialize_defaults(void) {
  // Dungeon common
  g_loot_tables[LOOT_TABLE_DUNGEON_COMMON] = (LootTable){
      .name = "Dungeon Common",
      .description = "Common dungeon chest loot",
      .entries = g_dungeon_common_loot,
      .entry_count = sizeof(g_dungeon_common_loot) / sizeof(LootEntry),
      .min_items = 2,
      .max_items = 5,
      .allow_duplicates = true};

  // Dungeon rare
  g_loot_tables[LOOT_TABLE_DUNGEON_RARE] = (LootTable){
      .name = "Dungeon Rare",
      .description = "Rare dungeon chest loot",
      .entries = g_dungeon_rare_loot,
      .entry_count = sizeof(g_dungeon_rare_loot) / sizeof(LootEntry),
      .min_items = 1,
      .max_items = 3,
      .allow_duplicates = false};

  // Dungeon boss
  g_loot_tables[LOOT_TABLE_DUNGEON_BOSS] = (LootTable){
      .name = "Dungeon Boss",
      .description = "Boss encounter loot",
      .entries = g_dungeon_boss_loot,
      .entry_count = sizeof(g_dungeon_boss_loot) / sizeof(LootEntry),
      .min_items = 2,
      .max_items = 4,
      .allow_duplicates = false};

  // Wooden chest
  g_loot_tables[LOOT_TABLE_CHEST_WOODEN] = (LootTable){
      .name = "Wooden Chest",
      .description = "Basic wooden chest",
      .entries = g_chest_wooden_loot,
      .entry_count = sizeof(g_chest_wooden_loot) / sizeof(LootEntry),
      .min_items = 1,
      .max_items = 3,
      .allow_duplicates = true};

  // Iron chest
  g_loot_tables[LOOT_TABLE_CHEST_IRON] =
      (LootTable){.name = "Iron Chest",
                  .description = "Reinforced iron chest",
                  .entries = g_chest_iron_loot,
                  .entry_count = sizeof(g_chest_iron_loot) / sizeof(LootEntry),
                  .min_items = 2,
                  .max_items = 4,
                  .allow_duplicates = true};

  // Golden chest
  g_loot_tables[LOOT_TABLE_CHEST_GOLDEN] = (LootTable){
      .name = "Golden Chest",
      .description = "Precious golden chest",
      .entries = g_chest_golden_loot,
      .entry_count = sizeof(g_chest_golden_loot) / sizeof(LootEntry),
      .min_items = 2,
      .max_items = 5,
      .allow_duplicates = false};

  // Calculate total weights
  for (u32 i = 0; i < LOOT_TABLE_COUNT; i++) {
    LootTable *table = &g_loot_tables[i];
    if (table->entries) {
      table->total_weight = 0.0f;
      for (u32 j = 0; j < table->entry_count; j++) {
        table->total_weight += table->entries[j].weight;
      }
    }
  }
}

void loot_tables_init(void) {
  if (g_initialized)
    return;

  memset(g_loot_tables, 0, sizeof(g_loot_tables));
  loot_tables_initialize_defaults();

  g_initialized = true;
  LOG_INFO("Loot tables initialized with %d tables", LOOT_TABLE_COUNT);
}

void loot_tables_free(void) {
  g_initialized = false;
  LOG_INFO("Loot tables freed");
}

bool loot_table_generate(LootTableType table_type, u32 seed,
                         LootResult *result) {
  if (!g_initialized || table_type >= LOOT_TABLE_COUNT || !result) {
    return false;
  }

  const LootTable *table = &g_loot_tables[table_type];
  if (!table->entries || table->entry_count == 0) {
    LOG_WARN("Loot table %d has no entries", table_type);
    return false;
  }

  memset(result, 0, sizeof(LootResult));

  u32 local_seed = seed;

  // Determine number of items to generate
  u32 num_items = table->min_items;
  if (table->max_items > table->min_items) {
    num_items +=
        loot_random(&local_seed) % (table->max_items - table->min_items + 1);
  }

  // Generate items
  bool used_entries[64] = {0}; // Track used entries if duplicates not allowed

  // First, add always_include items
  for (u32 i = 0; i < table->entry_count && result->item_count < 64; i++) {
    const LootEntry *entry = &table->entries[i];
    if (entry->always_include) {
      u32 quantity = loot_roll_quantity(entry->min_quantity,
                                        entry->max_quantity, &local_seed);
      result->item_ids[result->item_count] = entry->item_id;
      result->quantities[result->item_count] = quantity;
      result->item_count++;
      used_entries[i] = true;
    }
  }

  // Then add random items
  for (u32 i = 0; i < num_items && result->item_count < 64; i++) {
    const LootEntry *entry = loot_pick_random_entry(table, &local_seed);
    if (!entry)
      continue;

    // Check if already used (if duplicates not allowed)
    if (!table->allow_duplicates) {
      u32 entry_idx = (u32)(entry - table->entries);
      if (used_entries[entry_idx]) {
        i--; // Try again
        continue;
      }
      used_entries[entry_idx] = true;
    }

    // Check rarity
    if (!loot_should_generate_rare(entry->rarity, &local_seed)) {
      i--; // Try again
      continue;
    }

    u32 quantity = loot_roll_quantity(entry->min_quantity, entry->max_quantity,
                                      &local_seed);
    result->item_ids[result->item_count] = entry->item_id;
    result->quantities[result->item_count] = quantity;
    result->item_count++;
  }

  LOG_DEBUG("Generated %d items from loot table '%s' (seed: %u)",
            result->item_count, table->name, seed);

  return true;
}

bool loot_table_populate_inventory(LootTableType table_type, u32 seed,
                                   Inventory *inventory) {
  if (!inventory)
    return false;

  LootResult result;
  if (!loot_table_generate(table_type, seed, &result)) {
    return false;
  }

  // Add items to inventory
  for (u32 i = 0; i < result.item_count; i++) {
    // Note: Would call inventory_add_item with result.item_ids[i] and
    // result.quantities[i]
    LOG_TRACE("Adding loot: item_id=%u, quantity=%u", result.item_ids[i],
              result.quantities[i]);
  }

  return true;
}

bool loot_table_generate_chest_loot(LootTableType table_type, u32 seed,
                                    u32 *items, u32 *quantities,
                                    u32 max_items) {
  if (!items || !quantities)
    return false;

  LootResult result;
  if (!loot_table_generate(table_type, seed, &result)) {
    return false;
  }

  u32 count = result.item_count < max_items ? result.item_count : max_items;
  for (u32 i = 0; i < count; i++) {
    items[i] = result.item_ids[i];
    quantities[i] = result.quantities[i];
  }

  return true;
}

const LootTable *loot_table_get(LootTableType type) {
  if (!g_initialized || type >= LOOT_TABLE_COUNT)
    return NULL;
  return &g_loot_tables[type];
}

const char *loot_table_get_name(LootTableType type) {
  const LootTable *table = loot_table_get(type);
  return table ? table->name : "Unknown";
}

u32 loot_table_get_entry_count(LootTableType type) {
  const LootTable *table = loot_table_get(type);
  return table ? table->entry_count : 0;
}

f32 loot_table_get_rarity_chance(LootRarity rarity) {
  switch (rarity) {
  case LOOT_RARITY_COMMON:
    return 0.6f; // 60%
  case LOOT_RARITY_UNCOMMON:
    return 0.25f; // 25%
  case LOOT_RARITY_RARE:
    return 0.1f; // 10%
  case LOOT_RARITY_EPIC:
    return 0.04f; // 4%
  case LOOT_RARITY_LEGENDARY:
    return 0.01f; // 1%
  default:
    return 0.0f;
  }
}

u32 loot_roll_quantity(u32 min_qty, u32 max_qty, u32 *seed) {
  if (min_qty >= max_qty)
    return min_qty;
  return min_qty + (loot_random(seed) % (max_qty - min_qty + 1));
}

const LootEntry *loot_pick_random_entry(const LootTable *table, u32 *seed) {
  if (!table || !table->entries || table->entry_count == 0)
    return NULL;

  // Weighted random selection
  f32 roll = loot_random_f32(seed) * table->total_weight;
  f32 cumulative_weight = 0.0f;

  for (u32 i = 0; i < table->entry_count; i++) {
    cumulative_weight += table->entries[i].weight;
    if (roll <= cumulative_weight) {
      return &table->entries[i];
    }
  }

  // Fallback to last entry
  return &table->entries[table->entry_count - 1];
}

bool loot_should_generate_rare(LootRarity rarity, u32 *seed) {
  f32 chance = loot_table_get_rarity_chance(rarity);
  f32 roll = loot_random_f32(seed);
  return roll <= chance;
}

u32 loot_table_register_custom(const char *name, LootEntry *entries,
                               u32 entry_count, u32 min_items, u32 max_items) {
  (void)name;
  (void)entries;
  (void)entry_count;
  (void)min_items;
  (void)max_items;
  LOG_WARN("Custom loot table registration not yet implemented");
  return 0;
}

bool loot_table_unregister_custom(u32 table_id) {
  (void)table_id;
  return false;
}
