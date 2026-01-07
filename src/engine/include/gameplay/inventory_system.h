#ifndef INVENTORY_SYSTEM_H
#define INVENTORY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "include/math/vec3.h"
#include "core/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inventory System Limits
#define MAX_ITEMS 4096
#define MAX_INVENTORY_SIZE 64
#define MAX_EQUIPMENT_SLOTS 16
#define MAX_ITEM_NAME_LENGTH 64
#define MAX_ITEM_DESC_LENGTH 256
#define MAX_STACK_SIZE 999
#define MAX_MODIFIERS_PER_ITEM 8
#define MAX_CRAFTING_RECIPES 512

// Item Types
typedef enum {
    ITEM_TYPE_WEAPON = 0,
    ITEM_TYPE_ARMOR,
    ITEM_TYPE_CONSUMABLE,
    ITEM_TYPE_MATERIAL,
    ITEM_TYPE_TOOL,
    ITEM_TYPE_QUEST,
    ITEM_TYPE_CURRENCY,
    ITEM_TYPE_MISC
} ItemType;

// Item Rarity
typedef enum {
    RARITY_COMMON = 0,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY
} ItemRarity;

// Equipment Slots
typedef enum {
    SLOT_HEAD = 0,
    SLOT_CHEST,
    SLOT_LEGS,
    SLOT_BOOTS,
    SLOT_GLOVES,
    SLOT_WEAPON_MAIN,
    SLOT_WEAPON_OFF,
    SLOT_NECK,
    SLOT_RING,
    SLOT_TRINKET,
    SLOT_BACKPACK,
    SLOT_COUNT
} EquipmentSlot;

// Item Modifier Types
typedef enum {
    MODIFIER_DAMAGE = 0,
    MODIFIER_ARMOR,
    MODIFIER_HEALTH,
    MODIFIER_MANA,
    MODIFIER_SPEED,
    MODIFIER_CRIT_CHANCE,
    MODIFIER_CRIT_DAMAGE,
    MODIFIER_FIRE_RESIST,
    MODIFIER_ICE_RESIST,
    MODIFIER_LIGHTNING_RESIST,
    MODIFIER_POISON_RESIST,
    MODIFIER_STRENGTH,
    MODIFIER_DEXTERITY,
    MODIFIER_INTELLIGENCE,
    MODIFIER_WISDOM,
    MODIFIER_LUCK
} ModifierType;

// Item Stat Modifier
typedef struct {
    ModifierType type;
    float value;
    bool is_percentage;
} ItemModifier;

// Item Definition (template for all items of this type)
typedef struct {
    uint32_t id;
    char name[MAX_ITEM_NAME_LENGTH];
    char description[MAX_ITEM_DESC_LENGTH];
    ItemType type;
    ItemRarity rarity;
    
    // Base stats
    uint32_t base_damage;
    uint32_t base_armor;
    float base_speed_modifier;
    
    // Visual
    char icon_name[64];
    char model_name[64];
    uint32_t color_tint;
    
    // Properties
    uint32_t max_stack_size;
    uint32_t max_durability;
    bool is_stackable;
    bool is_equippable;
    bool is_consumable;
    bool is_craftable;
    
    // Equipment
    EquipmentSlot equip_slot;
    
    // Requirements
    uint32_t level_requirement;
    uint32_t strength_requirement;
    uint32_t dexterity_requirement;
    uint32_t intelligence_requirement;
    
    // Value
    uint32_t base_value;
    uint32_t sell_value;
    
    // Modifiers (base modifiers for this item type)
    ItemModifier base_modifiers[MAX_MODIFIERS_PER_ITEM];
    uint8_t modifier_count;
} ItemDefinition;

// Item Instance (actual item in inventory)
typedef struct {
    uint32_t instance_id;
    uint32_t definition_id;
    uint32_t stack_count;
    uint32_t durability;
    uint32_t max_durability;
    
    // Custom modifiers (from random generation, enchantments, etc.)
    ItemModifier custom_modifiers[MAX_MODIFIERS_PER_ITEM];
    uint8_t custom_modifier_count;
    
    // Metadata
    uint32_t creation_time;
    uint32_t last_used_time;
    bool is_bound;
    bool is_soulbound;
    char custom_name[MAX_ITEM_NAME_LENGTH];
} ItemInstance;

