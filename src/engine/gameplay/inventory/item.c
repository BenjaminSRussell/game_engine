// item.c - Implementation
#include "include/gameplay/inventory/item_database.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

static Item *g_items = NULL;
static u32 g_item_count = 0;
static u32 g_item_capacity = 0;

static u32 item_get_max_stack(const Item *item) {
  if (!item) {
    return 1;
  }
  return item->max_stack_size > 0 ? item->max_stack_size : 1;
}

static u32 item_get_max_durability(const Item *item) {
  if (!item) {
    return 0;
  }

  switch (item->type) {
  case ITEM_TYPE_WEAPON:
    return item->data.weapon.durability;
  case ITEM_TYPE_ARMOR:
    return item->data.armor.durability;
  default:
    return 0;
  }
}

bool item_database_init(u32 max_items) {
  if (g_items) {
    return true;
  }

  g_item_capacity = max_items > 0 ? max_items : 1024;
  g_items = calloc(g_item_capacity, sizeof(Item));
  if (!g_items) {
    g_item_capacity = 0;
    LOG_ERROR("Failed to allocate item database");
    return false;
  }

  g_item_count = 0;
  LOG_INFO("Item database initialized (capacity: %u)", g_item_capacity);
  return true;
}

void item_database_shutdown(void) {
  free(g_items);
  g_items = NULL;
  g_item_count = 0;
  g_item_capacity = 0;
  LOG_INFO("Item database shutdown");
}

bool item_database_register(const Item *item) {
  if (!item || !g_items || g_item_count >= g_item_capacity) {
    return false;
  }

  Item stored = *item;
  stored.id = g_item_count + 1;
  g_items[g_item_count++] = stored;
  return true;
}

bool item_database_load_from_json(const char *filepath) {
  (void)filepath;
  LOG_WARN("item_database_load_from_json not implemented");
  return false;
}

const Item *item_database_get(u32 item_id) {
  if (item_id == 0 || item_id > g_item_count) {
    return NULL;
  }
  return &g_items[item_id - 1];
}

const Item *item_database_get_by_name(const char *name) {
  if (!name) {
    return NULL;
  }

  for (u32 i = 0; i < g_item_count; i++) {
    if (g_items[i].name && strcmp(g_items[i].name, name) == 0) {
      return &g_items[i];
    }
  }

  return NULL;
}

u32 item_database_get_max_stack(u32 item_id) {
  const Item *item = item_database_get(item_id);
  return item ? item_get_max_stack(item) : 1;
}

f32 item_database_get_weight(u32 item_id) {
  const Item *item = item_database_get(item_id);
  return item ? item->weight : 0.0f;
}

u32 item_database_get_value(u32 item_id) {
  const Item *item = item_database_get(item_id);
  return item ? item->value : 0;
}

u32 item_database_get_items_by_type(ItemType type, const Item **out_items,
                                    u32 max_items) {
  u32 count = 0;

  for (u32 i = 0; i < g_item_count; i++) {
    if (g_items[i].type == type) {
      if (out_items && count < max_items) {
        out_items[count] = &g_items[i];
      }
      count++;
      if (out_items && count >= max_items) {
        break;
      }
    }
  }

  return count;
}

u32 item_database_get_items_by_rarity(ItemRarity rarity, const Item **out_items,
                                      u32 max_items) {
  u32 count = 0;

  for (u32 i = 0; i < g_item_count; i++) {
    if (g_items[i].rarity == rarity) {
      if (out_items && count < max_items) {
        out_items[count] = &g_items[i];
      }
      count++;
      if (out_items && count >= max_items) {
        break;
      }
    }
  }

  return count;
}

u32 item_database_get_count(void) { return g_item_count; }

bool item_database_exists(u32 item_id) {
  return item_id > 0 && item_id <= g_item_count;
}

