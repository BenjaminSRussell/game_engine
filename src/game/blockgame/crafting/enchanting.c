// src/crafting/enchanting.c
//
// Purpose: Implementation of the enchanting system for applying magical
// properties to tools, weapons, and armor.
// TODO: Implement enchanting table visual effects (glyphs, particles).
// TODO: Add enchanting level requirement system.
// TODO: Implement enchantment compatibility checking.
// TODO: Add enchantment combination system for synergies.
// TODO: Implement enchantment removal system with grindstone.
// TODO: Add enchantment book system for storing enchantments.
// TODO: Implement enchanting cost scaling with item usage.
// TODO: Add enchanting preview system before applying.
// TODO: Implement enchantment rarity system affecting availability.
// TODO: Add enchanting animation system for table interactions.
#include <audio/audio_system.h>
#include <block/block.h>
#include <chunk/chunk.h>
#include <core/logger.h>
#include <crafting/enchanting.h>
#include <effects/vfx/particle_system.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Enchantment properties
typedef struct {
  const char *name;
  const char *description;
  EnchantmentRarity rarity;
  u32 max_level;
  u32 base_cost;        // Base XP cost at level 1
  bool can_be_treasure; // Can be obtained only from treasure (fishing, loot)
} EnchantmentProperty;

// Enchantment properties table
static const EnchantmentProperty enchant_properties[ENCHANT_COUNT] = {
    // PROTECTION
    [ENCHANT_PROTECTION] = {.name = "Protection",
                            .description = "Reduces most types of damage",
                            .rarity = RARITY_COMMON,
                            .max_level = 4,
                            .base_cost = 1,
                            .can_be_treasure = false},
    // FIRE_PROTECTION
    [ENCHANT_FIRE_PROTECTION] = {.name = "Fire Protection",
                                 .description =
                                     "Reduces fire damage and time on fire",
                                 .rarity = RARITY_COMMON,
                                 .max_level = 4,
                                 .base_cost = 1,
                                 .can_be_treasure = false},
    // FEATHER_FALLING
    [ENCHANT_FEATHER_FALLING] = {.name = "Feather Falling",
                                 .description = "Reduces fall damage",
                                 .rarity = RARITY_UNCOMMON,
                                 .max_level = 4,
                                 .base_cost = 1,
                                 .can_be_treasure = false},
    // BLAST_PROTECTION
    [ENCHANT_BLAST_PROTECTION] = {.name = "Blast Protection",
                                  .description =
                                      "Reduces explosion damage and knockback",
                                  .rarity = RARITY_UNCOMMON,
                                  .max_level = 4,
                                  .base_cost = 2,
                                  .can_be_treasure = false},
    // PROJECTILE_PROTECTION
    [ENCHANT_PROJECTILE_PROTECTION] = {.name = "Projectile Protection",
                                       .description =
                                           "Reduces projectile damage",
                                       .rarity = RARITY_UNCOMMON,
                                       .max_level = 4,
                                       .base_cost = 2,
                                       .can_be_treasure = false},
    // RESPIRATION
    [ENCHANT_RESPIRATION] = {.name = "Respiration",
                             .description = "Extends underwater breathing time",
                             .rarity = RARITY_RARE,
                             .max_level = 3,
                             .base_cost = 2,
                             .can_be_treasure = false},
    // AQUA_AFFINITY
    [ENCHANT_AQUA_AFFINITY] = {.name = "Aqua Affinity",
                               .description =
                                   "Increases mining speed underwater",
                               .rarity = RARITY_RARE,
                               .max_level = 1,
                               .base_cost = 2,
                               .can_be_treasure = false},
    // THORNS
    [ENCHANT_THORNS] = {.name = "Thorns",
                        .description = "Reflects damage back to attacker",
                        .rarity = RARITY_RARE,
                        .max_level = 3,
                        .base_cost = 4,
                        .can_be_treasure = false},
    // SHARPNESS
    [ENCHANT_SHARPNESS] = {.name = "Sharpness",
                           .description = "Increases melee damage",
                           .rarity = RARITY_COMMON,
                           .max_level = 5,
                           .base_cost = 1,
                           .can_be_treasure = false},
    // SMITE
    [ENCHANT_SMITE] = {.name = "Smite",
                       .description = "Increases damage to undead",
                       .rarity = RARITY_UNCOMMON,
                       .max_level = 5,
                       .base_cost = 1,
                       .can_be_treasure = false},
    // BANE_OF_ARTHROPODS
    [ENCHANT_BANE_OF_ARTHROPODS] = {.name = "Bane of Arthropods",
                                    .description =
                                        "Increases damage to arthropods",
                                    .rarity = RARITY_UNCOMMON,
                                    .max_level = 5,
                                    .base_cost = 1,
                                    .can_be_treasure = false},
    // KNOCKBACK
    [ENCHANT_KNOCKBACK] = {.name = "Knockback",
                           .description = "Increases knockback on hit",
                           .rarity = RARITY_UNCOMMON,
                           .max_level = 2,
                           .base_cost = 1,
                           .can_be_treasure = false},
    // FIRE_ASPECT
    [ENCHANT_FIRE_ASPECT] = {.name = "Fire Aspect",
                             .description = "Sets targets on fire",
                             .rarity = RARITY_RARE,
                             .max_level = 2,
                             .base_cost = 2,
                             .can_be_treasure = false},
    // LOOTING
    [ENCHANT_LOOTING] = {.name = "Looting",
                         .description = "Increases loot drops from enemies",
                         .rarity = RARITY_RARE,
                         .max_level = 3,
                         .base_cost = 2,
                         .can_be_treasure = false},
    // SWEEPING_EDGE
    [ENCHANT_SWEEPING_EDGE] = {.name = "Sweeping Edge",
                               .description = "Increases sweep attack damage",
                               .rarity = RARITY_UNCOMMON,
                               .max_level = 3,
                               .base_cost = 1,
                               .can_be_treasure = false},
    // EFFICIENCY
    [ENCHANT_EFFICIENCY] = {.name = "Efficiency",
                            .description = "Increases mining speed",
                            .rarity = RARITY_COMMON,
                            .max_level = 5,
                            .base_cost = 1,
                            .can_be_treasure = false},
    // SILK_TOUCH
    [ENCHANT_SILK_TOUCH] = {.name = "Silk Touch",
                            .description = "Mine blocks as items",
                            .rarity = RARITY_VERY_RARE,
                            .max_level = 1,
                            .base_cost = 4,
                            .can_be_treasure = false},
    // UNBREAKING
    [ENCHANT_UNBREAKING] = {.name = "Unbreaking",
                            .description = "Reduces durability loss",
                            .rarity = RARITY_COMMON,
                            .max_level = 3,
                            .base_cost = 1,
                            .can_be_treasure = false},
    // FORTUNE
    [ENCHANT_FORTUNE] = {.name = "Fortune",
                         .description = "Increases ore and drops",
                         .rarity = RARITY_VERY_RARE,
                         .max_level = 3,
                         .base_cost = 2,
                         .can_be_treasure = false},
    // POWER
    [ENCHANT_POWER] = {.name = "Power",
                       .description = "Increases projectile damage",
                       .rarity = RARITY_COMMON,
                       .max_level = 5,
                       .base_cost = 1,
                       .can_be_treasure = false},
    // PUNCH
    [ENCHANT_PUNCH] = {.name = "Punch",
                       .description = "Increases projectile knockback",
                       .rarity = RARITY_UNCOMMON,
                       .max_level = 2,
                       .base_cost = 2,
                       .can_be_treasure = false},
    // FLAME
    [ENCHANT_FLAME] = {.name = "Flame",
                       .description = "Arrows set targets on fire",
                       .rarity = RARITY_RARE,
                       .max_level = 1,
                       .base_cost = 2,
                       .can_be_treasure = false},
    // INFINITY
    [ENCHANT_INFINITY] = {.name = "Infinity",
                          .description = "Arrows never consumed",
                          .rarity = RARITY_VERY_RARE,
                          .max_level = 1,
                          .base_cost = 4,
                          .can_be_treasure = false},
    // LUCK_OF_THE_SEA
    [ENCHANT_LUCK_OF_THE_SEA] = {.name = "Luck of the Sea",
                                 .description = "Better fishing treasure",
                                 .rarity = RARITY_RARE,
                                 .max_level = 3,
                                 .base_cost = 2,
                                 .can_be_treasure = true},
    // LURE
    [ENCHANT_LURE] = {.name = "Lure",
                      .description = "Faster fishing",
                      .rarity = RARITY_UNCOMMON,
                      .max_level = 3,
                      .base_cost = 1,
                      .can_be_treasure = false},
    // MENDING
    [ENCHANT_MENDING] = {.name = "Mending",
                         .description = "Repair with experience orbs",
                         .rarity = RARITY_VERY_RARE,
                         .max_level = 1,
                         .base_cost = 4,
                         .can_be_treasure = true},
    // CURSE_OF_BINDING
    [ENCHANT_CURSE_OF_BINDING] = {.name = "Curse of Binding",
                                  .description = "Cannot unequip item",
                                  .rarity = RARITY_VERY_RARE,
                                  .max_level = 1,
                                  .base_cost = 4,
                                  .can_be_treasure = true},
    // FROST_WALKER
    [ENCHANT_FROST_WALKER] = {.name = "Frost Walker",
                              .description = "Walk on water",
                              .rarity = RARITY_RARE,
                              .max_level = 2,
                              .base_cost = 2,
                              .can_be_treasure = false},
    // SWIFT_SNEAK
    [ENCHANT_SWIFT_SNEAK] = {.name = "Swift Sneak",
                             .description = "Faster sneaking",
                             .rarity = RARITY_RARE,
                             .max_level = 3,
                             .base_cost = 2,
                             .can_be_treasure = false},
    // CURSE_OF_VANISHING
    [ENCHANT_CURSE_OF_VANISHING] = {.name = "Curse of Vanishing",
                                    .description = "Item disappears on death",
                                    .rarity = RARITY_VERY_RARE,
                                    .max_level = 1,
                                    .base_cost = 4,
                                    .can_be_treasure = true}};

