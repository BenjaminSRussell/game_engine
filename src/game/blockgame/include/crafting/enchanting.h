// include/crafting/enchanting.h
//
// Purpose: Defines the enchanting system for applying magical properties to
// items. Supports 30+ enchantments with level-based costs, lapis requirement,
// and bookshelf proximity bonuses.
//
// Public APIs:
// - `EnchantmentType`: Enumeration of 30+ enchantment types
// - `Enchantment`: Individual enchantment with level and properties
// - `EnchantingTable`: State for an enchanting table
// - `enchanting_table_init`: Initialize enchanting table
// - `enchanting_get_offers`: Get 3 random enchantment offers
// - `enchanting_apply`: Apply enchantment to item
// - `enchanting_calculate_cost`: Get cost in experience + lapis
// - `enchanting_can_apply`: Check if enchantment is valid for item
//
#ifndef ENCHANTING_H
#define ENCHANTING_H

#include "../block/block.h"
#include "../chunk/chunk.h"
#include "../game_common.h"
#include "../inventory/inventory.h"
#include "../inventory/item_registry.h"
#include <math/vec3.h>

// Enchantment types (30+)
typedef enum {
  // Armor enchantments
  ENCHANT_PROTECTION = 0,            // Reduces damage
  ENCHANT_FIRE_PROTECTION = 1,       // Reduces fire damage
  ENCHANT_FEATHER_FALLING = 2,       // Reduces fall damage
  ENCHANT_BLAST_PROTECTION = 3,      // Reduces explosion damage
  ENCHANT_PROJECTILE_PROTECTION = 4, // Reduces projectile damage
  ENCHANT_RESPIRATION = 5,           // Underwater breathing
  ENCHANT_AQUA_AFFINITY = 6,         // Faster mining underwater
  ENCHANT_THORNS = 7,                // Reflects damage

  // Tool/Weapon enchantments
  ENCHANT_SHARPNESS = 8,           // +Damage
  ENCHANT_SMITE = 9,               // +Damage to undead
  ENCHANT_BANE_OF_ARTHROPODS = 10, // +Damage to arthropods
  ENCHANT_KNOCKBACK = 11,          // Knockback on hit
  ENCHANT_FIRE_ASPECT = 12,        // Sets target on fire
  ENCHANT_LOOTING = 13,            // More loot drops
  ENCHANT_SWEEPING_EDGE = 14,      // Increased sweep damage
  ENCHANT_EFFICIENCY = 15,         // Faster mining
  ENCHANT_SILK_TOUCH = 16,         // Mine blocks as items
  ENCHANT_UNBREAKING = 17,         // Reduced durability loss
  ENCHANT_FORTUNE = 18,            // More block drops

  // Bow enchantments
  ENCHANT_POWER = 19,    // +Projectile damage
  ENCHANT_PUNCH = 20,    // Increased knockback
  ENCHANT_FLAME = 21,    // Flaming arrows
  ENCHANT_INFINITY = 22, // Infinite arrows

  // Tool enchantments
  ENCHANT_LUCK_OF_THE_SEA = 23, // Better fishing loot
  ENCHANT_LURE = 24,            // Faster fishing

  // Armor enchantments cont.
  ENCHANT_MENDING = 25,            // Repair with XP
  ENCHANT_CURSE_OF_BINDING = 26,   // Can't unequip
  ENCHANT_FROST_WALKER = 27,       // Walk on water
  ENCHANT_SWIFT_SNEAK = 28,        // Faster sneaking
  ENCHANT_CURSE_OF_VANISHING = 29, // Item disappears on death

  ENCHANT_COUNT = 30
} EnchantmentType;

// Individual enchantment
typedef struct {
  EnchantmentType type;
  u32 level;     // 1-5 for most, 1-10 for some
  u32 max_level; // Maximum level for this enchantment
} Enchantment;

