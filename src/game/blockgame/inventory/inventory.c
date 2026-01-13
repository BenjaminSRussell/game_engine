// Inventory container operations.
// Roadmap: docs/INVENTORY_ROADMAP.md.
// Inventory auto-sort system with configurable sort criteria
// Inventory quick-stack functionality for chests and containers
// Inventory item stacking optimization to reduce fragmentation
// Inventory search and filter system for large inventories
// Inventory item locking to prevent accidental movement
// Inventory favorites system for quick access to important items
// Inventory templates for saving and loading item sets
// Inventory weight system with encumbrance mechanics
// Inventory item durability display and warnings
// Inventory item comparison system for upgrades
// Inventory drag-and-drop with visual feedback
// Inventory undo/redo system for mistake recovery
#include "../include/inventory/inventory.h"
#include <stdlib.h>
#include <string.h>

// Define logging macros if not already defined
#ifndef LOG_INFO
#define LOG_INFO(...) printf(__VA_ARGS__)
#endif
#ifndef LOG_WARN  
#define LOG_WARN(...) printf(__VA_ARGS__)
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(...) printf(__VA_ARGS__)
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#endif

static void inventory_mark_dirty(Inventory *inv) {
  if (inv) {
    inv->dirty = true;
  }
}

static void inventory_emit(Inventory *inv, InventoryEventType event,
                           u32 item_id, u16 count, u32 slot_index) {
  if (inv && inv->on_event) {
    inv->on_event(inv, event, item_id, count, slot_index, inv->user_data);
  }
}

void inventory_init(Inventory *inv) {
  memset(inv, 0, sizeof(Inventory));
  inv->selected_hotbar = 0;
  inv->dirty = false;
  inv->on_event = NULL;
  inv->user_data = NULL;
}

void inventory_free(Inventory *inv) {
  if (!inv) {
    return;
  }

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].enchantments) {
      free(inv->slots[i].enchantments);
      inv->slots[i].enchantments = NULL;
    }
    inv->slots[i].enchantment_count = 0;
    inv->slots[i].enchantment_capacity = 0;
  }

  memset(inv, 0, sizeof(Inventory));
}

bool inventory_add_item(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || count == 0)
    return false;
  // Basic validation
  if (item_id == 0)
    return false; // invalid item id

  // Validate item_id is within valid range
  if (item_id > 65535) {
    LOG_WARN("Attempted to add invalid item_id %u to inventory (out of range)",
             item_id);
    return false;
  }

  u16 remaining = count;
  bool has_space = false;

  // First pass: try to stack with existing items (auto-stacking)
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS && remaining > 0; i++) {
    if (inv->slots[i].item_id == 0) {
      has_space = true;
      continue;
    }

    if (inv->slots[i].item_id == item_id) {
      u16 space = STACK_SIZE_DEFAULT - inv->slots[i].count;
      if (space > 0) {
        u16 add = (remaining < space) ? remaining : space;
        inv->slots[i].count += add;
        remaining -= add;
      }
    }
  }

  // Second pass: fill empty slots
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS && remaining > 0; i++) {
    if (inv->slots[i].item_id == 0) {
      u16 add =
          (remaining < STACK_SIZE_DEFAULT) ? remaining : STACK_SIZE_DEFAULT;
      inv->slots[i].item_id = item_id;
      inv->slots[i].count = add;
      remaining -= add;
      has_space = true;
    }
  }

  u16 added = (u16)(count - remaining);
  if (added > 0) {
    inv->total_items += added;
    inventory_mark_dirty(inv);
    inventory_emit(inv, INVENTORY_EVENT_ADD, item_id, added, UINT32_MAX);
  }

  // Inventory full notification/feedback
  if (remaining > 0) {
    LOG_WARN("Inventory full: could not add %u items of type %u", remaining,
             item_id);
    inventory_emit(inv, INVENTORY_EVENT_FULL, item_id, remaining, UINT32_MAX);
  }

  return remaining == 0;
}

bool inventory_remove_item(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || count == 0)
    return false;
  u16 remaining = count;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS && remaining > 0; i++) {
    if (inv->slots[i].item_id == item_id) {
      u16 remove =
          (remaining < inv->slots[i].count) ? remaining : inv->slots[i].count;
      inv->slots[i].count -= remove;
      remaining -= remove;

      if (inv->slots[i].count == 0) {
        inv->slots[i].item_id = 0;
      }
    }
  }

  u16 removed = (u16)(count - remaining);
  if (removed > 0) {
    inv->total_items -= removed;
    inventory_mark_dirty(inv);
    inventory_emit(inv, INVENTORY_EVENT_REMOVE, item_id, removed, UINT32_MAX);
  }

  return remaining == 0;
}

bool inventory_has_item(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || count == 0)
    return true;

  u16 total = 0;
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      total += inv->slots[i].count;
      if (total >= count)
        return true;
    }
  }

  return total >= count;
}

u16 inventory_get_item_count(Inventory *inv, u32 item_id) {
  if (!inv)
    return 0;

  u16 total = 0;
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      total += inv->slots[i].count;
    }
  }

  return total;
}

bool inventory_get_slot(Inventory *inv, u32 slot_index,
                        InventorySlot *out_slot) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS || !out_slot)
    return false;

  *out_slot = inv->slots[slot_index];
  return true;
}