// Initialize enchanting table
void enchanting_table_init(EnchantingTable *table, Vec3 position) {
  if (!table)
    return;

  memset(table, 0, sizeof(EnchantingTable));
  table->position = position;
  table->bookshelf_count = 0;
  table->enchantment_power = 0.0f;

  LOG_INFO("Enchanting table initialized at (%.0f, %.0f, %.0f)", position.x,
           position.y, position.z);
}

// Update enchanting table (count nearby bookshelves)
void enchanting_table_update(EnchantingTable *table,
                             ChunkManager *chunk_manager,
                             BlockRegistry *block_registry) {
  if (!table || !chunk_manager || !block_registry)
    return;

  table->bookshelf_count = 0;

  // Check for bookshelves in 5x5x5 area around table (up to 15)
  i32 cx = (i32)table->position.x;
  i32 cy = (i32)table->position.y;
  i32 cz = (i32)table->position.z;

  for (i32 x = cx - 2; x <= cx + 2; x++) {
    for (i32 y = cy - 2; y <= cy + 2; y++) {
      for (i32 z = cz - 2; z <= cz + 2; z++) {
        ChunkPos cp = world_to_chunk_pos(x, y, z);
        Chunk *chunk = chunk_manager_get(chunk_manager, cp);
        if (!chunk)
          continue;

        i32 local_x = x - cp.x * CHUNK_SIZE;
        i32 local_y = y - cp.y * CHUNK_SIZE;
        i32 local_z = z - cp.z * CHUNK_SIZE;

        BlockID block = chunk_get_block(chunk, local_x, local_y, local_z);

        // Bookshelf block nearby increases enchantment power.
        if (block == BLOCK_BOOKSHELF) {
          table->bookshelf_count++;
        }
      }
    }
  }

  // Cap at 15 bookshelves (15 gives max bonus)
  if (table->bookshelf_count > 15) {
    table->bookshelf_count = 15;
  }

  // Enchantment power = 0.75 + 1.0 * bookshelf_count / 2 (0.75 - 8.25)
  table->enchantment_power = 0.75f + (table->bookshelf_count * 1.0f) / 2.0f;

  LOG_DEBUG("Enchanting table updated: %u bookshelves, power: %.2f",
            table->bookshelf_count, table->enchantment_power);
}

