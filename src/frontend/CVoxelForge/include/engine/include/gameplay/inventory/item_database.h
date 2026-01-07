// include/gameplay/inventory/item_database.h
//
// Purpose: Centralized item database for managing item templates. Provides
// registration, lookup, and creation of item instances from templates.
//
// Public APIs:
// - Database initialization and shutdown
// - Item registration (manual and JSON)
// - Item lookup by ID or name
// - Item instance creation
//
#ifndef ITEM_DATABASE_H
#define ITEM_DATABASE_H

#include "item.h"
#include <common.h>

// ============================================================================
// DATABASE MANAGEMENT
// ============================================================================

// Initialize item database
bool item_database_init(u32 max_items);
void item_database_shutdown(void);

// Register items
bool item_database_register(const Item *item);
bool item_database_load_from_json(const char *filepath);

// ============================================================================
// ITEM LOOKUP
// ============================================================================

// Get item by ID
const Item *item_database_get(u32 item_id);
const Item *item_database_get_by_name(const char *name);

// Query items
u32 item_database_get_items_by_type(ItemType type, const Item **out_items,
                                    u32 max_items);
u32 item_database_get_items_by_rarity(ItemRarity rarity, const Item **out_items,
                                      u32 max_items);

// Database info
u32 item_database_get_count(void);
bool item_database_exists(u32 item_id);

// ============================================================================
// ITEM CREATION
// ============================================================================

// Create item stack from template
ItemStack item_create_stack(u32 item_id, u32 quantity);
ItemStack item_create_stack_by_name(const char *name, u32 quantity);

// Clone item stack
ItemStack item_stack_clone(const ItemStack *stack);

// ============================================================================
// DEFAULT ITEMS
// ============================================================================

// Register default item set
void item_database_register_defaults(void);

#endif // ITEM_DATABASE_H
