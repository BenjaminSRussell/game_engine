// include/world/loot_tables.h
//
// Purpose: Loot table system for generating randomized item drops in dungeons,
// chests, mob kills, and other loot sources. Supports weighted probabilities,
// rarity tiers, and conditional loot generation.
//
// Public APIs:
// - loot_table_init: Initialize the loot table system
// - loot_table_generate: Generate loot from a specific table
// - loot_table_populate_inventory: Fill an inventory with loot
// - loot_table_register_custom: Register custom loot tables
//
// Ownership: Loot tables are owned by the loot system.
// Generated items are owned by inventories/containers.
//
// Invariants:
// - Loot tables must have at least one entry
// - Total weight sum should be > 0 for weighted tables
// - Item IDs must be valid
// - Quantity ranges must be min <= max
//
#ifndef LOOT_TABLES_H
#define LOOT_TABLES_H

#include "../game_common.h"
#include "../inventory/inventory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Loot table types
typedef enum {
    LOOT_TABLE_DUNGEON_COMMON,
    LOOT_TABLE_DUNGEON_RARE,
    LOOT_TABLE_DUNGEON_BOSS,
    LOOT_TABLE_CHEST_WOODEN,
    LOOT_TABLE_CHEST_IRON,
    LOOT_TABLE_CHEST_GOLDEN,
    LOOT_TABLE_MINESHAFT,
    LOOT_TABLE_SHIPWRECK,
    LOOT_TABLE_DESERT_TEMPLE,
    LOOT_TABLE_JUNGLE_TEMPLE,
    LOOT_TABLE_NETHER_FORTRESS,
    LOOT_TABLE_END_CITY,
    LOOT_TABLE_COUNT
} LootTableType;

// Rarity tiers
typedef enum {
    LOOT_RARITY_COMMON,      // 60% base chance
    LOOT_RARITY_UNCOMMON,    // 25% base chance
    LOOT_RARITY_RARE,        // 10% base chance
    LOOT_RARITY_EPIC,        // 4% base chance
    LOOT_RARITY_LEGENDARY    // 1% base chance
} LootRarity;

// Loot entry (single item with probability)
typedef struct {
    u32 item_id;              // Item to generate
    u32 min_quantity;         // Minimum stack size
    u32 max_quantity;         // Maximum stack size
    f32 weight;               // Probability weight (higher = more common)
    LootRarity rarity;        // Rarity tier
    bool always_include;      // Always add this item
} LootEntry;

// Loot table definition
typedef struct {
    const char *name;
    const char *description;
    LootEntry *entries;
    u32 entry_count;
    u32 min_items;            // Minimum items to generate
    u32 max_items;            // Maximum items to generate
    f32 total_weight;         // Sum of all entry weights
    bool allow_duplicates;    // Can pick same item multiple times
} LootTable;

// Loot generation result
typedef struct {
    u32 item_ids[64];
    u32 quantities[64];
    u32 item_count;
    u32 total_value;          // Estimated total value
} LootResult;

// Lifecycle
void loot_tables_init(void);
void loot_tables_free(void);

// Loot generation
bool loot_table_generate(LootTableType table_type, u32 seed, LootResult *result);
bool loot_table_populate_inventory(LootTableType table_type, u32 seed, Inventory *inventory);
bool loot_table_generate_chest_loot(LootTableType table_type, u32 seed,
                                   u32 *items, u32 *quantities, u32 max_items);

// Custom tables
u32 loot_table_register_custom(const char *name, LootEntry *entries,
                               u32 entry_count, u32 min_items, u32 max_items);
bool loot_table_unregister_custom(u32 table_id);

// Queries
const LootTable *loot_table_get(LootTableType type);
const char *loot_table_get_name(LootTableType type);
u32 loot_table_get_entry_count(LootTableType type);
f32 loot_table_get_rarity_chance(LootRarity rarity);

// Utilities
u32 loot_roll_quantity(u32 min_qty, u32 max_qty, u32 *seed);
const LootEntry *loot_pick_random_entry(const LootTable *table, u32 *seed);
bool loot_should_generate_rare(LootRarity rarity, u32 *seed);

#ifdef __cplusplus
}
#endif

#endif // LOOT_TABLES_H