bool inventory_set_slot(Inventory *inv, u32 slot_index, u32 item_id,
                        u16 count) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS)
    return false;
  // Basic validation
  if (item_id == 0) {
    if (inv->slots[slot_index].enchantments) {
      free(inv->slots[slot_index].enchantments);
      inv->slots[slot_index].enchantments = NULL;
    }
    inv->slots[slot_index].enchantment_count = 0;
    inv->slots[slot_index].enchantment_capacity = 0;
    inv->slots[slot_index].item_id = 0;
    inv->slots[slot_index].count = 0;
    return true;
  }

  // Validate item_id range
  if (item_id > 65535) {
    LOG_WARN("Attempted to set invalid item_id %u in slot %u (out of range)",
             item_id, slot_index);
    return false;
  }

  // Clamp count to stack limit
  if (count == 0)
    count = 1; // Ensure at least 1 item
  if (count > STACK_SIZE_DEFAULT) {
    LOG_DEBUG("Clamping item count from %u to %u in slot %u", count,
              STACK_SIZE_DEFAULT, slot_index);
    count = STACK_SIZE_DEFAULT;
  }

  if (inv->slots[slot_index].item_id != 0 &&
      inv->slots[slot_index].item_id != item_id) {
    if (inv->slots[slot_index].enchantments) {
      free(inv->slots[slot_index].enchantments);
      inv->slots[slot_index].enchantments = NULL;
    }
    inv->slots[slot_index].enchantment_count = 0;
    inv->slots[slot_index].enchantment_capacity = 0;
  }

  inv->slots[slot_index].item_id = item_id;
  inv->slots[slot_index].count = count;

  if (item_id == 0) {
    inv->slots[slot_index].durability = 0.0f;
  }

  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_SET_SLOT, item_id, count, slot_index);
  return true;
}

void inventory_select_hotbar(Inventory *inv, u32 index) {
  if (!inv || index >= MAX_HOTBAR_SLOTS)
    return;
  inv->selected_hotbar = index;
}

u32 inventory_get_selected_item(Inventory *inv) {
  if (!inv)
    return 0;
  u32 hotbar_start = MAX_INVENTORY_SLOTS - MAX_HOTBAR_SLOTS;
  u32 slot_index = hotbar_start + inv->selected_hotbar;
  return inv->slots[slot_index].item_id;
}

InventorySlot inventory_get_selected_slot(Inventory *inv) {
  InventorySlot empty = {0, 0, 0};
  if (!inv)
    return empty;

  u32 hotbar_start = MAX_INVENTORY_SLOTS - MAX_HOTBAR_SLOTS;
  u32 slot_index = hotbar_start + inv->selected_hotbar;
  return inv->slots[slot_index];
}

bool inventory_is_full(Inventory *inv) {
  if (!inv)
    return true;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0 ||
        inv->slots[i].count < STACK_SIZE_DEFAULT) {
      return false;
    }
  }

  return true;
}

u32 inventory_get_empty_slots(Inventory *inv) {
  if (!inv)
    return 0;

  u32 empty = 0;
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0) {
      empty++;
    }
  }

  return empty;
}

void inventory_sort(Inventory *inv) {
  if (!inv)
    return;

  // Use insertion sort for stable sorting
  for (u32 i = 1; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0)
      continue; // Skip empty slots, they'll naturally move to the end

    InventorySlot key = inv->slots[i];
    i32 j = i - 1;

    // Move elements that are greater than key one position ahead
    while (j >= 0 && inv->slots[j].item_id != 0 && inv->slots[j].item_id > key.item_id) {
      inv->slots[j + 1] = inv->slots[j];
      j--;
    }
    inv->slots[j + 1] = key;
  }

  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_SORTED, 0, 0, UINT32_MAX);
}

void inventory_sort_by_type(Inventory *inv, InventorySortType sort_type) {
  if (!inv || sort_type < SORT_BY_NAME || sort_type > SORT_BY_DURABILITY)
    return;

  // Simple bubble sort for demonstration - in production would use more
  // efficient algorithm
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS - 1; i++) {
    for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
      bool should_swap = false;

      switch (sort_type) {
      case SORT_BY_NAME:
        // Would need item registry to get names - placeholder
        should_swap =
            (inv->slots[i].item_id == 0 && inv->slots[j].item_id != 0) ||
            (inv->slots[i].item_id != 0 && inv->slots[j].item_id != 0 &&
             inv->slots[i].item_id > inv->slots[j].item_id);
        break;

      case SORT_BY_TYPE:
        // Would need item registry to get types - placeholder
        should_swap =
            (inv->slots[i].item_id == 0 && inv->slots[j].item_id != 0);
        break;

      case SORT_BY_QUANTITY:
        should_swap = (inv->slots[i].count < inv->slots[j].count);
        break;

      case SORT_BY_ID:
        should_swap = (inv->slots[i].item_id > inv->slots[j].item_id);
        break;

      case SORT_BY_DURABILITY:
        should_swap = (inv->slots[i].durability < inv->slots[j].durability);
        break;

      default:
        break;
      }

      if (should_swap) {
        InventorySlot temp = inv->slots[i];
        inv->slots[i] = inv->slots[j];
        inv->slots[j] = temp;
      }
    }
  }

  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_SORTED, 0, 0, UINT32_MAX);
}