ItemStack item_create_stack(u32 item_id, u32 quantity) {
  ItemStack stack = {0};
  const Item *item = item_database_get(item_id);
  if (!item) {
    LOG_ERROR("Invalid item ID: %u", item_id);
    return stack;
  }

  u32 max_stack = item_get_max_stack(item);
  if (quantity > max_stack) {
    quantity = max_stack;
  }

  stack.item_id = item->id;
  stack.item = item;
  stack.quantity = quantity;
  stack.durability = item_get_max_durability(item);
  stack.instance_id = 0;
  stack.is_equipped = false;
  return stack;
}

ItemStack item_create_stack_by_name(const char *name, u32 quantity) {
  const Item *item = item_database_get_by_name(name);
  if (!item) {
    ItemStack empty = {0};
    return empty;
  }
  return item_create_stack(item->id, quantity);
}

ItemStack item_stack_clone(const ItemStack *stack) {
  ItemStack copy = {0};
  if (!stack) {
    return copy;
  }
  copy = *stack;
  return copy;
}

void item_database_register_defaults(void) {}

bool item_is_stackable(const Item *item) {
  return item && (item->flags & ITEM_FLAG_STACKABLE) &&
         item_get_max_stack(item) > 1;
}

bool item_can_stack_with(const ItemStack *a, const ItemStack *b) {
  if (!item_stack_is_valid(a) || !item_stack_is_valid(b)) {
    return false;
  }
  return a->item_id == b->item_id && a->durability == b->durability;
}

u32 item_stack_merge(ItemStack *dest, ItemStack *src) {
  if (!dest || !src || !item_can_stack_with(dest, src)) {
    return 0;
  }

  u32 max_stack = item_database_get_max_stack(dest->item_id);
  u32 space = max_stack > dest->quantity ? max_stack - dest->quantity : 0;
  u32 transfer = src->quantity < space ? src->quantity : space;

  dest->quantity += transfer;
  src->quantity -= transfer;

  if (src->quantity == 0) {
    src->item_id = 0;
    src->item = NULL;
    src->durability = 0;
    src->instance_id = 0;
    src->is_equipped = false;
  }

  return transfer;
}

bool item_stack_split(ItemStack *source, ItemStack *dest, u32 amount) {
  if (!source || !dest || !item_stack_is_valid(source) || amount == 0 ||
      amount > source->quantity) {
    return false;
  }

  *dest = *source;
  dest->quantity = amount;
  dest->item_id = source->item_id;
  dest->instance_id = 0;
  dest->is_equipped = false;

  source->quantity -= amount;
  if (source->quantity == 0) {
    source->item_id = 0;
    source->item = NULL;
    source->durability = 0;
    source->instance_id = 0;
    source->is_equipped = false;
  }

  return true;
}

bool item_equals(const Item *a, const Item *b) {
  if (!a || !b) {
    return false;
  }
  return a->id == b->id;
}

const char *item_type_to_string(ItemType type) {
  switch (type) {
  case ITEM_TYPE_WEAPON:
    return "Weapon";
  case ITEM_TYPE_ARMOR:
    return "Armor";
  case ITEM_TYPE_CONSUMABLE:
    return "Consumable";
  case ITEM_TYPE_MATERIAL:
    return "Material";
  case ITEM_TYPE_QUEST:
    return "Quest";
  case ITEM_TYPE_TOOL:
    return "Tool";
  case ITEM_TYPE_MISC:
    return "Misc";
  default:
    return "Unknown";
  }
}

const char *item_rarity_to_string(ItemRarity rarity) {
  switch (rarity) {
  case ITEM_RARITY_COMMON:
    return "Common";
  case ITEM_RARITY_UNCOMMON:
    return "Uncommon";
  case ITEM_RARITY_RARE:
    return "Rare";
  case ITEM_RARITY_EPIC:
    return "Epic";
  case ITEM_RARITY_LEGENDARY:
    return "Legendary";
  default:
    return "Unknown";
  }
}

bool item_is_valid(const Item *item) { return item && item->id != 0; }

bool item_stack_is_valid(const ItemStack *stack) {
  return stack && stack->item_id != 0 && stack->quantity > 0;
}
