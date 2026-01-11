// include/inventory/item_registry.h
//
// Purpose: Defines the comprehensive item registry system for managing all game
// items. This provides a centralized database of item definitions including
// tools, weapons, food, materials, armor, and blocks. The registry allows for
// runtime registration, querying, and modification of item properties.
//
// Public APIs:
// - `ItemID`: Unique identifier for each item type
// - `ItemRegistry`: Central structure managing all item definitions
// - `item_registry_init`: Initialize the registry with default items
// - `item_registry_register`: Register a new item type
// - `item_registry_get`: Retrieve item definition by ID
// - `item_registry_get_by_name`: Retrieve item by name lookup
//
// Ownership: The ItemRegistry owns all ItemDefinition data
//
// Invariants:
// - ItemRegistry must be initialized before use
// - Item IDs must be unique
// - Item names are case-insensitive for lookup
//
#ifndef ITEM_REGISTRY_H
#define ITEM_REGISTRY_H

#include "../game_common.h"
#include "inventory.h"

typedef u32 ItemID;

#define ITEM_REGISTRY_VERSION 1

// Item ID definitions (organized by category)

// === BLOCKS (0-255) ===
#define ITEM_AIR 0
#define ITEM_STONE 1
#define ITEM_GRASS_BLOCK 2
#define ITEM_DIRT 3
#define ITEM_COBBLESTONE 4
#define ITEM_PLANKS 5
#define ITEM_LEAVES 6
#define ITEM_SAND 7
#define ITEM_GRAVEL 8
#define ITEM_WATER_BUCKET 9
#define ITEM_LAVA_BUCKET 10
#define ITEM_GLASS 11
#define ITEM_BEDROCK 12
#define ITEM_FURNACE 13
#define ITEM_SOLAR_PANEL 14
#define ITEM_RUBBER_WOOD 15
#define ITEM_RUBBER_LEAVES 16
#define ITEM_IRON_ORE 20
#define ITEM_GOLD_ORE 21
#define ITEM_DIAMOND_ORE 22
#define ITEM_COAL_ORE 23
#define ITEM_COPPER_ORE 24
#define ITEM_TIN_ORE 25
#define ITEM_URANIUM_ORE 26
#define ITEM_OAK_LOG 30
#define ITEM_BIRCH_LOG 31
#define ITEM_SPRUCE_LOG 32
#define ITEM_JUNGLE_LOG 33
#define ITEM_CRAFTING_TABLE 40
#define ITEM_CHEST 41
#define ITEM_TORCH 42
#define ITEM_LADDER 43
#define ITEM_WHEAT 50
#define ITEM_CARROTS 51
#define ITEM_POTATOES 52
#define ITEM_BEETROOT 53

// === TOOLS (256-383) ===
#define ITEM_WOODEN_PICKAXE 256
#define ITEM_WOODEN_AXE 257
#define ITEM_WOODEN_SHOVEL 258
#define ITEM_WOODEN_HOE 259
#define ITEM_STONE_PICKAXE 260
#define ITEM_STONE_AXE 261
#define ITEM_STONE_SHOVEL 262
#define ITEM_STONE_HOE 263
#define ITEM_IRON_PICKAXE 264
#define ITEM_IRON_AXE 265
#define ITEM_IRON_SHOVEL 266
#define ITEM_IRON_HOE 267
#define ITEM_DIAMOND_PICKAXE 268
#define ITEM_DIAMOND_AXE 269
#define ITEM_DIAMOND_SHOVEL 270
#define ITEM_DIAMOND_HOE 271
#define ITEM_GOLDEN_PICKAXE 272
#define ITEM_GOLDEN_AXE 273
#define ITEM_GOLDEN_SHOVEL 274
#define ITEM_GOLDEN_HOE 275
#define ITEM_SHEARS 280
#define ITEM_FLINT_AND_STEEL 281
#define ITEM_FISHING_ROD 282
#define ITEM_COMPASS 283
#define ITEM_CLOCK 284

// === WEAPONS (384-511) ===
#define ITEM_WOODEN_SWORD 384
#define ITEM_STONE_SWORD 385
#define ITEM_IRON_SWORD 386
#define ITEM_DIAMOND_SWORD 387
#define ITEM_GOLDEN_SWORD 388
#define ITEM_BOW 390
#define ITEM_ARROW 391
#define ITEM_CROSSBOW 392
#define ITEM_TRIDENT 393
#define ITEM_SHIELD 394