void inventory_compact(Inventory *inv) {
  if (!inv)
    return;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0)
      continue;

    for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
      if (inv->slots[j].item_id == inv->slots[i].item_id) {
        u16 space = STACK_SIZE_DEFAULT - inv->slots[i].count;
        if (space > 0) {
          u16 move =
              (inv->slots[j].count < space) ? inv->slots[j].count : space;
          inv->slots[i].count += move;
          inv->slots[j].count -= move;

          if (inv->slots[j].count == 0) {
            inv->slots[j].item_id = 0;
          }
        }
      }
    }
  }
  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_COMPACTED, 0, 0, UINT32_MAX);
}

void inventory_clear(Inventory *inv) {
  if (!inv)
    return;
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].enchantments) {
      free(inv->slots[i].enchantments);
      inv->slots[i].enchantments = NULL;
    }
    inv->slots[i].enchantment_count = 0;
    inv->slots[i].enchantment_capacity = 0;
  }
  memset(inv->slots, 0, sizeof(inv->slots));
  inv->total_items = 0;
  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_CLEAR, 0, 0, UINT32_MAX);
}

// Advanced stacking and management functions
u16 inventory_get_max_stack_size(u32 item_id) {
  if (item_id == 0)
    return 0;

  // In a full implementation, this would query the item registry
  // For now, use simplified logic based on item ID ranges
  if (item_id >= 100 && item_id < 200) {        // Tools and weapons
    return STACK_SIZE_LARGE;                    // 1
  } else if (item_id >= 200 && item_id < 300) { // Armor
    return STACK_SIZE_LARGE;                    // 1
  } else if (item_id >= 300 && item_id < 400) { // Food
    return STACK_SIZE_DEFAULT;                  // 64
  } else {                                      // Blocks and materials
    return STACK_SIZE_DEFAULT;                  // 64
  }
}

bool inventory_can_stack_items(u32 item_id1, u32 item_id2) {
  if (item_id1 == 0 || item_id2 == 0)
    return false;
  if (item_id1 != item_id2)
    return false;

  // Items with max stack size of 1 cannot stack
  return inventory_get_max_stack_size(item_id1) > 1;
}

bool inventory_merge_stacks(Inventory *inv, u32 source_slot, u32 target_slot) {
  if (!inv || source_slot >= MAX_INVENTORY_SLOTS ||
      target_slot >= MAX_INVENTORY_SLOTS) {
    return false;
  }

  InventorySlot *source = &inv->slots[source_slot];
  InventorySlot *target = &inv->slots[target_slot];

  if (source->item_id == 0 || target->item_id == 0)
    return false;
  if (!inventory_can_stack_items(source->item_id, target->item_id))
    return false;

  u16 max_stack = inventory_get_max_stack_size(source->item_id);
  u16 space = max_stack - target->count;

  if (space == 0)
    return false;

  u16 move = (source->count < space) ? source->count : space;
  target->count += move;
  source->count -= move;

  if (source->count == 0) {
    source->item_id = 0;
  }

  inventory_mark_dirty(inv);
  return true;
}

bool inventory_split_stack(Inventory *inv, u32 source_slot, u32 target_slot,
                           u16 amount) {
  if (!inv || source_slot >= MAX_INVENTORY_SLOTS ||
      target_slot >= MAX_INVENTORY_SLOTS) {
    LOG_WARN("Invalid slot indices: source=%u, target=%u, max=%u", 
             source_slot, target_slot, MAX_INVENTORY_SLOTS);
    return false;
  }

  if (source_slot == target_slot) {
    LOG_DEBUG("Source and target slots are the same: %u", source_slot);
    return false;
  }

  InventorySlot *source = &inv->slots[source_slot];
  InventorySlot *target = &inv->slots[target_slot];

  // Validate source slot has items
  if (source->item_id == 0 || source->count == 0) {
    LOG_DEBUG("Source slot %u is empty", source_slot);
    return false;
  }

  // Validate amount
  if (amount == 0) {
    LOG_DEBUG("Split amount is 0");
    return false;
  }

  if (amount > source->count) {
    LOG_WARN("Split amount %u exceeds source count %u", amount, source->count);
    return false;
  }

  // Check if target slot is empty or can stack with source
  if (target->item_id != 0) {
    if (!inventory_can_stack_items(source->item_id, target->item_id)) {
      LOG_DEBUG("Target slot %u has incompatible item %u (source: %u)", 
                target_slot, target->item_id, source->item_id);
      return false;
    }
  }

  u16 max_stack = inventory_get_max_stack_size(source->item_id);
  
  // Calculate how much can actually be moved
  u16 available_space = max_stack - (target->item_id == 0 ? 0 : target->count);
  u16 actual_move = (amount < available_space) ? amount : available_space;
  
  if (actual_move == 0) {
    LOG_DEBUG("No space available in target slot %u", target_slot);
    return false;
  }

  // Handle enchantment transfer for split operations
  if (target->item_id == 0) {
    // Target is empty, create new slot with source properties
    target->item_id = source->item_id;
    target->count = actual_move;
    target->durability = source->durability;
    
    // Copy enchantments if we're taking the entire stack
    if (actual_move == source->count && source->enchantments) {
      target->enchantment_count = source->enchantment_count;
      target->enchantment_capacity = source->enchantment_capacity;
      
      if (target->enchantment_count > 0) {
        target->enchantments = malloc(sizeof(Enchantment) * target->enchantment_capacity);
        if (target->enchantments) {
          memcpy(target->enchantments, source->enchantments, 
                 sizeof(Enchantment) * target->enchantment_count);
        }
      }
    } else {
      target->enchantments = NULL;
      target->enchantment_count = 0;
      target->enchantment_capacity = 0;
    }
  } else {
    // Target has items, just add to count
    target->count += actual_move;
  }

  // Update source slot
  source->count -= actual_move;
  
  // Clear source slot if empty
  if (source->count == 0) {
    source->item_id = 0;
    source->durability = 0.0f;
    
    // Free enchantments if source is now empty
    if (source->enchantments) {
      free(source->enchantments);
      source->enchantments = NULL;
      source->enchantment_count = 0;
      source->enchantment_capacity = 0;
    }
  } else if (actual_move == source->count + actual_move) {
    // We split the entire stack, clear enchantments from source
    if (source->enchantments) {
      free(source->enchantments);
      source->enchantments = NULL;
      source->enchantment_count = 0;
      source->enchantment_capacity = 0;
    }
  }

  inventory_mark_dirty(inv);
  LOG_DEBUG("Split %u items from slot %u to slot %u (moved: %u)", 
            amount, source_slot, target_slot, actual_move);
  
  return true;
}

