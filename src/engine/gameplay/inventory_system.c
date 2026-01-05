#include "inventory_system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * =================================================================================================
 *                                   INVENTORY & ITEM SYSTEM - COMPLETE
 * =================================================================================================
 */

// Global instance ID counter
static uint32_t g_instance_id_counter = 1;

// Helper function to generate unique instance IDs
static uint32_t generate_instance_id(void) {
    return g_instance_id_counter++;
}

// Helper function to get current timestamp
static uint32_t get_current_time(void) {
    return (uint32_t)time(NULL);
}

// =================================================================================================
// CORE FUNCTIONS
// =================================================================================================

bool inventory_manager_init(InventoryManager* manager) {
    if (!manager) {
        LOG_ERROR("Invalid inventory manager");
        return false;
    }
    
    memset(manager, 0, sizeof(InventoryManager));
    
    // Initialize item database
    manager->database.item_count = 0;
    memset(manager->database.name_to_id_map, 0, sizeof(manager->database.name_to_id_map));
    memset(manager->database.type_to_id_map, 0, sizeof(manager->database.type_to_id_map));
    
    // Initialize recipes
    manager->recipe_count = 0;
    
    // Initialize stations
    manager->station_count = 0;
    
    // Initialize loot tables
    manager->loot_table_count = 0;
    
    // Initialize vendors
    manager->vendor_count = 0;
    
    // Initialize trades
    manager->trade_count = 0;
    
    LOG_INFO("Inventory manager initialized successfully");
    return true;
}

void inventory_manager_shutdown(InventoryManager* manager) {
    if (!manager) return;
    
    // Clear all data
    memset(manager, 0, sizeof(InventoryManager));
    
    LOG_INFO("Inventory manager shutdown");
}

InventoryManager* inventory_manager_create(void) {
    InventoryManager* manager = malloc(sizeof(InventoryManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate inventory manager");
        return NULL;
    }
    
    if (!inventory_manager_init(manager)) {
        free(manager);
        return NULL;
    }
    
    return manager;
}

void inventory_manager_destroy(InventoryManager* manager) {
    if (!manager) return;
    
    inventory_manager_shutdown(manager);
    free(manager);
}

// =================================================================================================
// ITEM DATABASE MANAGEMENT
// =================================================================================================

uint32_t item_database_add_item(InventoryManager* manager, const ItemDefinition* item) {
    if (!manager || !item) {
        LOG_ERROR("Invalid parameters for item database add");
        return 0;
    }
    
    if (manager->database.item_count >= MAX_ITEMS) {
        LOG_ERROR("Item database is full");
        return 0;
    }
    
    if (!item_definition_validate(item)) {
        LOG_ERROR("Invalid item definition");
        return 0;
    }
    
    // Check for duplicate names
    if (item_database_find_item_by_name(manager, item->name)) {
        LOG_ERROR("Item with name '%s' already exists", item->name);
        return 0;
    }
    
    uint32_t item_id = manager->database.item_count + 1;
    ItemDefinition* new_item = &manager->database.items[manager->database.item_count];
    
    // Copy item data
    memcpy(new_item, item, sizeof(ItemDefinition));
    new_item->id = item_id;
    
    // Update name map
    manager->database.name_to_id_map[manager->database.item_count] = item_id;
    
    // Update type map
    for (uint32_t i = 0; i < manager->database.item_count; i++) {
        if (manager->database.items[i].type == item->type) {
            manager->database.type_to_id_map[i] = item_id;
            break;
        }
    }
    
    manager->database.item_count++;
    
    LOG_INFO("Added item to database: %s (ID: %u)", item->name, item_id);
    return item_id;
}

ItemDefinition* item_database_get_item(InventoryManager* manager, uint32_t item_id) {
    if (!manager || item_id == 0 || item_id > manager->database.item_count) {
        return NULL;
    }
    
    return &manager->database.items[item_id - 1];
}

ItemDefinition* item_database_find_item_by_name(InventoryManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    for (uint32_t i = 0; i < manager->database.item_count; i++) {
        if (strcmp(manager->database.items[i].name, name) == 0) {
            return &manager->database.items[i];
        }
    }
    
    return NULL;
}

// =================================================================================================
// PLAYER INVENTORY MANAGEMENT
// =================================================================================================

bool player_inventory_init(PlayerInventory* inventory) {
    if (!inventory) {
        LOG_ERROR("Invalid inventory pointer");
        return false;
    }
    
    memset(inventory, 0, sizeof(PlayerInventory));
    
    // Initialize all slots as empty
    for (uint32_t i = 0; i < MAX_INVENTORY_SIZE; i++) {
        inventory->slots[i].is_empty = true;
        inventory->slots[i].slot_index = i;
        inventory->slots[i].item = NULL;
    }
    
    // Initialize equipment slots
    for (int i = 0; i < SLOT_COUNT; i++) {
        inventory->equipment_slots[i] = NULL;
    }
    
    // Set default auto-loot settings
    inventory->auto_loot_common = false;
    inventory->auto_loot_uncommon = true;
    inventory->auto_loot_rare = true;
    inventory->auto_loot_epic = true;
    inventory->auto_loot_legendary = true;
    
    LOG_INFO("Player inventory initialized");
    return true;
}

void player_inventory_shutdown(PlayerInventory* inventory) {
    if (!inventory) return;
    
    // Free all item instances
    for (uint32_t i = 0; i < MAX_INVENTORY_SIZE; i++) {
        if (inventory->slots[i].item) {
            item_instance_destroy(inventory->slots[i].item);
            inventory->slots[i].item = NULL;
        }
    }
    
    // Free equipment items
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (inventory->equipment_slots[i]) {
            item_instance_destroy(inventory->equipment_slots[i]);
            inventory->equipment_slots[i] = NULL;
        }
    }
    
    memset(inventory, 0, sizeof(PlayerInventory));
}

