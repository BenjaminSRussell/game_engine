// item_db.c - Item Database Implementation
// Central registry for item definitions with lookup by ID and name

#include "include/gameplay/inventory/item_database.h"
#include "include/core/json.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <common.h>

// ============================================================================
// INTERNAL DATA STRUCTURES
// ============================================================================

typedef struct {
  u32 item_id;
  Item item;
} ItemEntry;

typedef struct {
  ItemEntry *entries;
  u32 count;
  u32 capacity;
} ItemDatabase;

static ItemDatabase g_item_database = {0};

// Simple hash table for name-based lookups
typedef struct {
  char name[64];
  u32 item_id;
} NameEntry;

typedef struct {
  NameEntry *entries;
  u32 count;
  u32 capacity;
} NameIndex;

static NameIndex g_name_index = {0};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static u32 name_hash(const char *name) {
  u32 hash = 5381;
  for (const char *p = name; *p; p++) {
    hash = ((hash << 5) + hash) ^ (*p);
  }
  return hash;
}

static bool name_index_add(const char *name, u32 item_id) {
  if (g_name_index.count >= g_name_index.capacity) {
    g_name_index.capacity = g_name_index.capacity * 2 + 10;
    NameEntry *new_entries =
        (NameEntry *)realloc(g_name_index.entries,
                             sizeof(NameEntry) * g_name_index.capacity);
    if (!new_entries)
      return false;
    g_name_index.entries = new_entries;
  }

  NameEntry *entry = &g_name_index.entries[g_name_index.count++];
  strncpy(entry->name, name, sizeof(entry->name) - 1);
  entry->name[sizeof(entry->name) - 1] = '\0';
  entry->item_id = item_id;

  return true;
}

// ============================================================================
// PUBLIC API - DATABASE MANAGEMENT
// ============================================================================

bool item_database_init(u32 max_items) {
  if (g_item_database.entries != NULL) {
    LOG_WARN("Item database already initialized");
    return false;
  }

  g_item_database.capacity = max_items > 0 ? max_items : 1024;
  g_item_database.entries =
      (ItemEntry *)malloc(sizeof(ItemEntry) * g_item_database.capacity);
  if (!g_item_database.entries) {
    LOG_ERROR("Failed to allocate item database memory");
    return false;
  }

  g_item_database.count = 0;

  // Initialize name index
  g_name_index.capacity = 128;
  g_name_index.entries =
      (NameEntry *)malloc(sizeof(NameEntry) * g_name_index.capacity);
  if (!g_name_index.entries) {
    LOG_ERROR("Failed to allocate name index memory");
    free(g_item_database.entries);
    g_item_database.entries = NULL;
    return false;
  }
  g_name_index.count = 0;

  LOG_INFO("Item database initialized with capacity %u", g_item_database.capacity);
  return true;
}

void item_database_shutdown(void) {
  if (g_item_database.entries) {
    free(g_item_database.entries);
    g_item_database.entries = NULL;
  }
  g_item_database.count = 0;
  g_item_database.capacity = 0;

  if (g_name_index.entries) {
    free(g_name_index.entries);
    g_name_index.entries = NULL;
  }
  g_name_index.count = 0;
  g_name_index.capacity = 0;

  LOG_INFO("Item database shutdown");
}

bool item_database_register(const Item *item) {
  if (!item || !g_item_database.entries) {
    LOG_WARN("Invalid item or database not initialized");
    return false;
  }

  // Check for duplicate
  if (item_database_exists(item->id)) {
    LOG_WARN("Item ID %u already registered", item->id);
    return false;
  }

  // Ensure capacity
  if (g_item_database.count >= g_item_database.capacity) {
    LOG_WARN("Item database full, capacity %u", g_item_database.capacity);
    return false;
  }

  // Add to database
  ItemEntry *entry = &g_item_database.entries[g_item_database.count++];
  memcpy(&entry->item, item, sizeof(Item));
  entry->item_id = item->id;

  // Add to name index
  if (!name_index_add(item->name, item->id)) {
    LOG_WARN("Failed to add item %s to name index", item->name);
    g_item_database.count--;
    return false;
  }

  LOG_DEBUG("Registered item: %u - %s", item->id, item->name);
  return true;
}

