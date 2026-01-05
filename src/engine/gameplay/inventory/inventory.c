#include "gameplay/inventory/inventory.h"
#include "gameplay/inventory/item_database.h"
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONTAINER MANAGEMENT
// ============================================================================

Container *container_create(u32 capacity, f32 max_weight) {
  Container *container =
      (Container *)MALLOC_TAGGED(sizeof(Container), MEMORY_TAG_GAMEPLAY);
  if (!container)
    return NULL;

  container_init(container, capacity, max_weight);
  return container;
}

void container_destroy(Container *container) {
  if (!container)
    return;
  container_cleanup(container);
  FREE(0);
}

void container_init(Container *container, u32 capacity, f32 max_weight) {
  if (!container)
    return;

  memset(container, 0, sizeof(Container));
  container->capacity = capacity;
  container->max_weight = max_weight;
  container->slots = (ItemStack *)MALLOC_TAGGED(sizeof(ItemStack) * capacity,
                                                MEMORY_TAG_GAMEPLAY);

  if (container->slots) {
    memset(container->slots, 0, sizeof(ItemStack) * capacity);
  }
}

void container_cleanup(Container *container) {
  if (!container)
    return;

  if (container->slots) {
    FREE(0);
    container->slots = NULL;
  }
}

// ============================================================================
// ITEM OPERATIONS
// ============================================================================

bool container_add_item(Container *container, ItemStack item, u32 *out_slot) {
  if (!container || !container->slots)
    return false;
  if (!item_stack_is_valid(&item))
    return false;

  // Check weight limit
  if (container->max_weight > 0.0f) {
    f32 item_weight = item.item->weight * item.quantity;
    if (container->current_weight + item_weight > container->max_weight) {
      return false;
    }
  }

  // Try to stack with existing items
  if (item_is_stackable(item.item)) {
    for (u32 i = 0; i < container->capacity; i++) {
      if (container->slots[i].item &&
          item_can_stack_with(&container->slots[i], &item)) {

        u32 added = item_stack_merge(&container->slots[i], &item);
        container->current_weight += item.item->weight * added;

        if (item.quantity == 0) {
          if (out_slot)
            *out_slot = i;
          return true;
        }
      }
    }
  }

  // Find empty slot for remaining items
  i32 empty_slot = container_find_empty_slot(container);
  if (empty_slot < 0)
    return false;

  container->slots[empty_slot] = item;
  container->current_weight += item.item->weight * item.quantity;

  if (out_slot)
    *out_slot = (u32)empty_slot;
  return true;
}

bool container_add_item_to_slot(Container *container, u32 slot,
                                ItemStack item) {
  if (!container || !container->slots)
    return false;
  if (slot >= container->capacity)
    return false;
  if (!item_stack_is_valid(&item))
    return false;

  // If slot is empty, just place item
  if (!container->slots[slot].item) {
    container->slots[slot] = item;
    container->current_weight += item.item->weight * item.quantity;
    return true;
  }

  // Try to merge if stackable
  if (item_can_stack_with(&container->slots[slot], &item)) {
    u32 added = item_stack_merge(&container->slots[slot], &item);
    container->current_weight += item.item->weight * added;
    return item.quantity == 0;
  }

  return false;
}

bool container_remove_item(Container *container, u32 slot, u32 quantity,
                           ItemStack *out_item) {
  if (!container || !container->slots)
    return false;
  if (slot >= container->capacity)
    return false;
  if (!container->slots[slot].item)
    return false;

  ItemStack *stack = &container->slots[slot];

  if (quantity >= stack->quantity) {
    // Remove entire stack
    if (out_item)
      *out_item = *stack;
    container->current_weight -= stack->item->weight * stack->quantity;
    memset(stack, 0, sizeof(ItemStack));
    return true;
  }

  // Remove partial stack
  if (out_item) {
    *out_item = *stack;
    out_item->quantity = quantity;
  }

  stack->quantity -= quantity;
  container->current_weight -= stack->item->weight * quantity;
  return true;
}

