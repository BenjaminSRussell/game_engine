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

bool inventory_add_equipment_slot(InventoryComponent *inventory, EquipmentSlot slot, u32 capacity) {
  if (!inventory || slot >= EQUIPMENT_SLOT_COUNT || inventory->equipment_slots[slot]) {
    return false;
  }
  
  inventory->equipment_slots[slot] = container_create(capacity, 0.0f);
  return inventory->equipment_slots[slot] != NULL;
}

bool inventory_equip_item(InventoryComponent *inventory, u32 item_id, EquipmentSlot slot) {
  if (!inventory || slot >= EQUIPMENT_SLOT_COUNT || !inventory->equipment_slots[slot]) {
    return false;
  }
  
  // Remove item from main inventory or quick slots
  ItemStack item;
  u32 source_slot;
  if (!container_find_item(inventory->main_inventory, item_id, &source_slot, &item) &&
      !container_find_item(inventory->quick_slots, item_id, &source_slot, &item)) {
    return false;
  }
  
  // Check if item can be equipped in this slot
  if (!item_can_be_equipped(&item, slot)) {
    return false;
  }
  
  // Unequip current item if any
  ItemStack current_item;
  if (container_get_item_at(inventory->equipment_slots[slot], 0, &current_item)) {
    // Move current item back to main inventory
    if (!container_add_item(inventory->main_inventory, current_item, NULL)) {
      return false;  // No space to move current item
    }
  }
  
  // Remove item from source
  Container* source_container = (source_slot < inventory->main_inventory->capacity) ? 
                                inventory->main_inventory : inventory->quick_slots;
  container_remove_item_at(source_container, source_slot);
  
  // Equip item
  return container_set_item_at(inventory->equipment_slots[slot], 0, item);
}

bool inventory_unequip_item(InventoryComponent *inventory, EquipmentSlot slot) {
  if (!inventory || slot >= EQUIPMENT_SLOT_COUNT || !inventory->equipment_slots[slot]) {
    return false;
  }
  
  ItemStack equipped_item;
  if (!container_get_item_at(inventory->equipment_slots[slot], 0, &equipped_item)) {
    return false;  // No item equipped
  }
  
  // Try to add to main inventory
  if (!container_add_item(inventory->main_inventory, equipped_item, NULL)) {
    return false;  // No space in inventory
  }
  
  // Remove from equipment slot
  container_remove_item_at(inventory->equipment_slots[slot], 0);
  return true;
}

bool inventory_get_equipped_item(InventoryComponent *inventory, EquipmentSlot slot, ItemStack* item) {
  if (!inventory || slot >= EQUIPMENT_SLOT_COUNT || !inventory->equipment_slots[slot] || !item) {
    return false;
  }
  
  return container_get_item_at(inventory->equipment_slots[slot], 0, item);
}

void inventory_update_durability(InventoryComponent *inventory, float delta_time) {
  if (!inventory) return;
  
  // Update durability for all items
  container_update_durability(inventory->main_inventory, delta_time);
  container_update_durability(inventory->quick_slots, delta_time);
  
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i]) {
      container_update_durability(inventory->equipment_slots[i], delta_time);
    }
  }
}

bool inventory_repair_item(InventoryComponent *inventory, u32 item_id, float repair_amount) {
  if (!inventory) return false;
  
  // Try to repair in main inventory
  if (container_repair_item(inventory->main_inventory, item_id, repair_amount)) {
    return true;
  }
  
  // Try quick slots
  if (container_repair_item(inventory->quick_slots, item_id, repair_amount)) {
    return true;
  }
  
  // Try equipment slots
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i] && 
        container_repair_item(inventory->equipment_slots[i], item_id, repair_amount)) {
      return true;
    }
  }
  
  return false;
}

bool inventory_stack_items(InventoryComponent *inventory, u32 item_id) {
  if (!inventory) return false;
  
  bool stacked = false;
  
  // Stack in main inventory
  stacked |= container_stack_items(inventory->main_inventory, item_id);
  
  // Stack in quick slots
  stacked |= container_stack_items(inventory->quick_slots, item_id);
  
  return stacked;
}

float inventory_get_total_weight(InventoryComponent *inventory) {
  if (!inventory) return 0.0f;
  
  float weight = 0.0f;
  
  weight += container_get_total_weight(inventory->main_inventory);
  weight += container_get_total_weight(inventory->quick_slots);
  
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i]) {
      weight += container_get_total_weight(inventory->equipment_slots[i]);
    }
  }
  
  return weight;
}

