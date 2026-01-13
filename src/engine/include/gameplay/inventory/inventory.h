// include/gameplay/inventory/inventory.h
//
// Purpose: Container and inventory management system. Provides slot-based
// storage with support for stacking, sorting, and capacity limits.
//
// Public APIs:
// - Container: Slot-based storage structure
// - InventoryComponent: ECS component for entities with inventories
// - Slot operations (add, remove, swap, split)
//
#ifndef INVENTORY_H
#define INVENTORY_H

#include "include/gameplay/inventory/item.h"
#include "engine/include/common.h"
#include <ecs/ecs.h>

// Container for items (inventory, chest, etc.)
typedef struct {
  ItemStack *slots;   // Array of item slots
  u32 capacity;       // Number of slots
  f32 max_weight;     // Maximum weight capacity (0 = unlimited)
  f32 current_weight; // Current total weight
  Entity owner;       // Entity that owns this container
  const char *name;   // Container name (optional)
} Container;

// Inventory component for ECS
typedef struct {
  Container *main_inventory; // Main inventory
  Container *quick_slots;    // Quick access slots (hotbar)
  u32 gold;                  // Currency
} InventoryComponent;

// ============================================================================
// CONTAINER MANAGEMENT
// ============================================================================

// Create and destroy containers
Container *container_create(u32 capacity, f32 max_weight);
void container_destroy(Container *container);

// Initialize container with existing allocation
void container_init(Container *container, u32 capacity, f32 max_weight);
void container_cleanup(Container *container);

// ============================================================================
// ITEM OPERATIONS
// ============================================================================

// Add items to container
bool container_add_item(Container *container, ItemStack item, u32 *out_slot);
bool container_add_item_to_slot(Container *container, u32 slot, ItemStack item);

// Remove items from container
bool container_remove_item(Container *container, u32 slot, u32 quantity,
                           ItemStack *out_item);
bool container_remove_item_by_id(Container *container, u32 item_id,
                                 u32 quantity);

// Swap items between slots
bool container_swap_slots(Container *container, u32 slot_a, u32 slot_b);
bool container_transfer(Container *src, u32 src_slot, Container *dest,
                        u32 dest_slot);

// Split stack
bool container_split_stack(Container *container, u32 slot, u32 amount,
                           u32 *out_new_slot);

// ============================================================================
// QUERIES
// ============================================================================

// Check if container has item
bool container_has_item(const Container *container, u32 item_id, u32 quantity);
u32 container_count_item(const Container *container, u32 item_id);

// Find items
i32 container_find_item(const Container *container, u32 item_id);
i32 container_find_empty_slot(const Container *container);
i32 container_find_stackable_slot(const Container *container, u32 item_id);

// Container state
bool container_is_full(const Container *container);
bool container_is_empty(const Container *container);
u32 container_get_used_slots(const Container *container);
f32 container_get_weight(const Container *container);

// ============================================================================
// SORTING & ORGANIZATION
// ============================================================================

// Sort container
void container_sort_by_type(Container *container);
void container_sort_by_rarity(Container *container);
void container_sort_by_value(Container *container);

// Stack consolidation
void container_consolidate_stacks(Container *container);

// Clear container
void container_clear(Container *container);

// ============================================================================
// INVENTORY COMPONENT
// ============================================================================

// Create inventory component
InventoryComponent inventory_component_create(u32 main_capacity,
                                              u32 quickslot_count);
void inventory_component_destroy(InventoryComponent *inventory);

// Access containers
Container *inventory_get_main(InventoryComponent *inventory);
Container *inventory_get_quickslots(InventoryComponent *inventory);

// Currency operations
void inventory_add_gold(InventoryComponent *inventory, u32 amount);
bool inventory_remove_gold(InventoryComponent *inventory, u32 amount);

#endif // INVENTORY_H
