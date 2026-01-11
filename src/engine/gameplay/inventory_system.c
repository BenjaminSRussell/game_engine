// inventory_system.c - Implementation
#include "include/gameplay/inventory/inventory.h"
#include "include/gameplay/inventory/item_database.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

Container *container_create(u32 capacity, f32 max_weight) {
  Container *container = calloc(1, sizeof(Container));
  if (!container) {
    LOG_ERROR("Failed to allocate container");
    return NULL;
  }
  
  container_init(container, capacity, max_weight);
  return container;
}

void container_destroy(Container *container) {
  if (!container) return;
  
  container_cleanup(container);
  free(container);
}

void container_init(Container *container, u32 capacity, f32 max_weight) {
  if (!container) return;
  
  container->slots = calloc(capacity, sizeof(ItemStack));
  container->capacity = capacity;
  container->max_weight = max_weight;
  container->current_weight = 0.0f;
  container->owner = 0;
  container->name = NULL;
}

void container_cleanup(Container *container) {
  if (!container) return;
  
  free(container->slots);
  container->slots = NULL;
}

bool container_add_item(Container *container, ItemStack item, u32 *out_slot) {
  if (!container || !item.item_id) return false;
  
  // Try to stack with existing items first
  i32 stackable_slot = container_find_stackable_slot(container, item.item_id);
  if (stackable_slot >= 0) {
    ItemStack *existing = &container->slots[stackable_slot];
    u32 max_stack = item_database_get_max_stack(item.item_id);
    u32 can_add = max_stack - existing->quantity;
    
    if (can_add > 0) {
      u32 add_amount = (item.quantity < can_add) ? item.quantity : can_add;
      existing->quantity += add_amount;
      
      if (out_slot) *out_slot = stackable_slot;
      
      if (item.quantity > can_add) {
        // Add remaining to empty slot
        ItemStack remaining = item;
        remaining.quantity = item.quantity - can_add;
        return container_add_item(container, remaining, out_slot);
      }
      return true;
    }
  }
  
  // Find empty slot
  i32 empty_slot = container_find_empty_slot(container);
  if (empty_slot >= 0) {
    container->slots[empty_slot] = item;
    if (out_slot) *out_slot = empty_slot;
    return true;
  }
  
  return false;
}

bool container_add_item_to_slot(Container *container, u32 slot, ItemStack item) {
  if (!container || slot >= container->capacity) return false;
  
  ItemStack *existing = &container->slots[slot];
  if (existing->item_id == 0) {
    // Empty slot
    container->slots[slot] = item;
    return true;
  } else if (existing->item_id == item.item_id) {
    // Stack with existing
    u32 max_stack = item_database_get_max_stack(item.item_id);
    u32 can_add = max_stack - existing->quantity;
    u32 add_amount = (item.quantity < can_add) ? item.quantity : can_add;
    
    existing->quantity += add_amount;
    return item.quantity <= can_add;
  }
  
  return false;
}

bool container_remove_item(Container *container, u32 slot, u32 quantity,
                           ItemStack *out_item) {
  if (!container || slot >= container->capacity) return false;
  
  ItemStack *item = &container->slots[slot];
  if (item->item_id == 0 || quantity == 0) return false;
  
  if (out_item) {
    out_item->item_id = item->item_id;
    out_item->quantity = (quantity < item->quantity) ? quantity : item->quantity;
  }
  
  if (quantity >= item->quantity) {
    // Remove entire stack
    item->item_id = 0;
    item->quantity = 0;
  } else {
    // Remove partial stack
    item->quantity -= quantity;
  }
  
  return true;
}

bool container_remove_item_by_id(Container *container, u32 item_id,
                                 u32 quantity) {
  if (!container || !item_id) return false;
  
  u32 remaining = quantity;
  for (u32 i = 0; i < container->capacity && remaining > 0; i++) {
    if (container->slots[i].item_id == item_id) {
      u32 remove = (container->slots[i].quantity < remaining) ? 
                   container->slots[i].quantity : remaining;
      
      container_remove_item(container, i, remove, NULL);
      remaining -= remove;
    }
  }
  
  return remaining == 0;
}

bool container_swap_slots(Container *container, u32 slot_a, u32 slot_b) {
  if (!container || slot_a >= container->capacity || slot_b >= container->capacity) {
    return false;
  }
  
  ItemStack temp = container->slots[slot_a];
  container->slots[slot_a] = container->slots[slot_b];
  container->slots[slot_b] = temp;
  
  return true;
}

bool container_transfer(Container *src, u32 src_slot, Container *dest,
                        u32 dest_slot) {
  if (!src || !dest || src_slot >= src->capacity || dest_slot >= dest->capacity) {
    return false;
  }
  
  ItemStack item = src->slots[src_slot];
  if (item.item_id == 0) return true; // Nothing to transfer
  
  if (container_add_item_to_slot(dest, dest_slot, item)) {
    container_remove_item(src, src_slot, item.quantity, NULL);
    return true;
  }
  
  return false;
}