bool container_remove_item_by_id(Container *container, u32 item_id,
                                 u32 quantity) {
  if (!container)
    return false;

  u32 remaining = quantity;

  for (u32 i = 0; i < container->capacity && remaining > 0; i++) {
    if (container->slots[i].item && container->slots[i].item->id == item_id) {
      u32 to_remove = (container->slots[i].quantity < remaining)
                          ? container->slots[i].quantity
                          : remaining;

      container_remove_item(container, i, to_remove, NULL);
      remaining -= to_remove;
    }
  }

  return remaining == 0;
}

bool container_swap_slots(Container *container, u32 slot_a, u32 slot_b) {
  if (!container || !container->slots)
    return false;
  if (slot_a >= container->capacity || slot_b >= container->capacity)
    return false;
  if (slot_a == slot_b)
    return true;

  ItemStack temp = container->slots[slot_a];
  container->slots[slot_a] = container->slots[slot_b];
  container->slots[slot_b] = temp;

  return true;
}

bool container_transfer(Container *src, u32 src_slot, Container *dest,
                        u32 dest_slot) {
  if (!src || !dest)
    return false;
  if (src_slot >= src->capacity || dest_slot >= dest->capacity)
    return false;
  if (!src->slots[src_slot].item)
    return false;

  ItemStack item = src->slots[src_slot];

  // Remove from source
  src->current_weight -= item.item->weight * item.quantity;
  memset(&src->slots[src_slot], 0, sizeof(ItemStack));

  // Add to destination
  bool success = container_add_item_to_slot(dest, dest_slot, item);

  if (!success) {
    // Restore to source if failed
    src->slots[src_slot] = item;
    src->current_weight += item.item->weight * item.quantity;
    return false;
  }

  return true;
}

bool container_split_stack(Container *container, u32 slot, u32 amount,
                           u32 *out_new_slot) {
  if (!container || !container->slots)
    return false;
  if (slot >= container->capacity)
    return false;

  ItemStack *source = &container->slots[slot];
  if (!source->item)
    return false;
  if (source->quantity <= amount)
    return false;

  i32 empty = container_find_empty_slot(container);
  if (empty < 0)
    return false;

  ItemStack split;
  if (!item_stack_split(source, &split, amount))
    return false;

  container->slots[empty] = split;
  if (out_new_slot)
    *out_new_slot = (u32)empty;

  return true;
}

// ============================================================================
// QUERIES
// ============================================================================

bool container_has_item(const Container *container, u32 item_id, u32 quantity) {
  return container_count_item(container, item_id) >= quantity;
}

u32 container_count_item(const Container *container, u32 item_id) {
  if (!container || !container->slots)
    return 0;

  u32 total = 0;
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item && container->slots[i].item->id == item_id) {
      total += container->slots[i].quantity;
    }
  }

  return total;
}

i32 container_find_item(const Container *container, u32 item_id) {
  if (!container || !container->slots)
    return -1;

  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item && container->slots[i].item->id == item_id) {
      return (i32)i;
    }
  }

  return -1;
}

i32 container_find_empty_slot(const Container *container) {
  if (!container || !container->slots)
    return -1;

  for (u32 i = 0; i < container->capacity; i++) {
    if (!container->slots[i].item) {
      return (i32)i;
    }
  }

  return -1;
}

i32 container_find_stackable_slot(const Container *container, u32 item_id) {
  if (!container || !container->slots)
    return -1;

  const Item *item = item_database_get(item_id);
  if (!item || !item_is_stackable(item))
    return -1;

  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item && container->slots[i].item->id == item_id &&
        container->slots[i].quantity < item->max_stack_size) {
      return (i32)i;
    }
  }

  return -1;
}

bool container_is_full(const Container *container) {
  return container_find_empty_slot(container) < 0;
}