u32 inventory_find_best_stack_slot(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || item_id == 0)
    return UINT32_MAX;

  u16 max_stack = inventory_get_max_stack_size(item_id);

  // First, find existing stacks that can accommodate the items
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      u16 space = max_stack - inv->slots[i].count;
      if (space >= count) {
        return i; // Perfect fit
      }
    }
  }

  // Then find any existing stack of the same item
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      return i; // Partial fit
    }
  }

  // Finally, find an empty slot
  return inventory_find_empty_slot(inv);
}

void inventory_auto_stack(Inventory *inv) {
  if (!inv)
    return;

  inventory_compact(inv);                  // First consolidate existing stacks
  inventory_sort_by_type(inv, SORT_BY_ID); // Sort by item type
}

// Item management functions
bool inventory_move_item(Inventory *inv, u32 source_slot, u32 target_slot) {
  if (!inv || source_slot >= MAX_INVENTORY_SLOTS ||
      target_slot >= MAX_INVENTORY_SLOTS) {
    LOG_WARN("Invalid slot indices: source=%u, target=%u, max=%u", 
             source_slot, target_slot, MAX_INVENTORY_SLOTS);
    return false;
  }

  if (source_slot == target_slot) {
    LOG_DEBUG("Source and target slots are the same: %u", source_slot);
    return true;
  }

  InventorySlot *source = &inv->slots[source_slot];
  InventorySlot *target = &inv->slots[target_slot];

  // Validate source has items
  if (source->item_id == 0 || source->count == 0) {
    LOG_DEBUG("Source slot %u is empty", source_slot);
    return false;
  }

  // If target is empty, just move the entire stack
  if (target->item_id == 0) {
    *target = *source;
    memset(source, 0, sizeof(InventorySlot));
    inventory_mark_dirty(inv);
    LOG_DEBUG("Moved entire stack from slot %u to empty slot %u", source_slot, target_slot);
    return true;
  }

  // Check if items can be stacked
  if (inventory_can_stack_items(source->item_id, target->item_id)) {
    // Try to merge stacks
    u16 max_stack = inventory_get_max_stack_size(source->item_id);
    u16 space_available = max_stack - target->count;
    
    if (space_available > 0) {
      u16 move_amount = (source->count < space_available) ? 
                       source->count : space_available;
      
      target->count += move_amount;
      source->count -= move_amount;
      
      // Clear source if empty
      if (source->count == 0) {
        source->item_id = 0;
        source->durability = 0.0f;
        
        // Transfer enchantments if entire stack moved
        if (move_amount == source->count + move_amount && source->enchantments) {
          // Merge enchantment arrays
          u32 total_enchantments = target->enchantment_count + source->enchantment_count;
          if (total_enchantments > 0) {
            Enchantment *new_enchantments = realloc(target->enchantments, 
                                               sizeof(Enchantment) * total_enchantments);
            if (new_enchantments) {
              // Copy source enchantments to target
              memcpy(&new_enchantments[target->enchantment_count], 
                     source->enchantments, 
                     sizeof(Enchantment) * source->enchantment_count);
              
              target->enchantments = new_enchantments;
              target->enchantment_count = total_enchantments;
              target->enchantment_capacity = total_enchantments;
            }
          }
          
          // Clear source enchantments
          free(source->enchantments);
          source->enchantments = NULL;
          source->enchantment_count = 0;
          source->enchantment_capacity = 0;
        }
      }
      
      inventory_mark_dirty(inv);
      LOG_DEBUG("Merged %u items from slot %u to slot %u", 
                move_amount, source_slot, target_slot);
      return true;
    }
    
    // No space available, need to swap
    LOG_DEBUG("Target slot %u is full, swapping", target_slot);
  } else {
    LOG_DEBUG("Items cannot stack: source=%u, target=%u", 
              source->item_id, target->item_id);
  }

  // If we can't merge, perform atomic swap
  return inventory_swap_items(inv, source_slot, target_slot);
}

bool inventory_swap_items(Inventory *inv, u32 slot1, u32 slot2) {
  if (!inv || slot1 >= MAX_INVENTORY_SLOTS || slot2 >= MAX_INVENTORY_SLOTS) {
    LOG_WARN("Invalid slot indices: slot1=%u, slot2=%u, max=%u", 
             slot1, slot2, MAX_INVENTORY_SLOTS);
    return false;
  }

  if (slot1 == slot2) {
    LOG_DEBUG("Slots are the same: %u", slot1);
    return true;
  }

  // Store original slots for logging and potential rollback
  InventorySlot original_slot1 = inv->slots[slot1];
  InventorySlot original_slot2 = inv->slots[slot2];

  // Perform atomic swap
  inv->slots[slot1] = original_slot2;
  inv->slots[slot2] = original_slot1;

  inventory_mark_dirty(inv);
  
  LOG_DEBUG("Swapped slots %u and %u: slot1 had %u x %d, slot2 had %u x %d", 
            slot1, slot2, 
            original_slot1.item_id, original_slot1.count,
            original_slot2.item_id, original_slot2.count);
  
  return true;
}