bool container_split_stack(Container *container, u32 slot, u32 amount,
                           u32 *out_new_slot) {
  if (!container || slot >= container->capacity || amount == 0) return false;
  
  ItemStack *source = &container->slots[slot];
  if (source->item_id == 0 || source->quantity <= amount) return false;
  
  // Find empty slot for split
  i32 empty_slot = container_find_empty_slot(container);
  if (empty_slot < 0) return false;
  
  // Create split stack
  ItemStack split = {
    .item_id = source->item_id,
    .quantity = amount
  };
  
  container->slots[empty_slot] = split;
  source->quantity -= amount;
  
  if (out_new_slot) *out_new_slot = empty_slot;
  return true;
}

bool container_has_item(const Container *container, u32 item_id, u32 quantity) {
  return container_count_item(container, item_id) >= quantity;
}

u32 container_count_item(const Container *container, u32 item_id) {
  if (!container || !item_id) return 0;
  
  u32 count = 0;
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id == item_id) {
      count += container->slots[i].quantity;
    }
  }
  return count;
}

i32 container_find_item(const Container *container, u32 item_id) {
  if (!container || !item_id) return -1;
  
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id == item_id) {
      return i;
    }
  }
  return -1;
}

i32 container_find_empty_slot(const Container *container) {
  if (!container) return -1;
  
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id == 0) {
      return i;
    }
  }
  return -1;
}

i32 container_find_stackable_slot(const Container *container, u32 item_id) {
  if (!container || !item_id) return -1;
  
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id == item_id) {
      u32 max_stack = item_database_get_max_stack(item_id);
      if (container->slots[i].quantity < max_stack) {
        return i;
      }
    }
  }
  return -1;
}

bool container_is_full(const Container *container) {
  return container_find_empty_slot(container) < 0;
}

bool container_is_empty(const Container *container) {
  if (!container) return true;
  
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id != 0) {
      return false;
    }
  }
  return true;
}

u32 container_get_used_slots(const Container *container) {
  if (!container) return 0;
  
  u32 count = 0;
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id != 0) {
      count++;
    }
  }
  return count;
}

f32 container_get_weight(const Container *container) {
  if (!container) return 0.0f;
  
  f32 weight = 0.0f;
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id != 0) {
      weight += item_database_get_weight(container->slots[i].item_id) * 
                container->slots[i].quantity;
    }
  }
  return weight;
}

void container_sort_by_type(Container *container) {
  // Simple bubble sort by item ID
  if (!container) return;
  
  for (u32 i = 0; i < container->capacity - 1; i++) {
    for (u32 j = 0; j < container->capacity - i - 1; j++) {
      if (container->slots[j].item_id > container->slots[j + 1].item_id) {
        container_swap_slots(container, j, j + 1);
      }
    }
  }
}

void container_sort_by_rarity(Container *container) {
  // Sort by item rarity (would need item database lookup)
  container_sort_by_type(container); // Fallback to type sort
}

void container_sort_by_value(Container *container) {
  // Sort by item value (would need item database lookup)
  container_sort_by_type(container); // Fallback to type sort
}

void container_consolidate_stacks(Container *container) {
  if (!container) return;
  
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item_id == 0) continue;
    
    // Look for matching items to consolidate
    for (u32 j = i + 1; j < container->capacity; j++) {
      if (container->slots[j].item_id == container->slots[i].item_id) {
        u32 max_stack = item_database_get_max_stack(container->slots[i].item_id);
        u32 can_transfer = max_stack - container->slots[i].quantity;
        
        if (can_transfer > 0) {
          u32 transfer = (container->slots[j].quantity < can_transfer) ?
                         container->slots[j].quantity : can_transfer;
          
          container->slots[i].quantity += transfer;
          container->slots[j].quantity -= transfer;
          
          if (container->slots[j].quantity == 0) {
            container->slots[j].item_id = 0;
          }
        }
      }
    }
  }
}

void container_clear(Container *container) {
  if (!container) return;
  
  memset(container->slots, 0, container->capacity * sizeof(ItemStack));
  container->current_weight = 0.0f;
}

InventoryComponent inventory_component_create(u32 main_capacity,
                                              u32 quickslot_count) {
  InventoryComponent inventory = {0};
  
  inventory.main_inventory = container_create(main_capacity, 0.0f);
  inventory.quick_slots = container_create(quickslot_count, 0.0f);
  inventory.gold = 0;
  
  return inventory;
}

void inventory_component_destroy(InventoryComponent *inventory) {
  if (!inventory) return;
  
  container_destroy(inventory->main_inventory);
  container_destroy(inventory->quick_slots);
  inventory->main_inventory = NULL;
  inventory->quick_slots = NULL;
}

Container *inventory_get_main(InventoryComponent *inventory) {
  return inventory ? inventory->main_inventory : NULL;
}

Container *inventory_get_quickslots(InventoryComponent *inventory) {
  return inventory ? inventory->quick_slots : NULL;
}

void inventory_add_gold(InventoryComponent *inventory, u32 amount) {
  if (inventory) {
    inventory->gold += amount;
  }
}

bool inventory_remove_gold(InventoryComponent *inventory, u32 amount) {
  if (!inventory || inventory->gold < amount) {
    return false;
  }
  
  inventory->gold -= amount;
  return true;
}