// Get available enchantment offers
void enchanting_get_offers(EnchantingTable *table, EnchantmentOffer *out_offers,
                           u32 offer_count) {
  if (!table || !out_offers || offer_count == 0)
    return;

  // Use seeded random for consistent offers based on table position and seed
  srand(table->xp_seed + (u32)table->position.x + (u32)table->position.y + 
       (u32)table->position.z);

  // Generate 3 distinct enchantment offers based on bookshelf power
  u32 used_enchantments[3] = {0}; // Track used enchantments to avoid duplicates
  
  for (u32 i = 0; i < offer_count && i < 3; i++) {
    EnchantmentType enchant_type;
    u32 attempts = 0;
    
    // Try to find a distinct enchantment (avoid duplicates)
    do {
      // Weight enchantment selection based on rarity and bookshelf power
      f32 power_factor = table->enchantment_power / 8.25f; // Normalize to 0-1
      f32 rand_val = (f32)rand() / RAND_MAX;
      
      if (power_factor > 0.7f && rand_val < 0.1f) {
        // High power: chance for rare enchantments
        enchant_type = ENCHANT_MENDING + (rand() % (ENCHANT_COUNT - ENCHANT_MENDING));
      } else if (power_factor > 0.4f && rand_val < 0.3f) {
        // Medium power: uncommon enchantments
        enchant_type = ENCHANT_FEATHER_FALLING + (rand() % (ENCHANT_MENDING - ENCHANT_FEATHER_FALLING));
      } else {
        // Low power: common enchantments
        enchant_type = rand() % ENCHANT_FEATHER_FALLING;
      }
      
      attempts++;
    } while ((enchant_type == used_enchantments[0] || 
              enchant_type == used_enchantments[1] || 
              enchant_type == used_enchantments[2]) && attempts < 10);
    
    used_enchantments[i] = enchant_type;
    
    // Calculate level based on bookshelf power
    u32 max_level = enchant_properties[enchant_type].max_level;
    u32 level = 1;
    
    // Higher bookshelf power allows higher levels
    f32 level_chance = (table->enchantment_power / 8.25f) * (f32)max_level;
    level = 1 + (u32)(level_chance * ((f32)rand() / RAND_MAX));
    if (level > max_level) level = max_level;
    
    out_offers[i].enchantment.type = enchant_type;
    out_offers[i].enchantment.level = level;
    out_offers[i].enchantment.max_level = max_level;

    // Calculate cost based on enchantment rarity, level, and bookshelf power
    u32 base_cost = enchant_properties[enchant_type].base_cost;
    EnchantmentRarity rarity = enchant_properties[enchant_type].rarity;
    
    // Rarity multiplier
    f32 rarity_multiplier = 1.0f;
    switch (rarity) {
      case RARITY_UNCOMMON: rarity_multiplier = 1.5f; break;
      case RARITY_RARE: rarity_multiplier = 2.0f; break;
      case RARITY_VERY_RARE: rarity_multiplier = 3.0f; break;
      default: break;
    }
    
    // Level and power-based cost calculation
    f32 cost_multiplier = 1.0f + (level - 1) * 0.5f;
    f32 bookshelf_bonus = 1.0f + (table->enchantment_power / 30.0f) * 0.5f;
    
    out_offers[i].cost_levels = (u32)(base_cost * cost_multiplier * 
                                      rarity_multiplier * bookshelf_bonus);
    
    // Cap costs appropriately
    u32 max_cost = 15 + (u32)(table->enchantment_power * 1.5f);
    if (out_offers[i].cost_levels > max_cost) {
      out_offers[i].cost_levels = max_cost;
    }
    if (out_offers[i].cost_levels > 30) {
      out_offers[i].cost_levels = 30; // Hard cap at 30 levels
    }

    // Lapis cost scales with enchantment level
    out_offers[i].cost_lapis = 1 + (level / 2); // 1-3 lapis based on level
    
    // Check if player can afford (placeholder - would check player XP/Lapis)
    out_offers[i].is_available = true;
  }

  // Store offers in table for reference
  for (u32 i = 0; i < 3; i++) {
    table->current_offers[i] = out_offers[i];
  }

  LOG_DEBUG("Generated %u distinct enchantment offers with power %.2f", 
            offer_count, table->enchantment_power);
}

