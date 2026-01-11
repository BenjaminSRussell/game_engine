// inventory.c - Implementation
#include "include/gameplay/inventory/inventory.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

void inventory_destroy(InventoryComponent *inventory);

bool inventory_system_init(void) {
  LOG_INFO("Inventory system initialized");
  return true;
}

void inventory_system_shutdown(void) {
  LOG_INFO("Inventory system shutdown");
}

InventoryComponent* inventory_create(u32 main_capacity, u32 quickslot_count) {
  InventoryComponent *inventory = calloc(1, sizeof(InventoryComponent));
  if (!inventory) {
    LOG_ERROR("Failed to allocate inventory component");
    return NULL;
  }
  
  inventory->main_inventory = container_create(main_capacity, 0.0f);
  inventory->quick_slots = container_create(quickslot_count, 0.0f);
  inventory->gold = 0;
  
  if (!inventory->main_inventory || !inventory->quick_slots) {
    inventory_destroy(inventory);
    return NULL;
  }
  
  return inventory;
}

void inventory_destroy(InventoryComponent *inventory) {
  if (!inventory) return;
  
  container_destroy(inventory->main_inventory);
  container_destroy(inventory->quick_slots);
  free(inventory);
}

bool inventory_add_item(InventoryComponent *inventory, ItemStack item) {
  if (!inventory) return false;
  
  // Try to add to main inventory first
  u32 slot;
  if (container_add_item(inventory->main_inventory, item, &slot)) {
    return true;
  }
  
  // Try quick slots if main is full
  return container_add_item(inventory->quick_slots, item, NULL);
}

bool inventory_remove_item(InventoryComponent *inventory, u32 item_id, u32 quantity) {
  if (!inventory) return false;
  
  // Try main inventory first
  if (container_remove_item_by_id(inventory->main_inventory, item_id, quantity)) {
    return true;
  }
  
  // Try quick slots
  return container_remove_item_by_id(inventory->quick_slots, item_id, quantity);
}

bool inventory_has_item(InventoryComponent *inventory, u32 item_id, u32 quantity) {
  if (!inventory) return false;
  
  u32 main_count = container_count_item(inventory->main_inventory, item_id);
  u32 quick_count = container_count_item(inventory->quick_slots, item_id);
  
  return (main_count + quick_count) >= quantity;
}

u32 inventory_count_item(InventoryComponent *inventory, u32 item_id) {
  if (!inventory) return 0;
  
  u32 main_count = container_count_item(inventory->main_inventory, item_id);
  u32 quick_count = container_count_item(inventory->quick_slots, item_id);
  
  return main_count + quick_count;
}

Container* inventory_get_main_container(InventoryComponent *inventory) {
  return inventory ? inventory->main_inventory : NULL;
}

Container* inventory_get_quickslot_container(InventoryComponent *inventory) {
  return inventory ? inventory->quick_slots : NULL;
}

u32 inventory_get_gold(InventoryComponent *inventory) {
  return inventory ? inventory->gold : 0;
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

void inventory_sort(InventoryComponent *inventory) {
  if (!inventory) return;
  
  container_sort_by_type(inventory->main_inventory);
  container_sort_by_type(inventory->quick_slots);
}
