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
#include "../../engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

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

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS - 1; i++) {
    for (u32 j = i + 1; j < MAX_INVENTORY_SLOTS; j++) {
      if (inv->slots[i].item_id == 0 && inv->slots[j].item_id != 0) {
        InventorySlot temp = inv->slots[i];
        inv->slots[i] = inv->slots[j];
        inv->slots[j] = temp;
      }
    }
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
    return false;
  }

  InventorySlot *source = &inv->slots[source_slot];
  InventorySlot *target = &inv->slots[target_slot];

  if (source->item_id == 0 || amount == 0 || amount > source->count)
    return false;

  // Check if target slot is empty or can stack
  if (target->item_id != 0 &&
      !inventory_can_stack_items(source->item_id, target->item_id)) {
    return false;
  }

  u16 max_stack = inventory_get_max_stack_size(source->item_id);
  u16 available_space = max_stack - target->count;
  u16 actual_move = (amount < available_space) ? amount : available_space;

  if (target->item_id == 0) {
    target->item_id = source->item_id;
    target->count = actual_move;
    target->durability = source->durability;
  } else {
    target->count += actual_move;
  }

  source->count -= actual_move;
  if (source->count == 0) {
    source->item_id = 0;
  }

  inventory_mark_dirty(inv);
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
    return false;
  }

  if (source_slot == target_slot)
    return true;

  InventorySlot *source = &inv->slots[source_slot];
  InventorySlot *target = &inv->slots[target_slot];

  // If target is empty, just move
  if (target->item_id == 0) {
    *target = *source;
    memset(source, 0, sizeof(InventorySlot));
    inventory_mark_dirty(inv);
    return true;
  }

  // Try to stack if possible
  if (inventory_merge_stacks(inv, source_slot, target_slot)) {
    return true;
  }

  // Otherwise swap
  return inventory_swap_items(inv, source_slot, target_slot);
}

bool inventory_swap_items(Inventory *inv, u32 slot1, u32 slot2) {
  if (!inv || slot1 >= MAX_INVENTORY_SLOTS || slot2 >= MAX_INVENTORY_SLOTS) {
    return false;
  }

  if (slot1 == slot2)
    return true;

  InventorySlot temp = inv->slots[slot1];
  inv->slots[slot1] = inv->slots[slot2];
  inv->slots[slot2] = temp;

  inventory_mark_dirty(inv);
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
  if (!inv)
    return false;

  u32 total_count = 0;

  for (u32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
    InventorySlot *slot = &inv->slots[i];

    if (slot->item_id != 0) {
      // Validate count
      u16 max_stack = inventory_get_max_stack_size(slot->item_id);
      if (slot->count == 0 || slot->count > max_stack) {
        LOG_WARN("Invalid item count %u in slot %u for item %u (max: %u)",
                 slot->count, i, slot->item_id, max_stack);
        return false;
      }

      total_count += slot->count;
    }
  }

  if (total_count != inv->total_items) {
    LOG_WARN("Inventory total count mismatch: calculated %u, stored %u",
             total_count, inv->total_items);
    inv->total_items = total_count; // Auto-correct
  }

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