// Apply enchantment to item
bool enchanting_apply(Enchantment *enchantment, InventorySlot *item_slot,
                      ItemRegistry *item_registry) {
  if (!enchantment || !item_slot || !item_registry)
    return false;

  if (!enchanting_can_apply(enchantment->type, item_slot->item_id,
                            item_registry)) {
    LOG_WARN("Enchantment %d cannot be applied to item %u", enchantment->type,
             item_slot->item_id);
    return false;
  }

  if (enchanting_has_conflict(item_slot, enchantment->type)) {
    LOG_WARN("Enchantment %d conflicts with existing enchants on item %u",
             enchantment->type, item_slot->item_id);
    return false;
  }

  // Store enchantments on the item slot.
  // InventorySlot uses a void* to avoid a hard dependency on enchanting.h.
  Enchantment *enchants = (Enchantment *)item_slot->enchantments;

  // If enchant already exists, upgrade the level (clamped).
  for (u32 i = 0; i < item_slot->enchantment_count; i++) {
    if (enchants[i].type == enchantment->type) {
      u32 new_level = enchantment->level;
      if (new_level > enchants[i].max_level)
        new_level = enchants[i].max_level;
      if (new_level > enchants[i].level) {
        enchants[i].level = new_level;
      }

      LOG_INFO("Applied enchantment %d level %u to item %u", enchantment->type,
               enchants[i].level, item_slot->item_id);
      return true;
    }
  }

  // Ensure capacity.
  if (item_slot->enchantment_count == item_slot->enchantment_capacity) {
    u32 new_capacity = (item_slot->enchantment_capacity == 0)
                           ? 4
                           : (item_slot->enchantment_capacity * 2);
    void *new_mem = realloc(item_slot->enchantments,
                            (size_t)new_capacity * sizeof(Enchantment));
    if (!new_mem) {
      LOG_ERROR("Failed to allocate enchantment storage for item %u",
                item_slot->item_id);
      return false;
    }
    item_slot->enchantments = new_mem;
    item_slot->enchantment_capacity = new_capacity;
    enchants = (Enchantment *)item_slot->enchantments;
  }

  // Append.
  Enchantment stored = *enchantment;
  if (stored.type >= ENCHANT_COUNT) {
    LOG_WARN("Invalid enchantment type %d for item %u", stored.type,
             item_slot->item_id);
    return false;
  }
  if (stored.level > stored.max_level)
    stored.level = stored.max_level;

  enchants[item_slot->enchantment_count++] = stored;

  LOG_INFO("Applied enchantment %d level %u to item %u", enchantment->type,
           enchantment->level, item_slot->item_id);

  // Note: Visual/audio effects can be triggered by calling
  // `enchanting_emit_apply_vfx(enchantment, position, particle_system,
  // audio_system)` from higher-level code which has access to a position and
  // systems.

  return true;
}