// Inventory Slot
typedef struct {
    ItemInstance* item;
    uint32_t slot_index;
    bool is_empty;
    bool is_locked;
} InventorySlot;

// Player Inventory
typedef struct {
    InventorySlot slots[MAX_INVENTORY_SIZE];
    uint32_t slot_count;
    
    // Equipment
    ItemInstance* equipment_slots[SLOT_COUNT];
    
    // Currency
    uint32_t gold;
    uint32_t gems;
    uint32_t tokens;
    
    // Statistics
    uint32_t total_items;
    uint32_t unique_items;
    uint32_t total_value;
    
    // Auto-loot filters
    bool auto_loot_common;
    bool auto_loot_uncommon;
    bool auto_loot_rare;
    bool auto_loot_epic;
    bool auto_loot_legendary;
} PlayerInventory;

// Crafting Recipe
typedef struct {
    uint32_t id;
    char name[MAX_ITEM_NAME_LENGTH];
    uint32_t result_item_id;
    uint32_t result_count;
    
    struct {
        uint32_t item_id;
        uint32_t count;
    } ingredients[8];
    uint8_t ingredient_count;
    
    // Requirements
    uint32_t crafting_level_requirement;
    uint32_t tool_requirement;
    float crafting_time;
    
    // Properties
    bool is_discoverable;
    bool is_one_time;
    uint32_t craft_count_limit;
    uint32_t times_crafted;
} CraftingRecipe;

// Crafting Station
typedef struct {
    uint32_t station_id;
    char name[MAX_ITEM_NAME_LENGTH];
    uint32_t station_type;
    
    // Available recipes
    uint32_t available_recipes[MAX_CRAFTING_RECIPES];
    uint32_t recipe_count;
    
    // Properties
    Vec3 position;
    float interaction_radius;
    bool is_active;
} CraftingStation;

// Loot Table
typedef struct {
    uint32_t id;
    char name[MAX_ITEM_NAME_LENGTH];
    
    struct {
        uint32_t item_id;
        uint32_t min_count;
        uint32_t max_count;
        float drop_chance;
        ItemRarity min_rarity;
        ItemRarity max_rarity;
    } possible_drops[32];
    uint8_t drop_count;
    
    // Properties
    bool is_unique_drop;
    uint32_t max_drops_per_kill;
    float guaranteed_drop_chance;
} LootTable;

// Vendor System
typedef struct {
    uint32_t vendor_id;
    char name[MAX_ITEM_NAME_LENGTH];
    
    struct {
        uint32_t item_id;
        uint32_t count;
        uint32_t buy_price;
        bool sells_infinite;
    } inventory[64];
    uint32_t item_count;
    
    // Pricing
    float buy_multiplier;    // Price multiplier when buying from player
    float sell_multiplier;   // Price multiplier when selling to player
    
    // Properties
    Vec3 position;
    float interaction_radius;
    bool is_active;
    uint32_t refresh_interval; // Time in seconds to refresh inventory
    uint32_t last_refresh_time;
} Vendor;

// Trade System (Player-to-Player)
typedef struct {
    uint32_t trade_id;
    uint32_t player1_id;
    uint32_t player2_id;
    
    struct {
        ItemInstance* items[16];
        uint32_t item_count;
        uint32_t gold_offered;
        bool confirmed;
    } player1_offer, player2_offer;
    
    // Trade state
    bool is_active;
    uint32_t start_time;
    uint32_t timeout_duration;
} PlayerTrade;

// Quickbar (1-9 hotkeys)
typedef struct {
    ItemInstance* items[9];
    uint32_t item_counts[9];
    bool is_valid[9];
} Quickbar;

// Item Database
typedef struct {
    ItemDefinition items[MAX_ITEMS];
    uint32_t item_count;
    
    // Index maps for fast lookup
    uint32_t name_to_id_map[MAX_ITEMS];
    uint32_t type_to_id_map[MAX_ITEMS];
} ItemDatabase;

// Inventory Manager
typedef struct {
    ItemDatabase database;
    CraftingRecipe recipes[MAX_CRAFTING_RECIPES];
    uint32_t recipe_count;
    
    // Active crafting stations
    CraftingStation stations[64];
    uint32_t station_count;
    
    // Loot tables
    LootTable loot_tables[128];
    uint32_t loot_table_count;
    
    // Vendors
    Vendor vendors[32];
    uint32_t vendor_count;
    
    // Active trades
    PlayerTrade active_trades[16];
    uint32_t trade_count;
} InventoryManager;