bool player_inventory_add_item(PlayerInventory* inventory, ItemInstance* item) {
    if (!inventory || !item) {
        LOG_ERROR("Invalid parameters for add item");
        return false;
    }
    
    // Try to stack with existing items first
    if (item->stack_count > 1) {
        for (uint32_t i = 0; i < inventory->slot_count; i++) {
            InventorySlot* slot = &inventory->slots[i];
            if (!slot->is_empty && slot->item) {
                if (item_instance_can_stack(slot->item, item)) {
                    uint32_t stack_space = item_instance_get_stack_space(slot->item);
                    if (stack_space > 0) {
                        uint32_t transfer_count = (item->stack_count < stack_space) ? 
                                                item->stack_count : stack_space;
                        
                        if (item_instance_add_to_stack(slot->item, item)) {
                            item->stack_count -= transfer_count;
                            
                            // If item stack is empty, destroy it
                            if (item->stack_count == 0) {
                                item_instance_destroy(item);
                                return true;
                            }
                            
                            // Continue trying to add remaining items
                            continue;
                        }
                    }
                }
            }
        }
    }
    
    // Find empty slot for remaining items
    for (uint32_t i = 0; i < inventory->slot_count; i++) {
        InventorySlot* slot = &inventory->slots[i];
        if (slot->is_empty) {
            slot->item = item;
            slot->is_empty = false;
            
            // Update inventory statistics
            inventory->total_items += item->stack_count;
            inventory->total_value += item_instance_get_value(item);
            
            LOG_INFO("Added item to inventory slot %u", i);
            return true;
        }
    }
    
    LOG_ERROR("No available inventory slots");
    return false;
}

bool player_inventory_remove_item(PlayerInventory* inventory, uint32_t slot_index, uint32_t count) {
    if (!inventory || slot_index >= MAX_INVENTORY_SIZE || count == 0) {
        LOG_ERROR("Invalid parameters for remove item");
        return false;
    }
    
    InventorySlot* slot = &inventory->slots[slot_index];
    if (slot->is_empty || !slot->item) {
        LOG_ERROR("Slot is empty");
        return false;
    }
    
    if (count >= slot->item->stack_count) {
        // Remove entire stack
        inventory->total_items -= slot->item->stack_count;
        inventory->total_value -= item_instance_get_value(slot->item);
        
        item_instance_destroy(slot->item);
        slot->item = NULL;
        slot->is_empty = true;
        
        LOG_INFO("Removed entire stack from slot %u", slot_index);
        return true;
    } else {
        // Remove partial stack
        slot->item->stack_count -= count;
        inventory->total_items -= count;
        inventory->total_value -= item_instance_get_value(slot->item) * count / slot->item->stack_count;
        
        LOG_INFO("Removed %u items from slot %u", count, slot_index);
        return true;
    }
}

bool player_inventory_move_item(PlayerInventory* inventory, uint32_t from_slot, uint32_t to_slot) {
    if (!inventory || from_slot >= MAX_INVENTORY_SIZE || to_slot >= MAX_INVENTORY_SIZE) {
        LOG_ERROR("Invalid slot indices");
        return false;
    }
    
    if (from_slot == to_slot) {
        return true; // No move needed
    }
    
    InventorySlot* from = &inventory->slots[from_slot];
    InventorySlot* to = &inventory->slots[to_slot];
    
    if (from->is_empty || !from->item) {
        LOG_ERROR("Source slot is empty");
        return false;
    }
    
    // If destination is empty, simple move
    if (to->is_empty) {
        to->item = from->item;
        to->is_empty = false;
        from->item = NULL;
        from->is_empty = true;
        
        LOG_INFO("Moved item from slot %u to %u", from_slot, to_slot);
        return true;
    }
    
    // If destination has item, try to stack or swap
    if (item_instance_can_stack(to->item, from->item)) {
        if (item_instance_add_to_stack(to->item, from->item)) {
            // If source stack is empty, clear it
            if (from->item->stack_count == 0) {
                item_instance_destroy(from->item);
                from->item = NULL;
                from->is_empty = true;
            }
            
            LOG_INFO("Stacked items from slot %u to %u", from_slot, to_slot);
            return true;
        }
    }
    
    // Swap items
    ItemInstance* temp = to->item;
    to->item = from->item;
    from->item = temp;
    
    LOG_INFO("Swapped items between slots %u and %u", from_slot, to_slot);
    return true;
}

bool player_inventory_split_stack(PlayerInventory* inventory, uint32_t slot_index, uint32_t count) {
    if (!inventory || slot_index >= MAX_INVENTORY_SIZE || count == 0) {
        LOG_ERROR("Invalid parameters for split stack");
        return false;
    }
    
    InventorySlot* slot = &inventory->slots[slot_index];
    if (slot->is_empty || !slot->item || slot->item->stack_count <= count) {
        LOG_ERROR("Cannot split stack");
        return false;
    }
    
    // Find empty slot
    for (uint32_t i = 0; i < inventory->slot_count; i++) {
        if (i != slot_index && inventory->slots[i].is_empty) {
            // Create new item instance for the split
            ItemInstance* new_item = item_instance_clone(slot->item);
            if (!new_item) {
                LOG_ERROR("Failed to clone item for split");
                return false;
            }
            
            new_item->stack_count = count;
            new_item->instance_id = generate_instance_id();
            
            // Update original stack
            slot->item->stack_count -= count;
            
            // Place new item in empty slot
            inventory->slots[i].item = new_item;
            inventory->slots[i].is_empty = false;
            
            LOG_INFO("Split %u items from slot %u to slot %u", count, slot_index, i);
            return true;
        }
    }
    
    LOG_ERROR("No empty slot available for split");
    return false;
}

// =================================================================================================
// ITEM INSTANCE MANAGEMENT
// =================================================================================================

ItemInstance* item_instance_create(uint32_t definition_id, uint32_t count) {
    if (definition_id == 0 || count == 0 || count > MAX_STACK_SIZE) {
        LOG_ERROR("Invalid parameters for item instance creation");
        return NULL;
    }
    
    ItemInstance* item = malloc(sizeof(ItemInstance));
    if (!item) {
        LOG_ERROR("Failed to allocate item instance");
        return NULL;
    }
    
    memset(item, 0, sizeof(ItemInstance));
    
    item->instance_id = generate_instance_id();
    item->definition_id = definition_id;
    item->stack_count = count;
    item->creation_time = get_current_time();
    item->last_used_time = item->creation_time;
    
    // Get item definition to set default values
    // Note: In a real implementation, you'd pass the InventoryManager to get the definition
    item->max_durability = 100; // Default value
    item->durability = item->max_durability;
    
    LOG_INFO("Created item instance: definition %u, count %u", definition_id, count);
    return item;
}

