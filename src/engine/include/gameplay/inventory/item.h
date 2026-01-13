// include/gameplay/inventory/item.h
//
// Purpose: Core item system definitions. Items are the fundamental units of the
// inventory system, representing weapons, armor, consumables, materials, and
// quest items.
//
// Public APIs:
// - ItemType: Enum categorizing items
// - ItemRarity: Enum for item quality/rarity
// - Item: Structure defining item properties
// - ItemStack: Combination of item + quantity
//
#ifndef ITEM_H
#define ITEM_H

#include "engine/include/common.h"

// Item categories
typedef enum {
  ITEM_TYPE_WEAPON,
  ITEM_TYPE_ARMOR,
  ITEM_TYPE_CONSUMABLE,
  ITEM_TYPE_MATERIAL,
  ITEM_TYPE_QUEST,
  ITEM_TYPE_TOOL,
  ITEM_TYPE_MISC,
  ITEM_TYPE_COUNT
} ItemType;

// Item rarity/quality
typedef enum {
  ITEM_RARITY_COMMON,    // White
  ITEM_RARITY_UNCOMMON,  // Green
  ITEM_RARITY_RARE,      // Blue
  ITEM_RARITY_EPIC,      // Purple
  ITEM_RARITY_LEGENDARY, // Orange
  ITEM_RARITY_COUNT
} ItemRarity;

// Item flags
typedef enum {
  ITEM_FLAG_NONE = 0,
  ITEM_FLAG_STACKABLE = 1 << 0,
  ITEM_FLAG_TRADEABLE = 1 << 1,
  ITEM_FLAG_DROPPABLE = 1 << 2,
  ITEM_FLAG_CONSUMABLE = 1 << 3,
  ITEM_FLAG_EQUIPPABLE = 1 << 4,
  ITEM_FLAG_QUEST = 1 << 5,
  ITEM_FLAG_UNIQUE = 1 << 6,
} ItemFlag;

// Item definition (template from database)
typedef struct {
  u32 id;                  // Unique item ID
  const char *name;        // Display name
  const char *description; // Item description
  const char *icon_path;   // Path to icon texture

  ItemType type;     // Category
  ItemRarity rarity; // Quality tier
  u32 flags;         // ItemFlag bitfield

  u32 max_stack_size; // Max items per stack (1 for non-stackable)
  f32 weight;         // Weight per item
  u32 value;          // Base gold value

  // Type-specific data
  union {
    struct {
      f32 damage;
      f32 attack_speed;
      u32 durability;
    } weapon;

    struct {
      f32 defense;
      f32 magic_defense;
      u32 durability;
    } armor;

    struct {
      f32 healing;
      f32 mana_restore;
      f32 duration;
    } consumable;
  } data;
} Item;

// Item instance with quantity
typedef struct {
  u32 item_id;      // Item definition ID
  u32 quantity;     // Stack size (1 to max_stack_size)
  u32 durability;   // Current durability (for equipment)
  const Item *item; // Optional cached pointer to item template

  // Runtime data
  u32 instance_id;  // Unique instance ID for this stack
  bool is_equipped; // Is this item currently equipped?
} ItemStack;

// ============================================================================
// ITEM UTILITIES
// ============================================================================

// Check if item can stack
bool item_is_stackable(const Item *item);
bool item_can_stack_with(const ItemStack *a, const ItemStack *b);

// Stack operations
u32 item_stack_merge(ItemStack *dest, ItemStack *src);
bool item_stack_split(ItemStack *source, ItemStack *dest, u32 amount);

// Item comparison
bool item_equals(const Item *a, const Item *b);
const char *item_type_to_string(ItemType type);
const char *item_rarity_to_string(ItemRarity rarity);

// Item validation
bool item_is_valid(const Item *item);
bool item_stack_is_valid(const ItemStack *stack);

#endif // ITEM_H