void inventory_set_favorite(Inventory *inv, u32 slot_index, bool favorite) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS)
    return;

  inv->slots[slot_index].is_favorite = favorite;
  inventory_mark_dirty(inv);
}

bool inventory_is_favorite(Inventory *inv, u32 slot_index) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS)
    return false;

  return inv->slots[slot_index].is_favorite;
}

void inventory_set_locked(Inventory *inv, u32 slot_index, bool locked) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS)
    return;

  inv->slots[slot_index].is_locked = locked;
  inventory_mark_dirty(inv);
}

bool inventory_is_locked(Inventory *inv, u32 slot_index) {
  if (!inv || slot_index >= MAX_INVENTORY_SLOTS)
    return false;

  return inv->slots[slot_index].is_locked;
}

// Search and filter functions
u32 inventory_find_first_item(Inventory *inv, u32 item_id) {
  if (!inv || item_id == 0)
    return UINT32_MAX;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      return i;
    }
  }

  return UINT32_MAX;
}

u32 inventory_find_empty_slot(Inventory *inv) {
  if (!inv)
    return UINT32_MAX;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0) {
      return i;
    }
  }

  return UINT32_MAX;
}

u32 inventory_count_items_of_type(Inventory *inv, ItemType type) {
  if (!inv)
    return 0;

  u32 count = 0;
  // In a full implementation, this would query item registry for types
  // For now, count non-empty slots as placeholder
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id != 0) {
      count++;
    }
  }

  return count;
}

bool inventory_has_space_for(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || item_id == 0 || count == 0)
    return false;

  u16 max_stack = inventory_get_max_stack_size(item_id);
  u16 total_space = 0;

  // Check existing stacks
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == item_id) {
      total_space += (max_stack - inv->slots[i].count);
    } else if (inv->slots[i].item_id == 0) {
      total_space += max_stack;
    }
  }

  return total_space >= count;
}

// Utility functions
void inventory_debug_print(Inventory *inv) {
  if (!inv)
    return;

  LOG_DEBUG("Inventory Debug Info:");
  LOG_DEBUG("  Total Items: %u", inv->total_items);
  LOG_DEBUG("  Selected Hotbar: %u", inv->selected_hotbar);
  LOG_DEBUG("  Empty Slots: %u", inventory_get_empty_slots(inv));
  LOG_DEBUG("  Is Full: %s", inventory_is_full(inv) ? "Yes" : "No");

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id != 0) {
      LOG_DEBUG("  Slot %u: Item %u, Count %u, Favorite: %s, Locked: %s", i,
                inv->slots[i].item_id, inv->slots[i].count,
                inv->slots[i].is_favorite ? "Yes" : "No",
                inv->slots[i].is_locked ? "Yes" : "No");
    }
  }
}

bool inventory_validate(Inventory *inv) {
  if (!inv) {
    LOG_WARN("Inventory pointer is NULL");
    return false;
  }

  u32 total_count = 0;
  u32 non_empty_slots = 0;
  bool has_errors = false;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    InventorySlot *slot = &inv->slots[i];

    // Debug check: ensure item_count > 0 implies item_id != 0 and vice versa
    if (slot->item_id == 0 && slot->count > 0) {
      LOG_ERROR("Slot %u: item_id is 0 but count is %u", i, slot->count);
      has_errors = true;
    }
    
    if (slot->item_id > 0 && slot->count == 0) {
      LOG_ERROR("Slot %u: item_id is %u but count is 0", i, slot->item_id);
      has_errors = true;
    }

    if (slot->item_id != 0) {
      non_empty_slots++;
      
      // Validate item_id range
      if (slot->item_id > 65535) {
        LOG_ERROR("Slot %u: item_id %u is out of valid range", i, slot->item_id);
        has_errors = true;
      }

      // Validate count
      u16 max_stack = inventory_get_max_stack_size(slot->item_id);
      if (slot->count == 0) {
        LOG_ERROR("Slot %u: item_id %u has zero count", i, slot->item_id);
        has_errors = true;
      } else if (slot->count > max_stack) {
        LOG_ERROR("Slot %u: item count %u exceeds max stack %u for item %u", 
                  i, slot->count, max_stack, slot->item_id);
        has_errors = true;
      }

      // Validate durability if applicable
      if (slot->durability < 0.0f || slot->durability > 1.0f) {
        LOG_ERROR("Slot %u: durability %.3f is out of range [0.0, 1.0]", 
                  i, slot->durability);
        has_errors = true;
      }

      // Validate enchantments
      if (slot->enchantment_count > 0 && !slot->enchantments) {
        LOG_ERROR("Slot %u: enchantment_count is %u but enchantments is NULL", 
                  i, slot->enchantment_count);
        has_errors = true;
      }
      
      if (slot->enchantments && slot->enchantment_count == 0) {
        LOG_ERROR("Slot %u: enchantments is not NULL but enchantment_count is 0", i);
        has_errors = true;
      }
      
      if (slot->enchantment_count > slot->enchantment_capacity) {
        LOG_ERROR("Slot %u: enchantment_count %u exceeds capacity %u", 
                  i, slot->enchantment_count, slot->enchantment_capacity);
        has_errors = true;
      }

      total_count += slot->count;
    } else {
      // Empty slot should have all fields zeroed
      if (slot->count != 0 || slot->durability != 0.0f || 
          slot->enchantments != NULL || slot->enchantment_count != 0 || 
          slot->enchantment_capacity != 0) {
        LOG_ERROR("Slot %u: empty slot has non-zero fields", i);
        has_errors = true;
      }
    }
  }

  // Validate total items count
  if (total_count != inv->total_items) {
    LOG_WARN("Inventory total count mismatch: calculated %u, stored %u", 
              total_count, inv->total_items);
    inv->total_items = total_count; // Auto-correct
  }

  // Validate selected hotbar index
  if (inv->selected_hotbar >= MAX_INVENTORY_SLOTS) {
    LOG_ERROR("Selected hotbar %u is out of range (max: %u)", 
              inv->selected_hotbar, MAX_INVENTORY_SLOTS);
    has_errors = true;
  }

  if (has_errors) {
    LOG_ERROR("Inventory validation failed with %u non-empty slots", non_empty_slots);
    return false;
  }

  LOG_DEBUG("Inventory validation passed: %u items in %u slots", total_count, non_empty_slots);
  return true;
}