// Emit visual/audio effects for applying an enchantment at a given position.
void enchanting_emit_apply_vfx(Enchantment *enchantment, Vec3 position,
                               ParticleSystem *particle_system,
                               struct AudioSystem *audio_system) {
  if (!enchantment || !particle_system)
    return;

  // Emit a burst of enchant particles and magic glow
  particle_emit_burst(particle_system, PARTICLE_TYPE_ENCHANT, position,
                      (Vec3){0.0f, 0.25f, 0.0f}, 0.2f, 12, 1.0f);
  particle_emit_burst(particle_system, PARTICLE_TYPE_MAGIC_GLOW, position,
                      (Vec3){0.0f, 0.18f, 0.0f}, 0.15f, 8, 1.3f);

  // Play sound if audio system is available
  if (audio_system && audio_system->initialized) {
    audio_play_sound_2d(audio_system, SOUND_CRAFTING_SUCCESS, 1.0f,
                        SOUND_CATEGORY_NEUTRAL);
  }
}

// Apply enchantment and trigger VFX/audio at a given position if systems are
// available.
bool enchanting_apply_with_vfx(Enchantment *enchantment,
                               InventorySlot *item_slot,
                               ItemRegistry *item_registry, Vec3 position,
                               struct ParticleSystem *particle_system,
                               struct AudioSystem *audio_system) {
  bool res = enchanting_apply(enchantment, item_slot, item_registry);
  if (res) {
    enchanting_emit_apply_vfx(enchantment, position, particle_system,
                              audio_system);
  }
  return res;
}