bool item_database_load_from_json(const char *filepath) {
  if (!filepath || !g_item_database.entries) {
    LOG_WARN("Invalid filepath or database not initialized");
    return false;
  }

  // TODO: Implement JSON loading when needed
  // For now, this is a placeholder that logs a message
  LOG_INFO("JSON item loading not yet implemented: %s", filepath);
  return true;
}

// ============================================================================
// PUBLIC API - ITEM LOOKUP
// ============================================================================

const Item *item_database_get(u32 item_id) {
  if (!g_item_database.entries)
    return NULL;

  for (u32 i = 0; i < g_item_database.count; i++) {
    if (g_item_database.entries[i].item_id == item_id) {
      return &g_item_database.entries[i].item;
    }
  }

  LOG_DEBUG("Item ID %u not found in database", item_id);
  return NULL;
}

const Item *item_database_get_by_name(const char *name) {
  if (!name || !g_name_index.entries)
    return NULL;

  for (u32 i = 0; i < g_name_index.count; i++) {
    if (strcmp(g_name_index.entries[i].name, name) == 0) {
      return item_database_get(g_name_index.entries[i].item_id);
    }
  }

  LOG_DEBUG("Item name '%s' not found in database", name);
  return NULL;
}

u32 item_database_get_max_stack(u32 item_id) {
  const Item *item = item_database_get(item_id);
  if (!item)
    return 1;
  return item->max_stack_size;
}

f32 item_database_get_weight(u32 item_id) {
  const Item *item = item_database_get(item_id);
  if (!item)
    return 0.0f;
  return item->weight;
}

u32 item_database_get_value(u32 item_id) {
  const Item *item = item_database_get(item_id);
  if (!item)
    return 0;
  return item->sell_value;
}

u32 item_database_get_items_by_type(ItemType type, const Item **out_items,
                                    u32 max_items) {
  if (!out_items || !g_item_database.entries)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < g_item_database.count && count < max_items; i++) {
    if (g_item_database.entries[i].item.type == type) {
      out_items[count++] = &g_item_database.entries[i].item;
    }
  }

  return count;
}

u32 item_database_get_items_by_rarity(ItemRarity rarity, const Item **out_items,
                                      u32 max_items) {
  if (!out_items || !g_item_database.entries)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < g_item_database.count && count < max_items; i++) {
    if (g_item_database.entries[i].item.rarity == rarity) {
      out_items[count++] = &g_item_database.entries[i].item;
    }
  }

  return count;
}

u32 item_database_get_count(void) { return g_item_database.count; }

bool item_database_exists(u32 item_id) {
  return item_database_get(item_id) != NULL;
}

// ============================================================================
// PUBLIC API - ITEM CREATION
// ============================================================================

ItemStack item_create_stack(u32 item_id, u32 quantity) {
  ItemStack stack = {0};

  const Item *item = item_database_get(item_id);
  if (!item) {
    LOG_WARN("Cannot create stack for unknown item ID %u", item_id);
    return stack;
  }

  stack.item_id = item_id;
  stack.quantity = (quantity > 0) ? quantity : 1;
  stack.max_quantity = item->max_stack_size;
  stack.durability = item->max_durability;

  return stack;
}

ItemStack item_create_stack_by_name(const char *name, u32 quantity) {
  ItemStack stack = {0};

  const Item *item = item_database_get_by_name(name);
  if (!item) {
    LOG_WARN("Cannot create stack for unknown item name '%s'", name);
    return stack;
  }

  return item_create_stack(item->id, quantity);
}

ItemStack item_stack_clone(const ItemStack *stack) {
  ItemStack cloned = {0};

  if (!stack)
    return cloned;

  memcpy(&cloned, stack, sizeof(ItemStack));
  return cloned;
}

// ============================================================================
// PUBLIC API - DEFAULT ITEMS
// ============================================================================

void item_database_register_defaults(void) {
  // This function delegates to the game-specific item registry
  // The game layer (src/game/blockgame/) provides the actual default items
  // through its item_registry system

  LOG_INFO("Default items registration delegated to game layer item registry");
}
