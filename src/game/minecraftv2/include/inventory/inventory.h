// include/inventory/inventory.h
//
// Purpose: Defines the public API and data structures for a generic inventory system.
// This header provides the foundational elements for managing items within a player's
// or entity's storage, including item categorization (`ItemType`), detailed item
// properties (`ItemDefinition`), individual storage units (`InventorySlot`), and the
// overall `Inventory` structure. It also declares functions for comprehensive item
// management and inventory manipulation.
//
// Public APIs:
// - `MAX_INVENTORY_SLOTS`, `MAX_HOTBAR_SLOTS`, `STACK_SIZE_DEFAULT`: Constants defining
//   inventory dimensions and default stack limits.
// - `ItemType`: Enumeration categorizing items (e.g., Block, Tool, Weapon, Food).
// - `ItemDefinition`: Structure detailing global properties of an item, such as its
//   name, description, maximum stack size, durability, and enchantments.
// - `InventorySlot`: Structure representing a single slot in the inventory,
//   holding an `item_id`, `count` (quantity), and `durability` of the item.
// - `Inventory`: The main structure encapsulating a collection of `InventorySlot`s,
//   a selected hotbar slot, and the total count of distinct items.
// - `inventory_init`: Initializes an `Inventory` instance, setting all slots to empty.
// - `inventory_free`: Frees resources associated with the `Inventory`.
// - `inventory_add_item`, `inventory_remove_item`, `inventory_has_item`: Functions for
//   manipulating items within the inventory.
// - `inventory_get_item_count`: Returns the total quantity of a specific item in the inventory.
// - `inventory_get_slot`, `inventory_set_slot`: Functions to retrieve or modify items in specific slots.
// - `inventory_select_hotbar`, `inventory_get_selected_item`, `inventory_get_selected_slot`:
//   Functions specific to hotbar interaction within the inventory.
// - `inventory_is_full`, `inventory_get_empty_slots`: Queries about inventory capacity.
// - `inventory_sort`, `inventory_compact`: Utility functions for organizing inventory contents.
// - `inventory_clear`: Empties the entire inventory.
//
// Ownership: An `Inventory` instance owns its array of `InventorySlot`s. `ItemDefinition`s
// are global data, not owned by individual `Inventory` instances.
//
// Invariants:
// - An `Inventory` must be initialized with `inventory_init` before use.
// - `item_id` values must correspond to valid `ItemDefinition`s.
// - `count` in `InventorySlot` should not exceed `max_stack_size` for that item type.
// - `slot_index` parameters must be within `MAX_INVENTORY_SLOTS`.
//
#ifndef INVENTORY_H
#define INVENTORY_H


#include "../game_common.h"
#include "../block/block.h"

#define MAX_INVENTORY_SLOTS 36
#define MAX_HOTBAR_SLOTS 9
#define STACK_SIZE_DEFAULT 64
#define STACK_SIZE_SMALL 16
#define STACK_SIZE_LARGE 1

typedef enum {
    ITEM_TYPE_BLOCK,
    ITEM_TYPE_TOOL,
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_FOOD,
    ITEM_TYPE_MATERIAL,
    ITEM_TYPE_ARMOR,
    ITEM_TYPE_MISC
} ItemType;

typedef enum {
    SORT_BY_NAME,
    SORT_BY_TYPE,
    SORT_BY_QUANTITY,
    SORT_BY_ID,
    SORT_BY_DURABILITY
} InventorySortType;

typedef struct {
    BlockID block_id;
    ItemType item_type;
    char name[64];
    char description[256];
    u16 max_stack_size;
    f32 durability;
    f32 max_durability;
    u32 enchantments;
} ItemDefinition;

typedef struct {
    u32 item_id;
    u16 count;
    f32 durability;
    // Enchantments (stored as void* to avoid circular dependency with enchanting.h)
    // In enchanting.c, cast to Enchantment* when accessing
    void *enchantments;            // Array of enchantments on this item
    u32 enchantment_count;         // Number of enchantments
    u32 enchantment_capacity;      // Capacity for enchantments
    bool is_favorite;              // Mark as favorite for quick access
    bool is_locked;                // Prevent accidental movement
} InventorySlot;

typedef enum {
    INVENTORY_EVENT_ADD,
    INVENTORY_EVENT_REMOVE,
    INVENTORY_EVENT_SET_SLOT,
    INVENTORY_EVENT_CLEAR,
    INVENTORY_EVENT_FULL,
    INVENTORY_EVENT_SORTED,
    INVENTORY_EVENT_COMPACTED
} InventoryEventType;

typedef struct Inventory Inventory;

typedef void (*InventoryEventCallback)(Inventory *inv,
                                       InventoryEventType event, u32 item_id,
                                       u16 count, u32 slot_index,
                                       void *user_data);

struct Inventory {
    InventorySlot slots[MAX_INVENTORY_SLOTS];
    u32 selected_hotbar;
    u32 total_items;
    bool dirty;
    InventoryEventCallback on_event;
    void *user_data;
};