// === ARMOR (512-639) ===
// Leather
#define ITEM_LEATHER_HELMET 512
#define ITEM_LEATHER_CHESTPLATE 513
#define ITEM_LEATHER_LEGGINGS 514
#define ITEM_LEATHER_BOOTS 515
// Chainmail
#define ITEM_CHAINMAIL_HELMET 516
#define ITEM_CHAINMAIL_CHESTPLATE 517
#define ITEM_CHAINMAIL_LEGGINGS 518
#define ITEM_CHAINMAIL_BOOTS 519
// Iron
#define ITEM_IRON_HELMET 520
#define ITEM_IRON_CHESTPLATE 521
#define ITEM_IRON_LEGGINGS 522
#define ITEM_IRON_BOOTS 523
// Diamond
#define ITEM_DIAMOND_HELMET 524
#define ITEM_DIAMOND_CHESTPLATE 525
#define ITEM_DIAMOND_LEGGINGS 526
#define ITEM_DIAMOND_BOOTS 527
// Golden
#define ITEM_GOLDEN_HELMET 528
#define ITEM_GOLDEN_CHESTPLATE 529
#define ITEM_GOLDEN_LEGGINGS 530
#define ITEM_GOLDEN_BOOTS 531

// === FOOD (640-767) ===
#define ITEM_APPLE 640
#define ITEM_GOLDEN_APPLE 641
#define ITEM_BREAD 642
#define ITEM_COOKED_PORKCHOP 643
#define ITEM_RAW_PORKCHOP 644
#define ITEM_COOKED_BEEF 645
#define ITEM_RAW_BEEF 646
#define ITEM_COOKED_CHICKEN 647
#define ITEM_RAW_CHICKEN 648
#define ITEM_COOKED_MUTTON 649
#define ITEM_RAW_MUTTON 650
#define ITEM_COOKED_FISH 651
#define ITEM_RAW_FISH 652
#define ITEM_COOKIE 653
#define ITEM_MELON_SLICE 654
#define ITEM_CARROT 655
#define ITEM_POTATO 656
#define ITEM_BAKED_POTATO 657
#define ITEM_POISONOUS_POTATO 658
#define ITEM_MUSHROOM_STEW 659
#define ITEM_BEETROOT_SOUP 660
#define ITEM_RABBIT_STEW 661
#define ITEM_PUMPKIN_PIE 662

// === MATERIALS (768-1023) ===
#define ITEM_STICK 768
#define ITEM_COAL 769
#define ITEM_CHARCOAL 770
#define ITEM_IRON_INGOT 771
#define ITEM_GOLD_INGOT 772
#define ITEM_DIAMOND 773
#define ITEM_EMERALD 774
#define ITEM_COPPER_INGOT 775
#define ITEM_TIN_INGOT 776
#define ITEM_BRONZE_INGOT 777
#define ITEM_STEEL_INGOT 778
#define ITEM_STRING 780
#define ITEM_FEATHER 781
#define ITEM_LEATHER 782
#define ITEM_GUNPOWDER 783
#define ITEM_WHEAT_SEEDS 784
#define ITEM_FLINT 785
#define ITEM_BONE 786
#define ITEM_SLIMEBALL 787
#define ITEM_ENDER_PEARL 788
#define ITEM_BLAZE_ROD 789
#define ITEM_GLOWSTONE_DUST 790
#define ITEM_REDSTONE 791
#define ITEM_LAPIS_LAZULI 792
#define ITEM_QUARTZ 793
#define ITEM_PRISMARINE_SHARD 794
#define ITEM_NAUTILUS_SHELL 795
#define ITEM_RUBBER 800
#define ITEM_PLASTIC 801
#define ITEM_CIRCUIT 802
#define ITEM_BATTERY 803
#define ITEM_SOLAR_CELL 804

// === POTIONS & BREWING (1024-1151) ===
#define ITEM_GLASS_BOTTLE 1024
#define ITEM_WATER_BOTTLE 1025
#define ITEM_AWKWARD_POTION 1026
#define ITEM_POTION_HEALING 1027
#define ITEM_POTION_FIRE_RESISTANCE 1028
#define ITEM_POTION_REGENERATION 1029
#define ITEM_POTION_STRENGTH 1030
#define ITEM_POTION_SWIFTNESS 1031
#define ITEM_POTION_NIGHT_VISION 1032
#define ITEM_POTION_INVISIBILITY 1033
#define ITEM_POTION_POISON 1034
#define ITEM_POTION_WEAKNESS 1035
#define ITEM_POTION_SLOWNESS 1036
#define ITEM_POTION_HARMING 1037
#define ITEM_NETHER_WART 1040
#define ITEM_BLAZE_POWDER 1041
#define ITEM_MAGMA_CREAM 1042
#define ITEM_SPIDER_EYE 1043
#define ITEM_FERMENTED_SPIDER_EYE 1044
#define ITEM_GHAST_TEAR 1045
#define ITEM_GOLDEN_CARROT 1046

// === MISC (1152-1279) ===
#define ITEM_BUCKET 1152
#define ITEM_BOOK 1153
#define ITEM_ENCHANTED_BOOK 1154
#define ITEM_MAP 1155
#define ITEM_SADDLE 1156
#define ITEM_MINECART 1157
#define ITEM_BOAT 1158
#define ITEM_ELYTRA 1159
#define ITEM_TOTEM_OF_UNDYING 1160
#define ITEM_EXPERIENCE_BOTTLE 1161