// Save Data Structures
typedef struct {
    uint32_t version;
    
    // Inventory slots
    struct {
        uint32_t definition_id;
        uint32_t stack_count;
        uint32_t durability;
        uint32_t custom_modifier_count;
        ItemModifier custom_modifiers[MAX_MODIFIERS_PER_ITEM];
        bool is_empty;
    } slots[MAX_INVENTORY_SIZE];
    
    // Equipment
    uint32_t equipment_ids[SLOT_COUNT];
    
    // Currency
    uint32_t gold, gems, tokens;
    
    // Quickbar
    uint32_t quickbar_items[9];
    uint32_t quickbar_counts[9];
    
    // Auto-loot settings
    bool auto_loot_settings[5];
} InventorySaveData;

// Core Functions
bool inventory_manager_init(InventoryManager* manager);
void inventory_manager_shutdown(InventoryManager* manager);
InventoryManager* inventory_manager_create(void);
void inventory_manager_destroy(InventoryManager* manager);

// Item Database Management
uint32_t item_database_add_item(InventoryManager* manager, const ItemDefinition* item);
ItemDefinition* item_database_get_item(InventoryManager* manager, uint32_t item_id);
ItemDefinition* item_database_find_item_by_name(InventoryManager* manager, const char* name);
bool item_database_load_from_file(InventoryManager* manager, const char* filename);

// Player Inventory Management
bool player_inventory_init(PlayerInventory* inventory);
void player_inventory_shutdown(PlayerInventory* inventory);
bool player_inventory_add_item(PlayerInventory* inventory, ItemInstance* item);
bool player_inventory_remove_item(PlayerInventory* inventory, uint32_t slot_index, uint32_t count);
bool player_inventory_move_item(PlayerInventory* inventory, uint32_t from_slot, uint32_t to_slot);
bool player_inventory_split_stack(PlayerInventory* inventory, uint32_t slot_index, uint32_t count);
bool player_inventory_merge_stacks(PlayerInventory* inventory, uint32_t slot1, uint32_t slot2);

// Item Instance Management
ItemInstance* item_instance_create(uint32_t definition_id, uint32_t count);
void item_instance_destroy(ItemInstance* item);
ItemInstance* item_instance_clone(const ItemInstance* source);
bool item_instance_apply_modifier(ItemInstance* item, const ItemModifier* modifier);
bool item_instance_remove_modifier(ItemInstance* item, ModifierType type);
float item_instance_get_stat_modifier(const ItemInstance* item, ModifierType type);

// Equipment System
bool player_inventory_equip_item(PlayerInventory* inventory, uint32_t slot_index, EquipmentSlot equip_slot);
bool player_inventory_unequip_item(PlayerInventory* inventory, EquipmentSlot equip_slot);
bool player_inventory_can_equip(const PlayerInventory* inventory, const ItemInstance* item, uint32_t player_level);
void player_inventory_apply_equipment_bonuses(const PlayerInventory* inventory, uint32_t* stats);

// Crafting System
uint32_t crafting_add_recipe(InventoryManager* manager, const CraftingRecipe* recipe);
CraftingRecipe* crafting_get_recipe(InventoryManager* manager, uint32_t recipe_id);
bool crafting_can_craft(const PlayerInventory* inventory, const CraftingRecipe* recipe);
bool crafting_has_ingredients(const PlayerInventory* inventory, const CraftingRecipe* recipe);
bool crafting_craft_item(PlayerInventory* inventory, const CraftingRecipe* recipe);
bool crafting_remove_ingredients(PlayerInventory* inventory, const CraftingRecipe* recipe);
void crafting_add_result(PlayerInventory* inventory, const CraftingRecipe* recipe);

// Loot System
uint32_t loot_table_create(InventoryManager* manager, const char* name);
bool loot_table_add_drop(InventoryManager* manager, uint32_t table_id, uint32_t item_id, 
                         uint32_t min_count, uint32_t max_count, float drop_chance);
ItemInstance** loot_table_generate_loot(InventoryManager* manager, uint32_t table_id, uint32_t* count, uint32_t player_level);

// Vendor System
uint32_t vendor_create(InventoryManager* manager, const char* name, Vec3 position);
bool vendor_add_item(InventoryManager* manager, uint32_t vendor_id, uint32_t item_id, 
                     uint32_t count, uint32_t price);