// TODO: Initialize all slots to empty, reset counters, and clear dirty flags.
void inventory_init(Inventory *inv);
// TODO: Free any dynamically allocated memory (e.g., custom item data).
void inventory_free(Inventory *inv);

// TODO: Check for existing stacks to merge into first, then find empty slot.
bool inventory_add_item(Inventory *inv, u32 item_id, u16 count);
// TODO: Validate item presence and decrease count; remove slot if count becomes 0.
bool inventory_remove_item(Inventory *inv, u32 item_id, u16 count);
// TODO: Iterate slots to check for item presence.
bool inventory_has_item(Inventory *inv, u32 item_id, u16 count);

u16 inventory_get_item_count(Inventory *inv, u32 item_id);
bool inventory_get_slot(Inventory *inv, u32 slot_index, InventorySlot *out_slot);
bool inventory_set_slot(Inventory *inv, u32 slot_index, u32 item_id, u16 count);

void inventory_select_hotbar(Inventory *inv, u32 index);
u32 inventory_get_selected_item(Inventory *inv);
InventorySlot inventory_get_selected_slot(Inventory *inv);

bool inventory_is_full(Inventory *inv);
u32 inventory_get_empty_slots(Inventory *inv);
void inventory_sort(Inventory *inv);
// TODO: Simulate adding an item to see how much would fit without modifying inventory.
// Returns the amount that *could* be added.
u16 inventory_simulate_add(Inventory *inv, u32 item_id, u16 count);
void inventory_sort_by_type(Inventory *inv, InventorySortType sort_type);
// TODO: Iterate inventory, finding partial stacks of same item_id and merging them.
void inventory_compact(Inventory *inv);

void inventory_clear(Inventory *inv);

// Inventory auto-sort system with configurable sort criteria
// TODO: Implement sorting algorithm (e.g., stable sort) based on SortType criteria.
// TODO: Trigger INVENTORY_EVENT_SORTED callback.
void inventory_auto_sort(Inventory *inv, InventorySortType sort_type);

// Inventory quick-stack functionality for chests and containers
// TODO: Iterate through 'inv', find matching stacks in 'target', and transfer items.
// TODO: Handle partial transfers if target stacks fill up.
u32 inventory_quick_stack(Inventory *inv, Inventory *target);

// Inventory search and filter system for large inventories
typedef struct {
  u32 item_id;
  u16 count;
  u32 slot_index;
} InventorySearchResult;

// TODO: Implement search logic (linear scan or cached index lookup).
u32 inventory_search_items(const Inventory *inv, u32 item_id, 
                           InventorySearchResult *results, u32 max_results);

// Inventory item stacking optimization to reduce fragmentation
// TODO: Scan inventory for split stacks of the same item and merge them.
void inventory_optimize_stacking(Inventory *inv);

// Advanced stacking and management functions
u16 inventory_get_max_stack_size(u32 item_id);
bool inventory_can_stack_items(u32 item_id1, u32 item_id2);
bool inventory_merge_stacks(Inventory *inv, u32 source_slot, u32 target_slot);
// TODO: Verify source has enough items, find/create target slot, transfer specific amount.
bool inventory_split_stack(Inventory *inv, u32 source_slot, u32 target_slot, u16 amount);
u32 inventory_find_best_stack_slot(Inventory *inv, u32 item_id, u16 count);
void inventory_auto_stack(Inventory *inv);

// Item management functions
// TODO: Handle swapping if target is occupied; merge if items are compatible.
bool inventory_move_item(Inventory *inv, u32 source_slot, u32 target_slot);
// TODO: atomic swap of two slots.
bool inventory_swap_items(Inventory *inv, u32 slot1, u32 slot2);
void inventory_set_favorite(Inventory *inv, u32 slot_index, bool favorite);
bool inventory_is_favorite(Inventory *inv, u32 slot_index);
void inventory_set_locked(Inventory *inv, u32 slot_index, bool locked);
bool inventory_is_locked(Inventory *inv, u32 slot_index);

// Search and filter functions
u32 inventory_find_first_item(Inventory *inv, u32 item_id);
u32 inventory_find_empty_slot(Inventory *inv);
u32 inventory_count_items_of_type(Inventory *inv, ItemType type);
bool inventory_has_space_for(Inventory *inv, u32 item_id, u16 count);

// Utility functions
void inventory_debug_print(Inventory *inv);
// TODO: Debug check: ensure item_count > 0 implies item_id != 0 and vice versa.
bool inventory_validate(Inventory *inv);

// ============= Persistence & Networking =============

// Serialization
// TODO: Implement JSON or binary serialization for saving player data/chest contents.
bool inventory_serialize(const Inventory *inv, char *buffer, u32 buffer_size);
bool inventory_deserialize(Inventory *inv, const char *buffer, u32 buffer_size);

// TODO: Add logic to generate a dirty mask or packet for network sync.

#endif