// Inventory auto-sort system with configurable sort criteria
void inventory_auto_sort(Inventory *inv, InventorySortType sort_type) {
  if (!inv)
    return;

  // Simple bubble sort implementation - could be optimized
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS - 1; i++) {
    for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
      bool should_swap = false;

      switch (sort_type) {
      case SORT_BY_NAME:
        // Would need item registry to get names - placeholder
        should_swap =
            (inv->slots[i].item_id == 0 && inv->slots[j].item_id != 0) ||
            (inv->slots[i].item_id != 0 && inv->slots[j].item_id != 0 &&
             inv->slots[i].item_id > inv->slots[j].item_id);
        break;

      case SORT_BY_TYPE:
        // Would need item registry to get types - placeholder
        should_swap =
            (inv->slots[i].item_id == 0 && inv->slots[j].item_id != 0);
        break;

      case SORT_BY_QUANTITY:
        should_swap = (inv->slots[i].count < inv->slots[j].count);
        break;

      case SORT_BY_DURABILITY:
        should_swap = (inv->slots[i].durability < inv->slots[j].durability);
        break;
      default:
        break;
      }

      if (should_swap) {
        InventorySlot temp = inv->slots[i];
        inv->slots[i] = inv->slots[j];
        inv->slots[j] = temp;
      }
    }
  }

  inventory_mark_dirty(inv);
  inventory_emit(inv, INVENTORY_EVENT_SORTED, 0, 0, UINT32_MAX);
  LOG_INFO("Inventory auto-sorted by type %d", sort_type);
}

// Inventory quick-stack functionality for chests and containers
u32 inventory_quick_stack(Inventory *inv, Inventory *target) {
  if (!inv || !target)
    return 0;

  u32 stacked_items = 0;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0 || inv->slots[i].count == 0)
      continue;

    u32 item_id = inv->slots[i].item_id;
    u16 available_count = inv->slots[i].count;

    // Try to stack in target inventory
    for (u32 j = 0; j < MAX_INVENTORY_SLOTS && available_count > 0; j++) {
      if (target->slots[j].item_id == item_id) {
        u16 space = STACK_SIZE_DEFAULT - target->slots[j].count;
        if (space > 0) {
          u16 transfer = (available_count < space) ? available_count : space;
          target->slots[j].count += transfer;
          inv->slots[i].count -= transfer;
          available_count -= transfer;
          stacked_items += transfer;
        }
      }
    }

    // If still has items, try to place in empty slots
    if (available_count > 0) {
      for (u32 j = 0; j < MAX_INVENTORY_SLOTS && available_count > 0; j++) {
        if (target->slots[j].item_id == 0) {
          u16 transfer = (available_count < STACK_SIZE_DEFAULT)
                             ? available_count
                             : STACK_SIZE_DEFAULT;
          target->slots[j].item_id = item_id;
          target->slots[j].count = transfer;
          inv->slots[i].count -= transfer;
          available_count -= transfer;
          stacked_items += transfer;
          break;
        }
      }
    }

    // Remove empty slot
    if (inv->slots[i].count == 0) {
      inv->slots[i].item_id = 0;
      inv->slots[i].durability = 0.0f;
    }
  }

  if (stacked_items > 0) {
    inventory_mark_dirty(inv);
    inventory_mark_dirty(target);
    LOG_INFO("Quick-stacked %u items", stacked_items);
  }

  return stacked_items;
}

u16 inventory_simulate_add(Inventory *inv, u32 item_id, u16 count) {
  if (!inv || count == 0 || item_id == 0)
    return 0;

  u16 remaining = count;
  u16 max_stack = inventory_get_max_stack_size(item_id);

  // First pass: simulate stacking with existing items
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS && remaining > 0; i++) {
    if (inv->slots[i].item_id == item_id) {
      u16 space = max_stack - inv->slots[i].count;
      if (space > 0) {
        u16 add = (remaining < space) ? remaining : space;
        remaining -= add;
      }
    }
  }

  // Second pass: simulate filling empty slots
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS && remaining > 0; i++) {
    if (inv->slots[i].item_id == 0) {
      u16 add = (remaining < max_stack) ? remaining : max_stack;
      remaining -= add;
    }
  }

  return count - remaining; // Return amount that could be added
}