bool container_is_empty(const Container *container) {
  if (!container || !container->slots)
    return true;

  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item)
      return false;
  }

  return true;
}

u32 container_get_used_slots(const Container *container) {
  if (!container || !container->slots)
    return 0;

  u32 used = 0;
  for (u32 i = 0; i < container->capacity; i++) {
    if (container->slots[i].item)
      used++;
  }

  return used;
}

f32 container_get_weight(const Container *container) {
  return container ? container->current_weight : 0.0f;
}

// ============================================================================
// SORTING & ORGANIZATION
// ============================================================================

static int compare_by_type(const void *a, const void *b) {
  const ItemStack *sa = (const ItemStack *)a;
  const ItemStack *sb = (const ItemStack *)b;

  if (!sa->item && !sb->item)
    return 0;
  if (!sa->item)
    return 1;
  if (!sb->item)
    return -1;

  return (int)sa->item->type - (int)sb->item->type;
}

static int compare_by_rarity(const void *a, const void *b) {
  const ItemStack *sa = (const ItemStack *)a;
  const ItemStack *sb = (const ItemStack *)b;

  if (!sa->item && !sb->item)
    return 0;
  if (!sa->item)
    return 1;
  if (!sb->item)
    return -1;

  return (int)sb->item->rarity - (int)sa->item->rarity; // Descending
}

static int compare_by_value(const void *a, const void *b) {
  const ItemStack *sa = (const ItemStack *)a;
  const ItemStack *sb = (const ItemStack *)b;

  if (!sa->item && !sb->item)
    return 0;
  if (!sa->item)
    return 1;
  if (!sb->item)
    return -1;

  return (int)sb->item->value - (int)sa->item->value; // Descending
}

void container_sort_by_type(Container *container) {
  if (!container || !container->slots)
    return;
  qsort(container->slots, container->capacity, sizeof(ItemStack),
        compare_by_type);
}

void container_sort_by_rarity(Container *container) {
  if (!container || !container->slots)
    return;
  qsort(container->slots, container->capacity, sizeof(ItemStack),
        compare_by_rarity);
}

void container_sort_by_value(Container *container) {
  if (!container || !container->slots)
    return;
  qsort(container->slots, container->capacity, sizeof(ItemStack),
        compare_by_value);
}

void container_consolidate_stacks(Container *container) {
  if (!container || !container->slots)
    return;

  for (u32 i = 0; i < container->capacity; i++) {
    if (!container->slots[i].item)
      continue;
    if (!item_is_stackable(container->slots[i].item))
      continue;

    for (u32 j = i + 1; j < container->capacity; j++) {
      if (item_can_stack_with(&container->slots[i], &container->slots[j])) {
        item_stack_merge(&container->slots[i], &container->slots[j]);

        if (container->slots[j].quantity == 0) {
          memset(&container->slots[j], 0, sizeof(ItemStack));
        }
      }
    }
  }
}

void container_clear(Container *container) {
  if (!container || !container->slots)
    return;

  memset(container->slots, 0, sizeof(ItemStack) * container->capacity);
  container->current_weight = 0.0f;
}

// ============================================================================
// INVENTORY COMPONENT
// ============================================================================

InventoryComponent inventory_component_create(u32 main_capacity,
                                              u32 quickslot_count) {
  InventoryComponent inventory = {0};

  inventory.main_inventory = container_create(main_capacity, 0.0f);
  inventory.quick_slots = container_create(quickslot_count, 0.0f);
  inventory.gold = 0;

  return inventory;
}

void inventory_component_destroy(InventoryComponent *inventory) {
  if (!inventory)
    return;

  if (inventory->main_inventory) {
    container_destroy(inventory->main_inventory);
  }
  if (inventory->quick_slots) {
    container_destroy(inventory->quick_slots);
  }

  memset(inventory, 0, sizeof(InventoryComponent));
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
  if (!inventory || inventory->gold < amount)
    return false;
  inventory->gold -= amount;
  return true;
}