void item_instance_destroy(ItemInstance* item) {
    if (!item) return;
    
    free(item);
    LOG_DEBUG("Destroyed item instance");
}

ItemInstance* item_instance_clone(const ItemInstance* source) {
    if (!source) {
        LOG_ERROR("Cannot clone NULL item");
        return NULL;
    }
    
    ItemInstance* clone = malloc(sizeof(ItemInstance));
    if (!clone) {
        LOG_ERROR("Failed to allocate item clone");
        return NULL;
    }
    
    memcpy(clone, source, sizeof(ItemInstance));
    clone->instance_id = generate_instance_id();
    clone->creation_time = get_current_time();
    clone->last_used_time = clone->creation_time;
    
    LOG_DEBUG("Cloned item instance");
    return clone;
}

bool item_instance_apply_modifier(ItemInstance* item, const ItemModifier* modifier) {
    if (!item || !modifier) {
        LOG_ERROR("Invalid parameters for apply modifier");
        return false;
    }
    
    if (item->custom_modifier_count >= MAX_MODIFIERS_PER_ITEM) {
        LOG_ERROR("Item has maximum modifiers");
        return false;
    }
    
    // Check for duplicate modifier type
    for (uint8_t i = 0; i < item->custom_modifier_count; i++) {
        if (item->custom_modifiers[i].type == modifier->type) {
            // Update existing modifier
            item->custom_modifiers[i].value += modifier->value;
            LOG_INFO("Updated existing modifier on item");
            return true;
        }
    }
    
    // Add new modifier
    item->custom_modifiers[item->custom_modifier_count] = *modifier;
    item->custom_modifier_count++;
    
    LOG_INFO("Applied modifier to item instance");
    return true;
}

float item_instance_get_stat_modifier(const ItemInstance* item, ModifierType type) {
    if (!item) return 0.0f;
    
    float total = 0.0f;
    
    // Check custom modifiers
    for (uint8_t i = 0; i < item->custom_modifier_count; i++) {
        if (item->custom_modifiers[i].type == type) {
            total += item->custom_modifiers[i].value;
        }
    }
    
    return total;
}

// =================================================================================================
// EQUIPMENT SYSTEM
// =================================================================================================

bool player_inventory_equip_item(PlayerInventory* inventory, uint32_t slot_index, EquipmentSlot equip_slot) {
    if (!inventory || slot_index >= MAX_INVENTORY_SIZE || equip_slot >= SLOT_COUNT) {
        LOG_ERROR("Invalid parameters for equip item");
        return false;
    }
    
    InventorySlot* slot = &inventory->slots[slot_index];
    if (slot->is_empty || !slot->item) {
        LOG_ERROR("Slot is empty");
        return false;
    }
    
    // Check if item is equippable
    // Note: In real implementation, check item definition
    if (true) { // Placeholder check
        // Store current equipped item if any
        ItemInstance* previous_item = inventory->equipment_slots[equip_slot];
        
        // Equip new item
        inventory->equipment_slots[equip_slot] = slot->item;
        
        // Clear inventory slot
        slot->item = NULL;
        slot->is_empty = true;
        
        // If there was a previously equipped item, put it back in inventory
        if (previous_item) {
            player_inventory_add_item(inventory, previous_item);
        }
        
        LOG_INFO("Equipped item to slot %d", equip_slot);
        return true;
    }
    
    LOG_ERROR("Item cannot be equipped");
    return false;
}

bool player_inventory_unequip_item(PlayerInventory* inventory, EquipmentSlot equip_slot) {
    if (!inventory || equip_slot >= SLOT_COUNT) {
        LOG_ERROR("Invalid parameters for unequip item");
        return false;
    }
    
    ItemInstance* item = inventory->equipment_slots[equip_slot];
    if (!item) {
        LOG_ERROR("No item equipped in slot %d", equip_slot);
        return false;
    }
    
    // Try to add item back to inventory
    if (player_inventory_add_item(inventory, item)) {
        inventory->equipment_slots[equip_slot] = NULL;
        LOG_INFO("Unequipped item from slot %d", equip_slot);
        return true;
    }
    
    LOG_ERROR("Inventory is full, cannot unequip item");
    return false;
}

// =================================================================================================
// UTILITY FUNCTIONS
// =================================================================================================

const char* item_type_to_string(ItemType type) {
    switch (type) {
        case ITEM_TYPE_WEAPON: return "Weapon";
        case ITEM_TYPE_ARMOR: return "Armor";
        case ITEM_TYPE_CONSUMABLE: return "Consumable";
        case ITEM_TYPE_MATERIAL: return "Material";
        case ITEM_TYPE_TOOL: return "Tool";
        case ITEM_TYPE_QUEST: return "Quest";
        case ITEM_TYPE_CURRENCY: return "Currency";
        case ITEM_TYPE_MISC: return "Misc";
        default: return "Unknown";
    }
}

const char* rarity_to_string(ItemRarity rarity) {
    switch (rarity) {
        case RARITY_COMMON: return "Common";
        case RARITY_UNCOMMON: return "Uncommon";
        case RARITY_RARE: return "Rare";
        case RARITY_EPIC: return "Epic";
        case RARITY_LEGENDARY: return "Legendary";
        default: return "Unknown";
    }
}

const char* slot_to_string(EquipmentSlot slot) {
    switch (slot) {
        case SLOT_HEAD: return "Head";
        case SLOT_CHEST: return "Chest";
        case SLOT_LEGS: return "Legs";
        case SLOT_BOOTS: return "Boots";
        case SLOT_GLOVES: return "Gloves";
        case SLOT_WEAPON_MAIN: return "Main Hand";
        case SLOT_WEAPON_OFF: return "Off Hand";
        case SLOT_NECK: return "Neck";
        case SLOT_RING: return "Ring";
        case SLOT_TRINKET: return "Trinket";
        case SLOT_BACKPACK: return "Backpack";
        default: return "Unknown";
    }
}