u32 inventory_search_items(const Inventory *inv, u32 item_id,
                           InventorySearchResult *results, u32 max_results) {
  if (!inv || !results || max_results == 0)
    return 0;

  u32 found = 0;
  
  // If item_id is 0, search for all non-empty slots
  if (item_id == 0) {
    for (u32 i = 0; i < MAX_INVENTORY_SLOTS && found < max_results; i++) {
      if (inv->slots[i].item_id > 0 && inv->slots[i].count > 0) {
        results[found].item_id = inv->slots[i].item_id;
        results[found].count = inv->slots[i].count;
        results[found].slot_index = i;
        found++;
      }
    }
  } else {
    // Search for specific item_id
    for (u32 i = 0; i < MAX_INVENTORY_SLOTS && found < max_results; i++) {
      if (inv->slots[i].item_id == item_id && inv->slots[i].count > 0) {
        results[found].item_id = inv->slots[i].item_id;
        results[found].count = inv->slots[i].count;
        results[found].slot_index = i;
        found++;
      }
    }
  }

  return found;
}

void inventory_optimize_stacking(Inventory *inv) {
  if (!inv)
    return;

  bool optimized = false;
  
  // First pass: consolidate split stacks of the same item
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id == 0 || inv->slots[i].count == 0)
      continue;

    u32 item_id = inv->slots[i].item_id;
    u16 max_stack = inventory_get_max_stack_size(item_id);
    
    // Skip if this slot is already full
    if (inv->slots[i].count >= max_stack)
      continue;
    
    // Find all other slots with the same item and merge them
    for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
      if (inv->slots[j].item_id == item_id && inv->slots[j].count > 0) {
        // Calculate how much we can move to slot i
        u16 space_available = max_stack - inv->slots[i].count;
        if (space_available > 0) {
          u16 move_amount = (inv->slots[j].count < space_available) ? 
                          inv->slots[j].count : space_available;
          
          inv->slots[i].count += move_amount;
          inv->slots[j].count -= move_amount;
          optimized = true;
          
          // Clear the source slot if it's now empty
          if (inv->slots[j].count == 0) {
            inv->slots[j].item_id = 0;
            inv->slots[j].durability = 0.0f;
            
            // Free enchantments if any
            if (inv->slots[j].enchantments) {
              free(inv->slots[j].enchantments);
              inv->slots[j].enchantments = NULL;
              inv->slots[j].enchantment_count = 0;
              inv->slots[j].enchantment_capacity = 0;
            }
          }
          
          // If slot i is now full, move to next slot
          if (inv->slots[i].count >= max_stack)
            break;
        }
      }
    }
  }
  
  // Second pass: compact inventory by moving items towards the beginning
  // This helps with inventory organization and UI display
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS - 1; i++) {
    if (inv->slots[i].item_id == 0 || inv->slots[i].count == 0) {
      // Find the next non-empty slot
      for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
        if (inv->slots[j].item_id > 0 && inv->slots[j].count > 0) {
          // Move slot j to slot i
          inv->slots[i] = inv->slots[j];
          
          // Clear slot j
          inv->slots[j].item_id = 0;
          inv->slots[j].count = 0;
          inv->slots[j].durability = 0.0f;
          inv->slots[j].enchantments = NULL;
          inv->slots[j].enchantment_count = 0;
          inv->slots[j].enchantment_capacity = 0;
          
          optimized = true;
          break;
        }
      }
    }
  }

  if (optimized) {
    inventory_mark_dirty(inv);
    LOG_DEBUG("Inventory stacking optimized and compacted");
  }
}

