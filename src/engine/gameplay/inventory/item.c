// item.c - Implementation
#include "include/gameplay/inventory/item.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

static ItemDatabase g_item_database = {0};

bool item_database_init(void) {
  memset(&g_item_database, 0, sizeof(ItemDatabase));
  
  LOG_INFO("Item database initialized");
  return true;
}

void item_database_shutdown(void) {
  memset(&g_item_database, 0, sizeof(ItemDatabase));
  LOG_INFO("Item database shutdown");
}

u32 item_database_add_item(const ItemDefinition *item_def) {
  if (!item_def || g_item_database.item_count >= MAX_ITEMS) {
    return 0;
  }
  
  u32 item_id = ++g_item_database.item_count;
  g_item_database.items[item_id - 1] = *item_def;
  g_item_database.items[item_id - 1].id = item_id;
  
  LOG_INFO("Added item to database: %s (ID: %d)", item_def->name, item_id);
  return item_id;
}

const ItemDefinition* item_database_get_item(u32 item_id) {
  if (item_id == 0 || item_id > g_item_database.item_count) {
    return NULL;
  }
  
  return &g_item_database.items[item_id - 1];
}

const ItemDefinition* item_database_find_by_name(const char *name) {
  if (!name) return NULL;
  
  for (u32 i = 0; i < g_item_database.item_count; i++) {
    if (strcmp(g_item_database.items[i].name, name) == 0) {
      return &g_item_database.items[i];
    }
  }
  
  return NULL;
}

u32 item_database_get_max_stack(u32 item_id) {
  const ItemDefinition *item = item_database_get_item(item_id);
  return item ? item->max_stack : 1;
}

f32 item_database_get_weight(u32 item_id) {
  const ItemDefinition *item = item_database_get_item(item_id);
  return item ? item->weight : 0.0f;
}

u32 item_database_get_value(u32 item_id) {
  const ItemDefinition *item = item_database_get_item(item_id);
  return item ? item->value : 0;
}

ItemStack item_create_stack(u32 item_id, u32 quantity) {
  ItemStack stack = {0};
  
  const ItemDefinition *item_def = item_database_get_item(item_id);
  if (!item_def) {
    LOG_ERROR("Invalid item ID: %d", item_id);
    return stack;
  }
  
  u32 max_stack = item_def->max_stack;
  if (quantity > max_stack) {
    quantity = max_stack;
    LOG_WARN("Item quantity exceeds max stack, clamping to %d", max_stack);
  }
  
  stack.item_id = item_id;
  stack.quantity = quantity;
  stack.durability = item_def->max_durability;
  
  return stack;
}

bool item_stack_is_valid(const ItemStack *stack) {
  return stack && stack->item_id != 0 && stack->quantity > 0;
}

bool item_stack_can_stack(const ItemStack *stack1, const ItemStack *stack2) {
  if (!item_stack_is_valid(stack1) || !item_stack_is_valid(stack2)) {
    return false;
  }
  
  return stack1->item_id == stack2->item_id && 
         stack1->durability == stack2->durability;
}

bool item_stack_merge(ItemStack *dest, ItemStack *source) {
  if (!dest || !source || !item_stack_can_stack(dest, source)) {
    return false;
  }
  
  u32 max_stack = item_database_get_max_stack(dest->item_id);
  u32 can_add = max_stack - dest->quantity;
  u32 transfer = (source->quantity < can_add) ? source->quantity : can_add;
  
  dest->quantity += transfer;
  source->quantity -= transfer;
  
  if (source->quantity == 0) {
    source->item_id = 0;
    source->durability = 0;
  }
  
  return transfer > 0;
}

bool item_stack_split(ItemStack *source, u32 amount, ItemStack *out_split) {
  if (!source || !out_split || amount == 0) {
    return false;
  }
  
  if (source->quantity <= amount) {
    *out_split = *source;
    memset(source, 0, sizeof(ItemStack));
    return true;
  }
  
  out_split->item_id = source->item_id;
  out_split->quantity = amount;
  out_split->durability = source->durability;
  
  source->quantity -= amount;
  return true;
}

bool item_stack_damage(ItemStack *stack, u32 damage) {
  if (!stack || !item_stack_is_valid(stack)) {
    return false;
  }
  
  const ItemDefinition *item_def = item_database_get_item(stack->item_id);
  if (!item_def || item_def->max_durability == 0) {
    return false; // Item cannot be damaged
  }
  
  if (stack->durability <= damage) {
    stack->durability = 0;
    stack->quantity = 0;
    stack->item_id = 0;
    return true; // Item destroyed
  }
  
  stack->durability -= damage;
  return false; // Item survived
}

f32 item_stack_get_condition_percentage(const ItemStack *stack) {
  if (!stack || !item_stack_is_valid(stack)) {
    return 0.0f;
  }
  
  const ItemDefinition *item_def = item_database_get_item(stack->item_id);
  if (!item_def || item_def->max_durability == 0) {
    return 100.0f; // Item cannot be damaged
  }
  
  return (f32)stack->durability / item_def->max_durability * 100.0f;
}