// Food properties
typedef struct {
  f32 hunger_restored;     // Hunger points restored (0-20)
  f32 saturation_modifier; // Saturation multiplier (0.0-1.0)
  f32 eat_duration;        // Seconds to consume
  bool has_effects;        // Can apply status effects
  u32 effect_id;           // Status effect ID if applicable
  f32 effect_chance;       // Chance of effect (0.0-1.0)
  f32 quality;             // Food quality, affects hunger/saturation. 1.0 is normal.
} FoodProperties;

// Tool properties
typedef struct {
  f32 mining_speed;  // Speed multiplier for mining
  u32 harvest_level; // 0=hand, 1=wood, 2=stone, 3=iron, 4=diamond
  f32 attack_damage; // Additional melee damage
  f32 attack_speed;  // Attacks per second
} ToolProperties;

// Weapon properties
typedef struct {
  f32 attack_damage;    // Base damage
  f32 attack_speed;     // Attacks per second
  f32 knockback;        // Knockback strength
  f32 reach;            // Attack reach distance
  bool is_ranged;       // Projectile weapon
  f32 projectile_speed; // For ranged weapons
} WeaponProperties;

// Armor properties
typedef struct {
  f32 defense;              // Defense points (0-20)
  f32 toughness;            // Armor toughness
  f32 knockback_resistance; // 0.0-1.0
  u32 armor_slot;           // 0=helmet, 1=chest, 2=legs, 3=boots
} ArmorProperties;

// Extended item definition
typedef struct {
  ItemDefinition base;

  // Type-specific properties
  union {
    FoodProperties food;
    ToolProperties tool;
    WeaponProperties weapon;
    ArmorProperties armor;
  } properties;

  // Additional metadata
  bool is_stackable;
  bool is_repairable;
  u32 repair_material_id;
  f32 fuel_value; // Burn time in seconds (for furnace fuel)
  u32 rarity;     // 0=common, 1=uncommon, 2=rare, 3=epic, 4=legendary

  // Material system integration
  u32 material_id; // MaterialID from material system (0 = no material)
} ExtendedItemDefinition;

typedef struct ItemRegistryStats {
  u32 total_registered;
  u32 duplicate_id_attempts;
  u32 duplicate_name_attempts;
  u32 invalid_items;
  u32 validation_failures;
  u32 hot_reload_count;
  u32 migrations_applied;
  u32 dependency_violations;
} ItemRegistryStats;

typedef struct ItemDependency {
  u32 item_id;
  u32 requires_item_id;
} ItemDependency;

// Item registry
typedef struct {
  ExtendedItemDefinition *items;
  u32 count;
  u32 capacity;
  void *name_lookup; // HashMap for name-based lookup
  u32 version;
  ItemRegistryStats stats;
  ItemDependency *dependencies;
  u32 dependency_count;
  u32 dependency_capacity;
} ItemRegistry;

// Registry functions
void item_registry_init(ItemRegistry *registry);
void item_registry_free(ItemRegistry *registry);

bool item_registry_register(ItemRegistry *registry, u32 item_id,
                            const ExtendedItemDefinition *def);
const ExtendedItemDefinition *item_registry_get(const ItemRegistry *registry,
                                                u32 item_id);
const ExtendedItemDefinition *
item_registry_get_by_name(const ItemRegistry *registry, const char *name);

// Initialize default items (called automatically by item_registry_init)
void item_registry_init_defaults(ItemRegistry *registry);

// Validation, stats, and dependencies
bool item_registry_validate(const ItemRegistry *registry);
bool item_registry_validate_dependencies(const ItemRegistry *registry);
bool item_registry_add_dependency(ItemRegistry *registry, u32 item_id,
                                  u32 requires_item_id);
const ItemRegistryStats *item_registry_get_stats(const ItemRegistry *registry);

// Search and filtering
u32 item_registry_find_by_type(const ItemRegistry *registry, ItemType type,
                               u32 *out_ids, u32 max_out);
u32 item_registry_find_by_name_contains(const ItemRegistry *registry,
                                        const char *needle, u32 *out_ids,
                                        u32 max_out);

// Serialization and migration
bool item_registry_save(const ItemRegistry *registry, const char *path);
bool item_registry_load(ItemRegistry *registry, const char *path);
bool item_registry_migrate(ItemRegistry *registry, u32 from_version,
                           u32 to_version);
bool item_registry_hot_reload(ItemRegistry *registry);

// Documentation generation
bool item_registry_write_docs(const ItemRegistry *registry, const char *path);

// Helper functions
bool item_is_food(const ExtendedItemDefinition *item);
bool item_is_tool(const ExtendedItemDefinition *item);
bool item_is_weapon(const ExtendedItemDefinition *item);
bool item_is_armor(const ExtendedItemDefinition *item);
bool item_is_fuel(const ExtendedItemDefinition *item);

#endif // ITEM_REGISTRY_H