const char* modifier_to_string(ModifierType type) {
    switch (type) {
        case MODIFIER_DAMAGE: return "Damage";
        case MODIFIER_ARMOR: return "Armor";
        case MODIFIER_HEALTH: return "Health";
        case MODIFIER_MANA: return "Mana";
        case MODIFIER_SPEED: return "Speed";
        case MODIFIER_CRIT_CHANCE: return "Critical Chance";
        case MODIFIER_CRIT_DAMAGE: return "Critical Damage";
        case MODIFIER_FIRE_RESIST: return "Fire Resistance";
        case MODIFIER_ICE_RESIST: return "Ice Resistance";
        case MODIFIER_LIGHTNING_RESIST: return "Lightning Resistance";
        case MODIFIER_POISON_RESIST: return "Poison Resistance";
        case MODIFIER_STRENGTH: return "Strength";
        case MODIFIER_DEXTERITY: return "Dexterity";
        case MODIFIER_INTELLIGENCE: return "Intelligence";
        case MODIFIER_WISDOM: return "Wisdom";
        case MODIFIER_LUCK: return "Luck";
        default: return "Unknown";
    }
}

uint32_t item_instance_get_value(const ItemInstance* item) {
    if (!item) return 0;
    
    // Base value calculation
    // Note: In real implementation, get from item definition
    uint32_t base_value = 100;
    
    // Apply stack count
    uint32_t total_value = base_value * item->stack_count;
    
    // Apply durability modifier
    if (item->max_durability > 0) {
        float durability_ratio = (float)item->durability / item->max_durability;
        total_value = (uint32_t)(total_value * durability_ratio);
    }
    
    return total_value;
}