bool vendor_buy_item(PlayerInventory* inventory, Vendor* vendor, uint32_t vendor_slot_index, uint32_t count);
bool vendor_sell_item(PlayerInventory* inventory, Vendor* vendor, uint32_t player_slot_index, uint32_t count);
void vendor_refresh_inventory(InventoryManager* manager, uint32_t vendor_id);

// Trading System
uint32_t trade_create(InventoryManager* manager, uint32_t player1_id, uint32_t player2_id);
bool trade_add_item(InventoryManager* manager, uint32_t trade_id, uint32_t player_id, 
                   uint32_t slot_index, uint32_t count);
bool trade_add_gold(InventoryManager* manager, uint32_t trade_id, uint32_t player_id, uint32_t amount);
bool trade_confirm(InventoryManager* manager, uint32_t trade_id, uint32_t player_id);
bool trade_execute(InventoryManager* manager, uint32_t trade_id);
void trade_cancel(InventoryManager* manager, uint32_t trade_id);

// Quickbar System
bool quickbar_set_item(Quickbar* quickbar, uint32_t slot, ItemInstance* item);
bool quickbar_use_item(Quickbar* quickbar, PlayerInventory* inventory, uint32_t slot, uint32_t player_id);
ItemInstance* quickbar_get_item(const Quickbar* quickbar, uint32_t slot);

// Inventory Sorting and Filtering
void player_inventory_sort(PlayerInventory* inventory, enum {
    SORT_BY_NAME,
    SORT_BY_TYPE,
    SORT_BY_RARITY,
    SORT_BY_VALUE
} sort_type);
void player_inventory_filter(PlayerInventory* inventory, ItemType type_filter, ItemRarity rarity_filter);

// Durability System
bool item_instance_repair(ItemInstance* item, uint32_t amount);
bool item_instance_take_damage(ItemInstance* item, uint32_t damage);
bool item_instance_is_broken(const ItemInstance* item);
float item_instance_get_durability_percentage(const ItemInstance* item);

// Auto-loot System
void player_inventory_set_auto_loot_filter(PlayerInventory* inventory, ItemRarity min_rarity, bool enabled);
bool player_inventory_should_auto_loot(const PlayerInventory* inventory, const ItemInstance* item);

// Save/Load System
bool inventory_save_state(const PlayerInventory* inventory, InventorySaveData* save_data);
bool inventory_load_state(PlayerInventory* inventory, const InventorySaveData* save_data, InventoryManager* manager);

// Utility Functions
const char* item_type_to_string(ItemType type);
const char* rarity_to_string(ItemRarity rarity);
const char* slot_to_string(EquipmentSlot slot);
const char* modifier_to_string(ModifierType type);
uint32_t item_instance_get_value(const ItemInstance* item);
uint32_t item_instance_get_sell_price(const ItemInstance* item, float multiplier);
bool item_instance_meets_requirements(const ItemInstance* item, uint32_t player_level, 
                                     uint32_t strength, uint32_t dexterity, uint32_t intelligence);

// Procedural Item Generation
ItemInstance* item_generate_random(uint32_t base_item_id, uint32_t item_level);
ItemInstance* item_generate_with_modifiers(uint32_t base_item_id, ItemModifier* modifiers, uint8_t count);
void item_apply_random_modifiers(ItemInstance* item, uint32_t item_level, uint8_t modifier_count);

// Stack Management
bool item_instance_can_stack(const ItemInstance* item1, const ItemInstance* item2);
uint32_t item_instance_get_stack_space(const ItemInstance* item);
bool item_instance_add_to_stack(ItemInstance* target, ItemInstance* source);

// Inventory Validation
bool player_inventory_validate(const PlayerInventory* inventory);
bool item_instance_validate(const ItemInstance* item);
bool item_definition_validate(const ItemDefinition* item);

// Statistics and Analytics
typedef struct {
    uint32_t items_picked_up;
    uint32_t items_sold;
    uint32_t items_crafted;
    uint32_t gold_earned;
    uint32_t gold_spent;
    uint32_t items_traded;
    uint32_t unique_items_discovered;
} InventoryStats;

void inventory_update_stats(PlayerInventory* inventory, const InventoryStats* stats);
InventoryStats inventory_get_stats(const PlayerInventory* inventory);

#ifdef __cplusplus
}
#endif

#endif // INVENTORY_SYSTEM_H