u32 inventory_get_total_value(InventoryComponent *inventory) {
  if (!inventory) return 0;
  
  u32 value = inventory->gold;
  
  value += container_get_total_value(inventory->main_inventory);
  value += container_get_total_value(inventory->quick_slots);
  
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i]) {
      value += container_get_total_value(inventory->equipment_slots[i]);
    }
  }
  
  return value;
}

bool inventory_transfer_item(InventoryComponent *source, InventoryComponent *dest, 
                           u32 item_id, u32 quantity) {
  if (!source || !dest) return false;
  
  // Find item in source inventory
  ItemStack item;
  u32 source_slot;
  Container* source_container = NULL;
  
  if (container_find_item(source->main_inventory, item_id, &source_slot, &item)) {
    source_container = source->main_inventory;
  } else if (container_find_item(source->quick_slots, item_id, &source_slot, &item)) {
    source_container = source->quick_slots;
  } else {
    return false;  // Item not found
  }
  
  // Limit quantity to available amount
  if (quantity > item.quantity) {
    quantity = item.quantity;
  }
  
  // Create transfer item stack
  ItemStack transfer_item = item;
  transfer_item.quantity = quantity;
  
  // Try to add to destination
  u32 dest_slot;
  if (!container_add_item(dest->main_inventory, transfer_item, &dest_slot)) {
    return false;  // No space in destination
  }
  
  // Remove from source
  if (quantity == item.quantity) {
    container_remove_item_at(source_container, source_slot);
  } else {
    item.quantity -= quantity;
    container_set_item_at(source_container, source_slot, item);
  }
  
  return true;
}

bool inventory_use_item(InventoryComponent *inventory, u32 item_id, u32 quantity) {
  if (!inventory) return false;
  
  // Find and consume item
  if (!inventory_remove_item(inventory, item_id, quantity)) {
    return false;
  }
  
  // Apply item effects (this would interface with other systems)
  // Placeholder for item usage logic
  return true;
}

void inventory_get_all_items(InventoryComponent *inventory, ItemStack* items, u32* count) {
  if (!inventory || !items || !count) {
    if (count) *count = 0;
    return;
  }
  
  u32 total_count = 0;
  
  // Collect items from main inventory
  u32 main_count;
  container_get_all_items(inventory->main_inventory, &items[total_count], &main_count);
  total_count += main_count;
  
  // Collect items from quick slots
  u32 quick_count;
  container_get_all_items(inventory->quick_slots, &items[total_count], &quick_count);
  total_count += quick_count;
  
  // Collect equipped items
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i]) {
      u32 equip_count;
      container_get_all_items(inventory->equipment_slots[i], &items[total_count], &equip_count);
      total_count += equip_count;
    }
  }
  
  *count = total_count;
}

bool inventory_save_state(InventoryComponent *inventory, InventorySaveData* save_data) {
  if (!inventory || !save_data) return false;
  
  save_data->gold = inventory->gold;
  save_data->main_capacity = inventory->main_inventory->capacity;
  save_data->quickslot_count = inventory->quick_slots->capacity;
  
  // Save main inventory
  container_save_state(inventory->main_inventory, &save_data->main_data);
  
  // Save quick slots
  container_save_state(inventory->quick_slots, &save_data->quickslot_data);
  
  // Save equipment slots
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (inventory->equipment_slots[i]) {
      container_save_state(inventory->equipment_slots[i], &save_data->equipment_data[i]);
      save_data->has_equipment[i] = true;
    } else {
      save_data->has_equipment[i] = false;
    }
  }
  
  return true;
}

bool inventory_load_state(InventoryComponent *inventory, const InventorySaveData* save_data) {
  if (!inventory || !save_data) return false;
  
  inventory->gold = save_data->gold;
  
  // Load main inventory
  container_load_state(inventory->main_inventory, &save_data->main_data);
  
  // Load quick slots
  container_load_state(inventory->quick_slots, &save_data->quickslot_data);
  
  // Load equipment slots
  for (int i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
    if (save_data->has_equipment[i] && inventory->equipment_slots[i]) {
      container_load_state(inventory->equipment_slots[i], &save_data->equipment_data[i]);
    }
  }
  
  return true;
}

// Helper function
static bool item_can_be_equipped(const ItemStack* item, EquipmentSlot slot) {
  // This would check item type against slot requirements
  // Placeholder implementation
  return true;
}