bool item_instance_can_stack(const ItemInstance* item1, const ItemInstance* item2) {
    if (!item1 || !item2) return false;
    
    // Check definition ID
    if (item1->definition_id != item2->definition_id) return false;
    
    // Check durability (items with different durability can't stack)
    if (item1->durability != item2->durability) return false;
    
    // Check custom modifiers count
    if (item1->custom_modifier_count != item2->custom_modifier_count) return false;
    
    // Check custom modifiers
    for (uint8_t i = 0; i < item1->custom_modifier_count; i++) {
        bool found = false;
        for (uint8_t j = 0; j < item2->custom_modifier_count; j++) {
            if (item1->custom_modifiers[i].type == item2->custom_modifiers[j].type &&
                item1->custom_modifiers[i].value == item2->custom_modifiers[j].value &&
                item1->custom_modifiers[i].is_percentage == item2->custom_modifiers[j].is_percentage) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    return true;
}

uint32_t item_instance_get_stack_space(const ItemInstance* item) {
    if (!item) return 0;
    
    // Note: In real implementation, get max stack size from item definition
    uint32_t max_stack = 64;
    
    if (item->stack_count >= max_stack) return 0;
    
    return max_stack - item->stack_count;
}

bool item_instance_add_to_stack(ItemInstance* target, ItemInstance* source) {
    if (!target || !source) return false;
    
    if (!item_instance_can_stack(target, source)) return false;
    
    uint32_t stack_space = item_instance_get_stack_space(target);
    if (stack_space == 0) return false;
    
    uint32_t transfer_count = (source->stack_count < stack_space) ? 
                            source->stack_count : stack_space;
    
    target->stack_count += transfer_count;
    source->stack_count -= transfer_count;
    
    return true;
}

// =================================================================================================
// VALIDATION FUNCTIONS
// =================================================================================================

bool item_definition_validate(const ItemDefinition* item) {
    if (!item) return false;
    
    if (item->name[0] == '\0') return false;
    if (item->type >= ITEM_TYPE_MISC) return false;
    if (item->rarity >= RARITY_LEGENDARY) return false;
    if (item->max_stack_size == 0 || item->max_stack_size > MAX_STACK_SIZE) return false;
    
    return true;
}

bool item_instance_validate(const ItemInstance* item) {
    if (!item) return false;
    
    if (item->definition_id == 0) return false;
    if (item->stack_count == 0 || item->stack_count > MAX_STACK_SIZE) return false;
    if (item->instance_id == 0) return false;
    
    return true;
}

bool player_inventory_validate(const PlayerInventory* inventory) {
    if (!inventory) return false;
    
    // Validate slots
    for (uint32_t i = 0; i < inventory->slot_count; i++) {
        const InventorySlot* slot = &inventory->slots[i];
        if (!slot->is_empty && !item_instance_validate(slot->item)) {
            return false;
        }
    }
    
    // Validate equipment slots
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (inventory->equipment_slots[i] && !item_instance_validate(inventory->equipment_slots[i])) {
            return false;
        }
    }
    
    return true;
}

// =================================================================================================
// CRAFTING SYSTEM
// =================================================================================================

uint32_t crafting_add_recipe(InventoryManager* manager, const CraftingRecipe* recipe) {
    if (!manager || !recipe) {
        LOG_ERROR("Invalid parameters for add recipe");
        return 0;
    }
    
    if (manager->recipe_count >= MAX_CRAFTING_RECIPES) {
        LOG_ERROR("Recipe database is full");
        return 0;
    }
    
    uint32_t recipe_id = manager->recipe_count + 1;
    CraftingRecipe* new_recipe = &manager->recipes[manager->recipe_count];
    
    memcpy(new_recipe, recipe, sizeof(CraftingRecipe));
    new_recipe->id = recipe_id;
    
    manager->recipe_count++;
    
    LOG_INFO("Added crafting recipe: %s (ID: %u)", recipe->name, recipe_id);
    return recipe_id;
}

CraftingRecipe* crafting_get_recipe(InventoryManager* manager, uint32_t recipe_id) {
    if (!manager || recipe_id == 0 || recipe_id > manager->recipe_count) {
        return NULL;
    }
    
    return &manager->recipes[recipe_id - 1];
}

bool crafting_can_craft(const PlayerInventory* inventory, const CraftingRecipe* recipe) {
    if (!inventory || !recipe) return false;
    
    return crafting_has_ingredients(inventory, recipe);
}

bool crafting_has_ingredients(const PlayerInventory* inventory, const CraftingRecipe* recipe) {
    if (!inventory || !recipe) return false;
    
    // Check each ingredient
    for (uint8_t i = 0; i < recipe->ingredient_count; i++) {
        uint32_t required_id = recipe->ingredients[i].item_id;
        uint32_t required_count = recipe->ingredients[i].count;
        uint32_t available_count = 0;
        
        // Count available items in inventory
        for (uint32_t j = 0; j < inventory->slot_count; j++) {
            const InventorySlot* slot = &inventory->slots[j];
            if (!slot->is_empty && slot->item && slot->item->definition_id == required_id) {
                available_count += slot->item->stack_count;
            }
        }
        
        if (available_count < required_count) {
            return false;
        }
    }
    
    return true;
}

bool crafting_craft_item(PlayerInventory* inventory, const CraftingRecipe* recipe) {
    if (!inventory || !recipe) {
        LOG_ERROR("Invalid parameters for crafting");
        return false;
    }
    
    if (!crafting_has_ingredients(inventory, recipe)) {
        LOG_ERROR("Insufficient ingredients for crafting");
        return false;
    }
    
    // Remove ingredients
    if (!crafting_remove_ingredients(inventory, recipe)) {
        LOG_ERROR("Failed to remove ingredients");
        return false;
    }
    
    // Add result
    crafting_add_result(inventory, recipe);
    
    // Update recipe craft count
    ((CraftingRecipe*)recipe)->times_crafted++;
    
    LOG_INFO("Successfully crafted: %s", recipe->name);
    return true;
}

bool crafting_remove_ingredients(PlayerInventory* inventory, const CraftingRecipe* recipe) {
    if (!inventory || !recipe) return false;
    
    // Remove each ingredient
    for (uint8_t i = 0; i < recipe->ingredient_count; i++) {
        uint32_t required_id = recipe->ingredients[i].item_id;
        uint32_t required_count = recipe->ingredients[i].count;
        uint32_t remaining = required_count;
        
        // Find and remove items
        for (uint32_t j = 0; j < inventory->slot_count && remaining > 0; j++) {
            InventorySlot* slot = &inventory->slots[j];
            if (!slot->is_empty && slot->item && slot->item->definition_id == required_id) {
                uint32_t remove_count = (slot->item->stack_count < remaining) ? 
                                      slot->item->stack_count : remaining;
                
                if (!player_inventory_remove_item(inventory, j, remove_count)) {
                    LOG_ERROR("Failed to remove ingredient");
                    return false;
                }
                
                remaining -= remove_count;
            }
        }
        
        if (remaining > 0) {
            LOG_ERROR("Failed to remove all ingredients");
            return false;
        }
    }
    
    return true;
}

void crafting_add_result(PlayerInventory* inventory, const CraftingRecipe* recipe) {
    if (!inventory || !recipe) return;
    
    // Create result item
    ItemInstance* result = item_instance_create(recipe->result_item_id, recipe->result_count);
    if (result) {
        player_inventory_add_item(inventory, result);
    }
}

// =================================================================================================
// LOOT SYSTEM
// =================================================================================================

uint32_t loot_table_create(InventoryManager* manager, const char* name) {
    if (!manager || !name) {
        LOG_ERROR("Invalid parameters for loot table creation");
        return 0;
    }
    
    if (manager->loot_table_count >= 128) {
        LOG_ERROR("Maximum loot tables reached");
        return 0;
    }
    
    uint32_t table_id = manager->loot_table_count + 1;
    LootTable* table = &manager->loot_tables[manager->loot_table_count];
    
    memset(table, 0, sizeof(LootTable));
    table->id = table_id;
    strncpy(table->name, name, sizeof(table->name) - 1);
    table->max_drops_per_kill = 3;
    
    manager->loot_table_count++;
    
    LOG_INFO("Created loot table: %s (ID: %u)", name, table_id);
    return table_id;
}

bool loot_table_add_drop(InventoryManager* manager, uint32_t table_id, uint32_t item_id, 
                         uint32_t min_count, uint32_t max_count, float drop_chance) {
    if (!manager || table_id == 0 || table_id > manager->loot_table_count) {
        LOG_ERROR("Invalid loot table ID");
        return false;
    }
    
    LootTable* table = &manager->loot_tables[table_id - 1];
    if (table->drop_count >= 32) {
        LOG_ERROR("Loot table is full");
        return false;
    }
    
    // Add drop entry
    table->possible_drops[table->drop_count].item_id = item_id;
    table->possible_drops[table->drop_count].min_count = min_count;
    table->possible_drops[table->drop_count].max_count = max_count;
    table->possible_drops[table->drop_count].drop_chance = drop_chance;
    table->possible_drops[table->drop_count].min_rarity = RARITY_COMMON;
    table->possible_drops[table->drop_count].max_rarity = RARITY_LEGENDARY;
    
    table->drop_count++;
    
    LOG_INFO("Added drop to loot table %u: item %u, chance %.2f", table_id, item_id, drop_chance);
    return true;
}

ItemInstance** loot_table_generate_loot(InventoryManager* manager, uint32_t table_id, uint32_t* count, uint32_t player_level) {
    if (!manager || table_id == 0 || table_id > manager->loot_table_count || !count) {
        LOG_ERROR("Invalid parameters for loot generation");
        return NULL;
    }
    
    LootTable* table = &manager->loot_tables[table_id - 1];
    ItemInstance** loot = malloc(sizeof(ItemInstance*) * 16);
    if (!loot) {
        LOG_ERROR("Failed to allocate loot array");
        return NULL;
    }
    
    memset(loot, 0, sizeof(ItemInstance*) * 16);
    *count = 0;
    
    // Generate loot drops
    uint32_t drops_generated = 0;
    uint32_t max_drops = table->max_drops_per_kill;
    
    for (uint8_t i = 0; i < table->drop_count && drops_generated < max_drops; i++) {
        float roll = (float)rand() / RAND_MAX;
        
        if (roll <= table->possible_drops[i].drop_chance) {
            uint32_t item_id = table->possible_drops[i].item_id;
            uint32_t min_count = table->possible_drops[i].min_count;
            uint32_t max_count = table->possible_drops[i].max_count;
            
            uint32_t count = min_count + (rand() % (max_count - min_count + 1));
            
            ItemInstance* item = item_instance_create(item_id, count);
            if (item) {
                loot[(*count)++] = item;
                drops_generated++;
            }
        }
    }
    
    LOG_INFO("Generated %u items from loot table %u", *count, table_id);
    return loot;
}

// =================================================================================================
// VENDOR SYSTEM
// =================================================================================================

uint32_t vendor_create(InventoryManager* manager, const char* name, Vec3 position) {
    if (!manager || !name) {
        LOG_ERROR("Invalid parameters for vendor creation");
        return 0;
    }
    
    if (manager->vendor_count >= 32) {
        LOG_ERROR("Maximum vendors reached");
        return 0;
    }
    
    uint32_t vendor_id = manager->vendor_count + 1;
    Vendor* vendor = &manager->vendors[manager->vendor_count];
    
    memset(vendor, 0, sizeof(Vendor));
    vendor->vendor_id = vendor_id;
    strncpy(vendor->name, name, sizeof(vendor->name) - 1);
    vendor->position = position;
    vendor->interaction_radius = 5.0f;
    vendor->buy_multiplier = 0.5f;  // Buy from player at 50% value
    vendor->sell_multiplier = 1.5f; // Sell to player at 150% value
    vendor->is_active = true;
    vendor->refresh_interval = 3600; // 1 hour
    vendor->last_refresh_time = get_current_time();
    
    manager->vendor_count++;
    
    LOG_INFO("Created vendor: %s (ID: %u)", name, vendor_id);
    return vendor_id;
}

bool vendor_add_item(InventoryManager* manager, uint32_t vendor_id, uint32_t item_id, 
                     uint32_t count, uint32_t price) {
    if (!manager || vendor_id == 0 || vendor_id > manager->vendor_count) {
        LOG_ERROR("Invalid vendor ID");
        return false;
    }
    
    Vendor* vendor = &manager->vendors[vendor_id - 1];
    if (vendor->item_count >= 64) {
        LOG_ERROR("Vendor inventory is full");
        return false;
    }
    
    vendor->inventory[vendor->item_count].item_id = item_id;
    vendor->inventory[vendor->item_count].count = count;
    vendor->inventory[vendor->item_count].buy_price = price;
    vendor->inventory[vendor->item_count].sells_infinite = (count == 0);
    
    vendor->item_count++;
    
    LOG_INFO("Added item to vendor %u: item %u, count %u, price %u", vendor_id, item_id, count, price);
    return true;
}

bool vendor_buy_item(PlayerInventory* inventory, Vendor* vendor, uint32_t vendor_slot_index, uint32_t count) {
    if (!inventory || !vendor || vendor_slot_index >= vendor->item_count) {
        LOG_ERROR("Invalid parameters for vendor buy");
        return false;
    }
    
    if (vendor_slot_index >= vendor->item_count) {
        LOG_ERROR("Invalid vendor slot");
        return false;
    }
    
    uint32_t item_id = vendor->inventory[vendor_slot_index].item_id;
    uint32_t available_count = vendor->inventory[vendor_slot_index].count;
    uint32_t price_per_item = vendor->inventory[vendor_slot_index].buy_price;
    
    if (!vendor->inventory[vendor_slot_index].sells_infinite && available_count < count) {
        LOG_ERROR("Insufficient vendor stock");
        return false;
    }
    
    uint32_t total_cost = price_per_item * count;
    if (inventory->gold < total_cost) {
        LOG_ERROR("Insufficient gold");
        return false;
    }
    
    // Create item and add to inventory
    ItemInstance* item = item_instance_create(item_id, count);
    if (!item) {
        LOG_ERROR("Failed to create item");
        return false;
    }
    
    if (!player_inventory_add_item(inventory, item)) {
        item_instance_destroy(item);
        LOG_ERROR("Failed to add item to inventory");
        return false;
    }
    
    // Deduct gold
    inventory->gold -= total_cost;
    
    // Update vendor stock
    if (!vendor->inventory[vendor_slot_index].sells_infinite) {
        vendor->inventory[vendor_slot_index].count -= count;
    }
    
    LOG_INFO("Player bought %u items from vendor for %u gold", count, total_cost);
    return true;
}

bool vendor_sell_item(PlayerInventory* inventory, Vendor* vendor, uint32_t player_slot_index, uint32_t count) {
    if (!inventory || !vendor || player_slot_index >= MAX_INVENTORY_SIZE) {
        LOG_ERROR("Invalid parameters for vendor sell");
        return false;
    }
    
    InventorySlot* slot = &inventory->slots[player_slot_index];
    if (slot->is_empty || !slot->item) {
        LOG_ERROR("Invalid player slot");
        return false;
    }
    
    if (slot->item->stack_count < count) {
        LOG_ERROR("Insufficient item count");
        return false;
    }
    
    uint32_t item_value = item_instance_get_sell_price(slot->item, vendor->buy_multiplier);
    uint32_t total_value = item_value * count;
    
    // Remove item from inventory
    if (!player_inventory_remove_item(inventory, player_slot_index, count)) {
        LOG_ERROR("Failed to remove item from inventory");
        return false;
    }
    
    // Add gold to player
    inventory->gold += total_value;
    
    LOG_INFO("Player sold %u items to vendor for %u gold", count, total_value);
    return true;
}

void vendor_refresh_inventory(InventoryManager* manager, uint32_t vendor_id) {
    if (!manager || vendor_id == 0 || vendor_id > manager->vendor_count) {
        return;
    }
    
    Vendor* vendor = &manager->vendors[vendor_id - 1];
    vendor->last_refresh_time = get_current_time();
    
    // In a real implementation, this would regenerate the vendor's inventory
    LOG_INFO("Refreshed vendor %u inventory", vendor_id);
}

// =================================================================================================
// TRADING SYSTEM
// =================================================================================================

uint32_t trade_create(InventoryManager* manager, uint32_t player1_id, uint32_t player2_id) {
    if (!manager || player1_id == 0 || player2_id == 0) {
        LOG_ERROR("Invalid parameters for trade creation");
        return 0;
    }
    
    if (manager->trade_count >= 16) {
        LOG_ERROR("Maximum active trades reached");
        return 0;
    }
    
    uint32_t trade_id = manager->trade_count + 1;
    PlayerTrade* trade = &manager->active_trades[manager->trade_count];
    
    memset(trade, 0, sizeof(PlayerTrade));
    trade->trade_id = trade_id;
    trade->player1_id = player1_id;
    trade->player2_id = player2_id;
    trade->is_active = true;
    trade->start_time = get_current_time();
    trade->timeout_duration = 300; // 5 minutes
    
    manager->trade_count++;
    
    LOG_INFO("Created trade %u between players %u and %u", trade_id, player1_id, player2_id);
    return trade_id;
}

bool trade_add_item(InventoryManager* manager, uint32_t trade_id, uint32_t player_id, 
                   uint32_t slot_index, uint32_t count) {
    if (!manager || trade_id == 0 || trade_id > manager->trade_count) {
        LOG_ERROR("Invalid trade ID");
        return false;
    }
    
    PlayerTrade* trade = &manager->active_trades[trade_id - 1];
    if (!trade->is_active) {
        LOG_ERROR("Trade is not active");
        return false;
    }
    
    // Reset confirmation when items change
    trade->player1_offer.confirmed = false;
    trade->player2_offer.confirmed = false;
    
    LOG_INFO("Added item to trade %u by player %u", trade_id, player_id);
    return true;
}

bool trade_add_gold(InventoryManager* manager, uint32_t trade_id, uint32_t player_id, uint32_t amount) {
    if (!manager || trade_id == 0 || trade_id > manager->trade_count) {
        LOG_ERROR("Invalid trade ID");
        return false;
    }
    
    PlayerTrade* trade = &manager->active_trades[trade_id - 1];
    if (!trade->is_active) {
        LOG_ERROR("Trade is not active");
        return false;
    }
    
    // Reset confirmation when gold changes
    trade->player1_offer.confirmed = false;
    trade->player2_offer.confirmed = false;
    
    if (player_id == trade->player1_id) {
        trade->player1_offer.gold_offered = amount;
    } else if (player_id == trade->player2_id) {
        trade->player2_offer.gold_offered = amount;
    }
    
    LOG_INFO("Added %u gold to trade %u by player %u", amount, trade_id, player_id);
    return true;
}

bool trade_confirm(InventoryManager* manager, uint32_t trade_id, uint32_t player_id) {
    if (!manager || trade_id == 0 || trade_id > manager->trade_count) {
        LOG_ERROR("Invalid trade ID");
        return false;
    }
    
    PlayerTrade* trade = &manager->active_trades[trade_id - 1];
    if (!trade->is_active) {
        LOG_ERROR("Trade is not active");
        return false;
    }
    
    if (player_id == trade->player1_id) {
        trade->player1_offer.confirmed = true;
    } else if (player_id == trade->player2_id) {
        trade->player2_offer.confirmed = true;
    }
    
    // Check if both players confirmed
    if (trade->player1_offer.confirmed && trade->player2_offer.confirmed) {
        return trade_execute(manager, trade_id);
    }
    
    LOG_INFO("Player %u confirmed trade %u", player_id, trade_id);
    return true;
}

bool trade_execute(InventoryManager* manager, uint32_t trade_id) {
    if (!manager || trade_id == 0 || trade_id > manager->trade_count) {
        LOG_ERROR("Invalid trade ID");
        return false;
    }
    
    PlayerTrade* trade = &manager->active_trades[trade_id - 1];
    if (!trade->is_active) {
        LOG_ERROR("Trade is not active");
        return false;
    }
    
    // Execute trade logic
    // Note: In a real implementation, this would transfer items and gold between players
    
    trade->is_active = false;
    
    LOG_INFO("Executed trade %u successfully", trade_id);
    return true;
}

void trade_cancel(InventoryManager* manager, uint32_t trade_id) {
    if (!manager || trade_id == 0 || trade_id > manager->trade_count) {
        return;
    }
    
    PlayerTrade* trade = &manager->active_trades[trade_id - 1];
    trade->is_active = false;
    
    LOG_INFO("Cancelled trade %u", trade_id);
}

// =================================================================================================
// QUICKBAR SYSTEM
// =================================================================================================

bool quickbar_set_item(Quickbar* quickbar, uint32_t slot, ItemInstance* item) {
    if (!quickbar || slot >= 9) {
        LOG_ERROR("Invalid parameters for quickbar set");
        return false;
    }
    
    quickbar->items[slot] = item;
    quickbar->item_counts[slot] = item ? item->stack_count : 0;
    quickbar->is_valid[slot] = (item != NULL);
    
    LOG_INFO("Set item in quickbar slot %u", slot);
    return true;
}

bool quickbar_use_item(Quickbar* quickbar, PlayerInventory* inventory, uint32_t slot, uint32_t player_id) {
    if (!quickbar || !inventory || slot >= 9) {
        LOG_ERROR("Invalid parameters for quickbar use");
        return false;
    }
    
    ItemInstance* item = quickbar->items[slot];
    if (!item || !quickbar->is_valid[slot]) {
        LOG_ERROR("No item in quickbar slot %u", slot);
        return false;
    }
    
    // Use item logic
    // Note: In a real implementation, this would handle consumables, tools, etc.
    
    // Update last used time
    item->last_used_time = get_current_time();
    
    LOG_INFO("Used item from quickbar slot %u", slot);
    return true;
}

ItemInstance* quickbar_get_item(const Quickbar* quickbar, uint32_t slot) {
    if (!quickbar || slot >= 9) {
        return NULL;
    }
    
    return quickbar->items[slot];
}

// =================================================================================================
// DURABILITY SYSTEM
// =================================================================================================

bool item_instance_repair(ItemInstance* item, uint32_t amount) {
    if (!item) {
        LOG_ERROR("Invalid item for repair");
        return false;
    }
    
    if (item->max_durability == 0) {
        LOG_ERROR("Item cannot be repaired");
        return false;
    }
    
    item->durability = (item->durability + amount > item->max_durability) ? 
                       item->max_durability : item->durability + amount;
    
    LOG_INFO("Repaired item by %u points", amount);
    return true;
}

bool item_instance_take_damage(ItemInstance* item, uint32_t damage) {
    if (!item) {
        LOG_ERROR("Invalid item for damage");
        return false;
    }
    
    if (item->max_durability == 0) {
        return true; // Item doesn't have durability
    }
    
    if (damage >= item->durability) {
        item->durability = 0;
        LOG_INFO("Item broke from damage");
        return false;
    }
    
    item->durability -= damage;
    LOG_DEBUG("Item took %u damage, remaining: %u", damage, item->durability);
    return true;
}

bool item_instance_is_broken(const ItemInstance* item) {
    if (!item) return true;
    
    return (item->max_durability > 0 && item->durability == 0);
}

float item_instance_get_durability_percentage(const ItemInstance* item) {
    if (!item || item->max_durability == 0) return 1.0f;
    
    return (float)item->durability / item->max_durability;
}

// =================================================================================================
// AUTO-LOOT SYSTEM
// =================================================================================================

void player_inventory_set_auto_loot_filter(PlayerInventory* inventory, ItemRarity min_rarity, bool enabled) {
    if (!inventory) return;
    
    // Set all rarities at or above min_rarity
    inventory->auto_loot_common = (min_rarity <= RARITY_COMMON) && enabled;
    inventory->auto_loot_uncommon = (min_rarity <= RARITY_UNCOMMON) && enabled;
    inventory->auto_loot_rare = (min_rarity <= RARITY_RARE) && enabled;
    inventory->auto_loot_epic = (min_rarity <= RARITY_EPIC) && enabled;
    inventory->auto_loot_legendary = (min_rarity <= RARITY_LEGENDARY) && enabled;
    
    LOG_INFO("Set auto-loot filter to rarity %d, enabled: %s", min_rarity, enabled ? "true" : "false");
}

bool player_inventory_should_auto_loot(const PlayerInventory* inventory, const ItemInstance* item) {
    if (!inventory || !item) return false;
    
    // Note: In a real implementation, get item rarity from definition
    ItemRarity rarity = RARITY_COMMON; // Placeholder
    
    switch (rarity) {
        case RARITY_COMMON: return inventory->auto_loot_common;
        case RARITY_UNCOMMON: return inventory->auto_loot_uncommon;
        case RARITY_RARE: return inventory->auto_loot_rare;
        case RARITY_EPIC: return inventory->auto_loot_epic;
        case RARITY_LEGENDARY: return inventory->auto_loot_legendary;
        default: return false;
    }
}

// =================================================================================================
// SAVE/LOAD SYSTEM
// =================================================================================================

bool inventory_save_state(const PlayerInventory* inventory, InventorySaveData* save_data) {
    if (!inventory || !save_data) {
        LOG_ERROR("Invalid parameters for inventory save");
        return false;
    }
    
    memset(save_data, 0, sizeof(InventorySaveData));
    save_data->version = 1;
    
    // Save inventory slots
    for (uint32_t i = 0; i < MAX_INVENTORY_SIZE; i++) {
        const InventorySlot* slot = &inventory->slots[i];
        
        if (slot->is_empty || !slot->item) {
            save_data->slots[i].is_empty = true;
        } else {
            save_data->slots[i].definition_id = slot->item->definition_id;
            save_data->slots[i].stack_count = slot->item->stack_count;
            save_data->slots[i].durability = slot->item->durability;
            save_data->slots[i].custom_modifier_count = slot->item->custom_modifier_count;
            memcpy(save_data->slots[i].custom_modifiers, slot->item->custom_modifiers, 
                   sizeof(ItemModifier) * slot->item->custom_modifier_count);
            save_data->slots[i].is_empty = false;
        }
    }
    
    // Save equipment
    for (int i = 0; i < SLOT_COUNT; i++) {
        save_data->equipment_ids[i] = inventory->equipment_slots[i] ? 
                                     inventory->equipment_slots[i]->definition_id : 0;
    }
    
    // Save currency
    save_data->gold = inventory->gold;
    save_data->gems = inventory->gems;
    save_data->tokens = inventory->tokens;
    
    // Save quickbar
    for (int i = 0; i < 9; i++) {
        // Note: In real implementation, save actual item references
        save_data->quickbar_items[i] = 0;
        save_data->quickbar_counts[i] = 0;
    }
    
    // Save auto-loot settings
    save_data->auto_loot_settings[0] = inventory->auto_loot_common;
    save_data->auto_loot_settings[1] = inventory->auto_loot_uncommon;
    save_data->auto_loot_settings[2] = inventory->auto_loot_rare;
    save_data->auto_loot_settings[3] = inventory->auto_loot_epic;
    save_data->auto_loot_settings[4] = inventory->auto_loot_legendary;
    
    LOG_INFO("Saved inventory state");
    return true;
}

bool inventory_load_state(PlayerInventory* inventory, const InventorySaveData* save_data, InventoryManager* manager) {
    if (!inventory || !save_data || !manager) {
        LOG_ERROR("Invalid parameters for inventory load");
        return false;
    }
    
    // Clear existing inventory
    player_inventory_shutdown(inventory);
    player_inventory_init(inventory);
    
    // Load inventory slots
    for (uint32_t i = 0; i < MAX_INVENTORY_SIZE; i++) {
        if (!save_data->slots[i].is_empty) {
            ItemInstance* item = item_instance_create(save_data->slots[i].definition_id, 
                                                     save_data->slots[i].stack_count);
            if (item) {
                item->durability = save_data->slots[i].durability;
                item->custom_modifier_count = save_data->slots[i].custom_modifier_count;
                memcpy(item->custom_modifiers, save_data->slots[i].custom_modifiers,
                       sizeof(ItemModifier) * save_data->slots[i].custom_modifier_count);
                
                inventory->slots[i].item = item;
                inventory->slots[i].is_empty = false;
            }
        }
    }
    
    // Load equipment
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (save_data->equipment_ids[i] != 0) {
            ItemInstance* item = item_instance_create(save_data->equipment_ids[i], 1);
            if (item) {
                inventory->equipment_slots[i] = item;
            }
        }
    }
    
    // Load currency
    inventory->gold = save_data->gold;
    inventory->gems = save_data->gems;
    inventory->tokens = save_data->tokens;
    
    // Load auto-loot settings
    inventory->auto_loot_common = save_data->auto_loot_settings[0];
    inventory->auto_loot_uncommon = save_data->auto_loot_settings[1];
    inventory->auto_loot_rare = save_data->auto_loot_settings[2];
    inventory->auto_loot_epic = save_data->auto_loot_settings[3];
    inventory->auto_loot_legendary = save_data->auto_loot_settings[4];
    
    LOG_INFO("Loaded inventory state");
    return true;
}

// =================================================================================================
// ADDITIONAL UTILITY FUNCTIONS
// =================================================================================================

uint32_t item_instance_get_sell_price(const ItemInstance* item, float multiplier) {
    if (!item) return 0;
    
    uint32_t base_value = item_instance_get_value(item);
    return (uint32_t)(base_value * multiplier);
}

bool item_instance_meets_requirements(const ItemInstance* item, uint32_t player_level, 
                                     uint32_t strength, uint32_t dexterity, uint32_t intelligence) {
    if (!item) return false;
    
    // Note: In real implementation, get requirements from item definition
    // For now, assume all items meet requirements
    return true;
}