// Calculate enchantment cost
void enchanting_calculate_cost(EnchantingTable *table, u32 offer_index,
                               u32 *out_xp_cost, u32 *out_lapis_cost) {
  if (!table || offer_index >= 3 || !out_xp_cost || !out_lapis_cost)
    return;

  *out_xp_cost = table->current_offers[offer_index].enchantment.max_level;
  *out_lapis_cost =
      1 + (table->current_offers[offer_index].enchantment.level / 2);
}

// Check if enchantment can be applied to item
bool enchanting_can_apply(EnchantmentType enchant_type, u32 item_id,
                          ItemRegistry *item_registry) {
  if (!item_registry)
    return false;

  const ExtendedItemDefinition *item =
      item_registry_get(item_registry, item_id);
  if (!item)
    return false;

  // Define which items can use which enchantments
  switch (enchant_type) {
  // Armor enchantments
  case ENCHANT_PROTECTION:
  case ENCHANT_FIRE_PROTECTION:
  case ENCHANT_BLAST_PROTECTION:
  case ENCHANT_PROJECTILE_PROTECTION:
  case ENCHANT_THORNS:
    return item->base.item_type == ITEM_TYPE_ARMOR;

  // Specific armor slot enchantments
  case ENCHANT_FEATHER_FALLING:
  case ENCHANT_FROST_WALKER:
    return item->base.item_type == ITEM_TYPE_ARMOR &&
           item->properties.armor.armor_slot == 3; // Boots

  case ENCHANT_RESPIRATION:
  case ENCHANT_AQUA_AFFINITY:
    return item->base.item_type == ITEM_TYPE_ARMOR &&
           item->properties.armor.armor_slot == 0; // Helmet

  // Tool enchantments
  case ENCHANT_EFFICIENCY:
  case ENCHANT_SILK_TOUCH:
  case ENCHANT_FORTUNE:
  case ENCHANT_UNBREAKING:
    return item->base.item_type == ITEM_TYPE_TOOL;

  // Weapon enchantments
  case ENCHANT_SHARPNESS:
  case ENCHANT_SMITE:
  case ENCHANT_BANE_OF_ARTHROPODS:
  case ENCHANT_KNOCKBACK:
  case ENCHANT_FIRE_ASPECT:
  case ENCHANT_LOOTING:
  case ENCHANT_SWEEPING_EDGE:
    return item->base.item_type == ITEM_TYPE_WEAPON &&
           !item->properties.weapon.is_ranged;

  // Bow enchantments
  case ENCHANT_POWER:
  case ENCHANT_PUNCH:
  case ENCHANT_FLAME:
  case ENCHANT_INFINITY:
    return item_id == ITEM_BOW || item_id == ITEM_CROSSBOW;

  // Fishing rod enchantments
  case ENCHANT_LUCK_OF_THE_SEA:
  case ENCHANT_LURE:
    return item_id == ITEM_FISHING_ROD;

  // Misc
  case ENCHANT_MENDING:
  case ENCHANT_CURSE_OF_VANISHING:
    return true; // Can be applied to any item

  case ENCHANT_CURSE_OF_BINDING:
    return item->base.item_type == ITEM_TYPE_ARMOR;

  case ENCHANT_SWIFT_SNEAK:
    return item->base.item_type == ITEM_TYPE_ARMOR &&
           item->properties.armor.armor_slot == 2; // Leggings

  default:
    return false;
  }
}

