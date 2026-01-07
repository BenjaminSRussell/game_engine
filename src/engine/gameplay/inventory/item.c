#include "gameplay/inventory/item.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ITEM UTILITIES
// ============================================================================

bool item_is_stackable(const Item *item) {
  if (!item)
    return false;
  return (item->flags & ITEM_FLAG_STACKABLE) && item->max_stack_size > 1;
}

bool item_can_stack_with(const ItemStack *a, const ItemStack *b) {
  if (!a || !b || !a->item || !b->item)
    return false;

  // Must be same item
  if (a->item->id != b->item->id)
    return false;

  // Must be stackable
  if (!item_is_stackable(a->item))
    return false;

  // Neither can be equipped
  if (a->is_equipped || b->is_equipped)
    return false;

  // For equipment, durability must match (or we don't stack damaged items)
  if (a->item->type == ITEM_TYPE_WEAPON || a->item->type == ITEM_TYPE_ARMOR) {
    if (a->durability != b->durability)
      return false;
  }

  return true;
}

u32 item_stack_merge(ItemStack *dest, ItemStack *src) {
  if (!dest || !src || !dest->item || !src->item)
    return 0;
  if (!item_can_stack_with(dest, src))
    return 0;

  u32 max_stack = dest->item->max_stack_size;
  u32 space = max_stack - dest->quantity;
  u32 to_move = (src->quantity < space) ? src->quantity : space;

  dest->quantity += to_move;
  src->quantity -= to_move;

  return to_move;
}

bool item_stack_split(ItemStack *source, ItemStack *dest, u32 amount) {
  if (!source || !dest || !source->item)
    return false;
  if (source->quantity <= amount)
    return false;

  // Create new stack with split amount
  *dest = *source;
  dest->quantity = amount;
  dest->instance_id = 0; // Will be assigned by inventory system

  source->quantity -= amount;

  return true;
}

bool item_equals(const Item *a, const Item *b) {
  if (!a || !b)
    return false;
  return a->id == b->id;
}

const char *item_type_to_string(ItemType type) {
  static const char *names[] = {"Weapon", "Armor", "Consumable", "Material",
                                "Quest",  "Tool",  "Misc"};

  if (type >= 0 && type < ITEM_TYPE_COUNT) {
    return names[type];
  }
  return "Unknown";
}

const char *item_rarity_to_string(ItemRarity rarity) {
  static const char *names[] = {"Common", "Uncommon", "Rare", "Epic",
                                "Legendary"};

  if (rarity >= 0 && rarity < ITEM_RARITY_COUNT) {
    return names[rarity];
  }
  return "Unknown";
}

bool item_is_valid(const Item *item) {
  if (!item)
    return false;
  if (!item->name || item->name[0] == '\0')
    return false;
  if (item->max_stack_size == 0)
    return false;
  return true;
}

bool item_stack_is_valid(const ItemStack *stack) {
  if (!stack)
    return false;
  if (!item_is_valid(stack->item))
    return false;
  if (stack->quantity == 0)
    return false;
  if (stack->quantity > stack->item->max_stack_size)
    return false;
  return true;
}