// Enchanting offers (what appears on screen)
// TODO: Implement logic to generate 3 distinct offers based on bookshelf power.
typedef struct {
  Enchantment enchantment;
  u32 cost_levels;   // XP levels required (1-30)
  u32 cost_lapis;    // Lapis lazuli required (1-3)
  bool is_available; // Whether player can afford
} EnchantmentOffer;

// Enchanting table state
typedef struct {
  Vec3 position;
  Inventory inventory; // 2 slots: 0 = Item to enchant, 1 = Lapis Lazuli
  u32 bookshelf_count; // Adjacent bookshelves (max 15)
  EnchantmentOffer current_offers[3]; // 3 random offers available
  f32 enchantment_power;              // 0.0 - 30.0 based on bookshelves
  u32 xp_seed;                        // Seed for deterministic offer generation
} EnchantingTable;

// Initialize enchanting table
void enchanting_table_init(EnchantingTable *table, Vec3 position);

// Update enchanting table (check nearby bookshelves)
// TODO: Scan 1-block radius with 1-block air gap for bookshelves.
// TODO: Update enchantment_power based on valid bookshelves (max 15).
void enchanting_table_update(EnchantingTable *table,
                             ChunkManager *chunk_manager,
                             BlockRegistry *block_registry);

// Get available enchantment offers
// TODO: Use a seeded random number generator (based on player seed + item
// encryption seed) for consistent offers.
void enchanting_get_offers(EnchantingTable *table, EnchantmentOffer *out_offers,
                           u32 offer_count);

// Apply enchantment to item
// TODO: Deduct XP levels and Lapis Lazuli from player inventory.
// TODO: Add NBT data (or equivalent) to item for enchantments.
bool enchanting_apply(Enchantment *enchantment, InventorySlot *item_slot,
                      ItemRegistry *item_registry);

// Calculate enchantment cost
// TODO: Formula: BaseCost + (Level * Multiplier) - BookshelfReduction.
void enchanting_calculate_cost(EnchantingTable *table, u32 offer_index,
                               u32 *out_xp_cost, u32 *out_lapis_cost);

// Check if enchantment can be applied to item
// TODO: Check ItemType vs EnchantmentType (e.g., Sharpness only on
// Swords/Axes).
bool enchanting_can_apply(EnchantmentType enchant_type, u32 item_id,
                          ItemRegistry *item_registry);

// Check for conflicting enchantments
// TODO: Check exclusion rules (e.g., Silk Touch vs Fortune, Protection vs Blast
// Protection).
bool enchanting_has_conflict(const InventorySlot *item,
                             EnchantmentType new_enchant);

// Get enchantment name
const char *enchanting_get_name(EnchantmentType type);
u32 enchanting_get_min_power(EnchantmentType type, u32 level);
u32 enchanting_get_max_power(EnchantmentType type, u32 level);

// Get enchantment description
const char *enchanting_get_description(EnchantmentType type, u32 level);

// Calculate damage/protection bonus from enchantment
f32 enchanting_calculate_bonus(Enchantment *enchantment);

// Visual & audio helpers: emit particles and sounds when applying enchantments
#include <effects/vfx/particle_system.h>
struct AudioSystem; /* forward */
void enchanting_emit_apply_vfx(Enchantment *enchantment, Vec3 position,
                               struct ParticleSystem *particle_system,
                               struct AudioSystem *audio_system);

// Apply enchantment and emit VFX/audio at position if provided
bool enchanting_apply_with_vfx(Enchantment *enchantment,
                               InventorySlot *item_slot,
                               ItemRegistry *item_registry, Vec3 position,
                               struct ParticleSystem *particle_system,
                               struct AudioSystem *audio_system);

// Get enchantment rarity
typedef enum {
  RARITY_COMMON,
  RARITY_UNCOMMON,
  RARITY_RARE,
  RARITY_VERY_RARE
} EnchantmentRarity;

EnchantmentRarity enchanting_get_rarity(EnchantmentType type);

#endif // ENCHANTING_H