bool inventory_serialize(const Inventory *inv, char *buffer, u32 buffer_size) {
  if (!inv || !buffer || buffer_size == 0) {
    LOG_WARN("Invalid parameters for inventory serialization");
    return false;
  }

  // Enhanced JSON serialization format with enchantments support
  // Format: {"total_items":X,"selected_hotbar":Y,"slots":[...]}
  
  int written = snprintf(buffer, buffer_size,
    "{\"total_items\":%u,\"selected_hotbar\":%u,\"slots\":[",
    inv->total_items, inv->selected_hotbar);
  
  if (written < 0 || written >= buffer_size) {
    LOG_WARN("Buffer overflow during inventory serialization header");
    return false;
  }

  bool first = true;
  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    if (inv->slots[i].item_id != 0) {
      const char *prefix = first ? "" : ",";
      
      // Start slot object
      int slot_start = snprintf(buffer + written, buffer_size - written,
        "%s{\"index\":%u,\"item_id\":%u,\"count\":%u,"
        "\"durability\":%.3f,\"spoil_progress\":%.3f,"
        "\"quality_modifier\":%.3f,\"is_favorite\":%s,\"is_locked\":%s",
        prefix, i, inv->slots[i].item_id, inv->slots[i].count,
        inv->slots[i].durability, inv->slots[i].spoil_progress,
        inv->slots[i].quality_modifier,
        inv->slots[i].is_favorite ? "true" : "false",
        inv->slots[i].is_locked ? "true" : "false");
      
      if (slot_start < 0 || written + slot_start >= buffer_size) {
        LOG_WARN("Buffer overflow during slot serialization");
        return false;
      }
      written += slot_start;
      
      // Add enchantments if present
      if (inv->slots[i].enchantment_count > 0 && inv->slots[i].enchantments) {
        int enchant_start = snprintf(buffer + written, buffer_size - written,
          ",\"enchantments\":[");
        
        if (enchant_start < 0 || written + enchant_start >= buffer_size) {
          LOG_WARN("Buffer overflow during enchantments start");
          return false;
        }
        written += enchant_start;
        
        bool first_enchant = true;
        for (u32 e = 0; e < inv->slots[i].enchantment_count; e++) {
          const char *enchant_prefix = first_enchant ? "" : ",";
          Enchantment *enchant = &inv->slots[i].enchantments[e];
          
          int enchant_written = snprintf(buffer + written, buffer_size - written,
            "%s{\"type\":%u,\"level\":%u,\"duration\":%u}",
            enchant_prefix, enchant->type, enchant->level, enchant->duration);
          
          if (enchant_written < 0 || written + enchant_written >= buffer_size) {
            LOG_WARN("Buffer overflow during enchantment serialization");
            return false;
          }
          written += enchant_written;
          first_enchant = false;
        }
        
        int enchant_end = snprintf(buffer + written, buffer_size - written, "]");
        if (enchant_end < 0 || written + enchant_end >= buffer_size) {
          LOG_WARN("Buffer overflow during enchantments end");
          return false;
        }
        written += enchant_end;
      }
      
      // Close slot object
      int slot_end = snprintf(buffer + written, buffer_size - written, "}");
      if (slot_end < 0 || written + slot_end >= buffer_size) {
        LOG_WARN("Buffer overflow during slot end");
        return false;
      }
      written += slot_end;
      first = false;
    }
  }

  // Close JSON structure
  int closing = snprintf(buffer + written, buffer_size - written, "]}");
  if (closing < 0 || written + closing >= buffer_size) {
    LOG_WARN("Buffer overflow during JSON closing");
    return false;
  }

  LOG_DEBUG("Inventory serialized successfully: %d bytes written", written + closing);
  return true;
}

bool inventory_deserialize(Inventory *inv, const char *buffer, u32 buffer_size) {
  if (!inv || !buffer || buffer_size == 0)
    return false;

  // Clear inventory first
  inventory_free(inv);
  inventory_init(inv);

  // Simple JSON parsing - this is a basic implementation
  // In production, use a proper JSON library
  
  // Parse total_items
  const char *total_items_str = strstr(buffer, "\"total_items\":");
  if (total_items_str) {
    inv->total_items = (u32)atoi(total_items_str + 14);
  }

  // Parse selected_hotbar
  const char *selected_hotbar_str = strstr(buffer, "\"selected_hotbar\":");
  if (selected_hotbar_str) {
    u32 hotbar = (u32)atoi(selected_hotbar_str + 18);
    if (hotbar < MAX_HOTBAR_SLOTS) {
      inv->selected_hotbar = hotbar;
    }
  }

  // Parse slots array
  const char *slots_start = strstr(buffer, "\"slots\":[");
  if (!slots_start)
    return false;

  const char *slots_end = strstr(slots_start, "]}");
  if (!slots_end)
    return false;

  const char *current = slots_start + 9; // Skip "\"slots\":["
  u32 slot_index = 0;

  while (current < slots_end && slot_index < MAX_INVENTORY_SLOTS) {
    const char *item_start = strstr(current, "{\"item_id\":");
    if (!item_start)
      break;

    const char *item_end = strstr(item_start, "}");
    if (!item_end)
      break;

    // Parse item_id
    const char *item_id_str = strstr(item_start, "\"item_id\":");
    if (item_id_str) {
      inv->slots[slot_index].item_id = (u32)atoi(item_id_str + 11);
    }

    // Parse count
    const char *count_str = strstr(item_start, "\"count\":");
    if (count_str) {
      inv->slots[slot_index].count = (u16)atoi(count_str + 8);
    }

    // Parse durability
    const char *durability_str = strstr(item_start, "\"durability\":");
    if (durability_str) {
      inv->slots[slot_index].durability = (f32)atof(durability_str + 13);
    }

    // Parse spoil_progress
    const char *spoil_str = strstr(item_start, "\"spoil_progress\":");
    if (spoil_str) {
      inv->slots[slot_index].spoil_progress = (f32)atof(spoil_str + 16);
    }

    // Parse quality_modifier
    const char *quality_str = strstr(item_start, "\"quality_modifier\":");
    if (quality_str) {
      inv->slots[slot_index].quality_modifier = (f32)atof(quality_str + 19);
    }

    // Parse enchantment_count
    const char *ench_str = strstr(item_start, "\"enchantment_count\":");
    if (ench_str) {
      inv->slots[slot_index].enchantment_count = (u32)atoi(ench_str + 20);
    }

    // Parse is_favorite
    const char *fav_str = strstr(item_start, "\"is_favorite\":");
    if (fav_str) {
      inv->slots[slot_index].is_favorite = (strncmp(fav_str + 14, "true", 4) == 0);
    }

    // Parse is_locked
    const char *locked_str = strstr(item_start, "\"is_locked\":");
    if (locked_str) {
      inv->slots[slot_index].is_locked = (strncmp(locked_str + 12, "true", 4) == 0);
    }

    slot_index++;
    current = item_end + 1;
  }

  inventory_mark_dirty(inv);
  LOG_INFO("Inventory deserialized successfully");
  return true;
}