// Check for conflicting enchantments
bool enchanting_has_conflict(const InventorySlot *item,
                             EnchantmentType new_enchant) {
  if (!item)
    return false;

  if (!item->enchantments || item->enchantment_count == 0) {
    return false;
  }

  // Cast void pointer to Enchantment array
  const Enchantment *enchantments = (const Enchantment *)item->enchantments;

  // Check item's existing enchantments and detect conflicts
  for (u32 i = 0; i < item->enchantment_count; i++) {
    EnchantmentType existing = enchantments[i].type;

    // Silk Touch conflicts with Fortune
    if ((existing == ENCHANT_SILK_TOUCH && new_enchant == ENCHANT_FORTUNE) ||
        (existing == ENCHANT_FORTUNE && new_enchant == ENCHANT_SILK_TOUCH)) {
      return true;
    }

    // Protection variants conflict with each other
    bool existing_protection = (existing == ENCHANT_PROTECTION ||
                                existing == ENCHANT_FIRE_PROTECTION ||
                                existing == ENCHANT_BLAST_PROTECTION ||
                                existing == ENCHANT_PROJECTILE_PROTECTION);
    bool new_protection = (new_enchant == ENCHANT_PROTECTION ||
                           new_enchant == ENCHANT_FIRE_PROTECTION ||
                           new_enchant == ENCHANT_BLAST_PROTECTION ||
                           new_enchant == ENCHANT_PROJECTILE_PROTECTION);

    if (existing_protection && new_protection) {
      return true;
    }

    // Mending conflicts with Infinity (bows only)
    if ((existing == ENCHANT_MENDING && new_enchant == ENCHANT_INFINITY) ||
        (existing == ENCHANT_INFINITY && new_enchant == ENCHANT_MENDING)) {
      return true;
    }

    // Sharpness, Smite, and Bane of Arthropods conflict (swords)
    bool existing_damage =
        (existing == ENCHANT_SHARPNESS || existing == ENCHANT_SMITE ||
         existing == ENCHANT_BANE_OF_ARTHROPODS);
    bool new_damage =
        (new_enchant == ENCHANT_SHARPNESS || new_enchant == ENCHANT_SMITE ||
         new_enchant == ENCHANT_BANE_OF_ARTHROPODS);

    if (existing_damage && new_damage) {
      return true;
    }
  }

  return false;
}

// Get enchantment name
const char *enchanting_get_name(EnchantmentType type) {
  if (type < ENCHANT_COUNT) {
    return enchant_properties[type].name;
  }
  return "Unknown";
}

// Get enchantment description
const char *enchanting_get_description(EnchantmentType type, u32 level) {
  if (type >= ENCHANT_COUNT)
    return "Unknown";

  return enchant_properties[type].description;
}

// Calculate bonus from enchantment
f32 enchanting_calculate_bonus(Enchantment *enchantment) {
  if (!enchantment)
    return 0.0f;

  // Bonuses vary by enchantment type
  switch (enchantment->type) {
  case ENCHANT_SHARPNESS:
    return 0.625f * enchantment->level + 0.375f; // +0.625/level

  case ENCHANT_PROTECTION:
  case ENCHANT_FIRE_PROTECTION:
  case ENCHANT_BLAST_PROTECTION:
  case ENCHANT_PROJECTILE_PROTECTION:
    return enchantment->level * 0.04f; // 4% per level

  case ENCHANT_EFFICIENCY:
    return enchantment->level * enchantment->level + 1; // Quadratic growth

  case ENCHANT_UNBREAKING:
    return 1.0f / (enchantment->level + 1.0f); // 50%, 33%, 25%

  case ENCHANT_POWER:
    return enchantment->level * 0.5f + 1.0f; // +0.5/level

  default:
    return 0.0f;
  }
}

// Get enchantment rarity
EnchantmentRarity enchanting_get_rarity(EnchantmentType type) {
  if (type < ENCHANT_COUNT) {
    return enchant_properties[type].rarity;
  }
  return RARITY_COMMON;
}
