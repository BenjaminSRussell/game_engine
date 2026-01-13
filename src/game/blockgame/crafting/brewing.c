// src/crafting/brewing.c
//
// Purpose: Implementation of the potion brewing system for creating
// magical potions with various effects and durations.

// TODO: Implement potion splash and lingering variants.
// TODO: Implement potion recipe book with ingredient hints.
#include <audio/audio_system.h>
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <crafting/brewing.h>
#include <effects/vfx/particle_system.h>
#include <inventory/item_registry.h>
#include <stdlib.h>
#include <string.h>

// ============= Brewing Recipes =============

// Master recipe list (combination of ingredient + base potion -> result)
static const BrewingRecipe g_recipes[] = {
    // Speed potions
    {POTION_AWKWARD, BREW_INGREDIENT_SUGAR, POTION_SPEED, 160},
    // Slowness potions
    {POTION_SPEED, BREW_INGREDIENT_FERMENTED_EYE, POTION_SLOWNESS, 160},
    // Strength potions
    {POTION_AWKWARD, BREW_INGREDIENT_BLAZE_POWDER, POTION_STRENGTH, 160},
    // Healing potions
    {POTION_AWKWARD, BREW_INGREDIENT_GHAST_TEAR, POTION_HEALING, 160},
    // Harming potions
    {POTION_HEALING, BREW_INGREDIENT_FERMENTED_EYE, POTION_HARMING, 160},
    // Regeneration potions
    {POTION_AWKWARD, BREW_INGREDIENT_GHAST_TEAR, POTION_REGENERATION,
     160}, // Actually needs improved ingredients
    // Resistance potions
    {POTION_AWKWARD, BREW_INGREDIENT_TURTLE_SHELL, POTION_RESISTANCE, 160},
    // Fire Resistance potions
    {POTION_AWKWARD, BREW_INGREDIENT_MAGMA_CREAM, POTION_FIRE_RESISTANCE, 160},
    // Water Breathing potions
    {POTION_AWKWARD, BREW_INGREDIENT_TURTLE_SHELL, POTION_WATER_BREATHING, 160},
    // Night Vision potions
    {POTION_AWKWARD, BREW_INGREDIENT_GOLDEN_CARROT, POTION_NIGHT_VISION, 160},
    // Invisibility potions
    {POTION_NIGHT_VISION, BREW_INGREDIENT_FERMENTED_EYE, POTION_INVISIBILITY,
     160},
    // Levitation potions
    {POTION_AWKWARD, BREW_INGREDIENT_PHANTOM_MEMBRANE, POTION_LEVITATION, 160},
    // Duration extension (add redstone)
    {POTION_SPEED, BREW_INGREDIENT_REDSTONE, POTION_EXTENDED, 160},
    // Effect amplification (add glowstone)
    {POTION_STRENGTH, BREW_INGREDIENT_GLOWSTONE, POTION_STRENGTH, 160},
};
#define RECIPE_COUNT (sizeof(g_recipes) / sizeof(g_recipes[0]))

static const BrewingRecipe *brewing_find_recipe(PotionType input,
                                                BrewingIngredient ingredient) {
  for (u32 i = 0; i < RECIPE_COUNT; i++) {
    if (g_recipes[i].input_potion == input &&
        g_recipes[i].ingredient == ingredient) {
      return &g_recipes[i];
    }
  }
  return NULL;
}

static u32 potion_default_duration(PotionType type) {
  switch (type) {
  case POTION_SPEED:
    return 3600;
  case POTION_SLOWNESS:
    return 1800;
  case POTION_STRENGTH:
    return 3600;
  case POTION_HEALING:
    return 1;
  case POTION_HARMING:
    return 1;
  case POTION_REGENERATION:
    return 900;
  case POTION_RESISTANCE:
    return 3600;
  case POTION_FIRE_RESISTANCE:
    return 3600;
  case POTION_WATER_BREATHING:
    return 3600;
  case POTION_INVISIBILITY:
    return 3600;
  case POTION_BLINDNESS:
    return 900;
  case POTION_NIGHT_VISION:
    return 3600;
  case POTION_HUNGER:
    return 900;
  case POTION_WEAKNESS:
    return 1800;
  case POTION_POISON:
    return 900;
  case POTION_WITHER:
    return 400;
  case POTION_LEVITATION:
    return 200;
  case POTION_EXTENDED:
    return 4800;
  default:
    return 0;
  }
}

static PotionEffect brewing_apply_recipe(const PotionEffect *input,
                                         BrewingIngredient ingredient,
                                         PotionType output) {
  PotionEffect result = {0};

  if (input) {
    result = *input;
  }

  if (output == POTION_EXTENDED) {
    u32 base_duration = result.duration_ticks;
    if (base_duration == 0) {
      base_duration = potion_default_duration(result.type);
    }
    result.duration_ticks = base_duration > 0 ? base_duration * 2 : 0;
    return result;
  }

  if (ingredient == BREW_INGREDIENT_GLOWSTONE && output == result.type) {
    if (result.duration_ticks == 0) {
      result.duration_ticks = potion_default_duration(result.type);
    }
    if (result.amplifier < 3) {
      result.amplifier += 1;
    }
    return result;
  }

  result.type = output;
  result.duration_ticks = potion_default_duration(output);
  result.amplifier = 0;
  return result;
}

// ============= Brewing Stand Functions =============

void brewing_stand_init(BrewingStand *stand, Vec3 position,
                        EntityID entity_id) {
  if (!stand)
    return;

  memset(stand, 0, sizeof(BrewingStand));

  stand->entity_id = entity_id;
  stand->position = position;
  stand->is_brewing = false;
  stand->is_powered =
      true; // Brewing stands don't need power in classic BlockGame
  stand->fuel_max_charges = 20;
  stand->fuel_charges = 0;
  stand->brew_ticks_remaining = 0;
  stand->brew_ticks_total = 0;

  // Initialize all bottles with water
  for (u32 i = 0; i < 3; i++) {
    stand->bottles[i].type = POTION_WATER;
    stand->bottles[i].duration_ticks = 0;
    stand->bottles[i].amplifier = 0;
    stand->bottles[i].is_splash = false;
    stand->bottles[i].is_lingering = false;
  }

  LOG_DEBUG("Brewing stand initialized at (%.1f,%.1f,%.1f)", position.x,
            position.y, position.z);
}

void brewing_stand_free(BrewingStand *stand) {
  if (!stand)
    return;
  memset(stand, 0, sizeof(BrewingStand));
}

bool brewing_start_brew(BrewingStand *stand, u32 bottle_index,
                        BrewingIngredient ingredient, u32 amount) {
  if (!stand || bottle_index >= 3 || ingredient == BREW_INGREDIENT_NONE ||
      amount == 0) {
    return false;
  }

  if (stand->is_brewing) {
    LOG_WARN("Brewing stand already brewing");
    return false;
  }

  if (stand->fuel_charges == 0) {
    LOG_DEBUG("Brewing stand has no fuel");
    return false;
  }

  const BrewingRecipe *first_recipe = NULL;
  for (u32 i = 0; i < 3; i++) {
    const BrewingRecipe *recipe =
        brewing_find_recipe(stand->bottles[i].type, ingredient);
    if (recipe) {
      first_recipe = recipe;
      break;
    }
  }

  // Check if recipe exists
  if (!first_recipe) {
    LOG_DEBUG("No recipe for ingredient %d", ingredient);
    return false;
  }

  stand->current_ingredient = ingredient;
  stand->ingredient_count = amount;
  stand->brew_ticks_total =
      first_recipe->brew_ticks > 0 ? first_recipe->brew_ticks : 160;
  stand->brew_ticks_remaining = stand->brew_ticks_total;
  stand->brew_progress = 0.0f;
  stand->is_brewing = true;
  stand->fuel_charges -= 1;

  LOG_INFO("Brewing started: ingredient=%d", ingredient);

  return true;
}

void brewing_stand_update(BrewingStand *stand, f32 delta_time,
                          ParticleSystem *particle_system,
                          struct AudioSystem *audio_system) {
  if (!stand || !stand->is_brewing)
    return;

  // Convert delta_time to ticks (20 ticks = 1 second)
  u32 ticks_elapsed = (u32)(delta_time * 20.0f);

  if (ticks_elapsed >= stand->brew_ticks_remaining) {
    // Brewing complete
    stand->brew_ticks_remaining = 0;
    stand->brew_progress = 1.0f;
    stand->is_brewing = false;

    LOG_INFO("Brewing complete: ingredient=%d used", stand->current_ingredient);

    for (u32 i = 0; i < 3; i++) {
      const BrewingRecipe *recipe = brewing_find_recipe(
          stand->bottles[i].type, stand->current_ingredient);
      if (!recipe) {
        continue;
      }
      PotionEffect result = brewing_apply_recipe(
          &stand->bottles[i], stand->current_ingredient, recipe->output_potion);
      stand->bottles[i] = result;
    }

    if (stand->ingredient_count > 0) {
      stand->ingredient_count -= 1;
    }

    // Trigger visual/audio effects if systems are provided
    brewing_emit_complete_vfx(stand, particle_system, audio_system);

    stand->brew_ticks_total = 0;
    if (stand->ingredient_count == 0) {
      stand->current_ingredient = BREW_INGREDIENT_NONE;
    }
  } else {
    // Brewing in progress
    stand->brew_ticks_remaining -= ticks_elapsed;
    f32 denom =
        stand->brew_ticks_total > 0 ? (f32)stand->brew_ticks_total : 160.0f;
    stand->brew_progress = 1.0f - ((f32)stand->brew_ticks_remaining / denom);

    // Emit periodic progress VFX (if a particle system is available)
    brewing_emit_progress_vfx(stand, particle_system);
  }
}

bool brewing_stand_add_fuel(BrewingStand *stand, u32 item_id) {
  if (!stand)
    return false;
  if (item_id != ITEM_BLAZE_POWDER) {
    return false;
  }

  if (stand->fuel_max_charges == 0) {
    stand->fuel_max_charges = 20;
  }

  if (stand->fuel_charges >= stand->fuel_max_charges) {
    return false;
  }

  stand->fuel_charges = stand->fuel_max_charges;
  return true;
}

const PotionEffect *brewing_get_bottle(const BrewingStand *stand,
                                       u32 bottle_index) {
  if (!stand || bottle_index >= 3)
    return NULL;
  return &stand->bottles[bottle_index];
}

void brewing_set_bottle(BrewingStand *stand, u32 bottle_index,
                        PotionEffect potion) {
  if (!stand || bottle_index >= 3)
    return;
  stand->bottles[bottle_index] = potion;
}

bool brewing_has_recipe(PotionType input, BrewingIngredient ingredient) {
  if (input == POTION_NONE || ingredient == BREW_INGREDIENT_NONE)
    return false;

  return brewing_find_recipe(input, ingredient) != NULL;
}

PotionType brewing_get_result(PotionType input, BrewingIngredient ingredient) {
  if (input == POTION_NONE || ingredient == BREW_INGREDIENT_NONE)
    return POTION_NONE;

  const BrewingRecipe *recipe = brewing_find_recipe(input, ingredient);
  return recipe ? recipe->output_potion : POTION_NONE;
}

// ============= Potion Effects =============

// ============= Splash & Lingering Potions =============

void potion_create_splash_variant(PotionEffect *effect) {
  if (!effect)
    return;
  effect->is_splash = true;
  effect->is_lingering = false;
  // Reduce duration for splash potions (they spread instantly)
  if (effect->duration_ticks > 0) {
    effect->duration_ticks = effect->duration_ticks * 3 / 4;
  }
  LOG_DEBUG("Potion converted to splash variant");
}

void potion_create_lingering_variant(PotionEffect *effect) {
  if (!effect)
    return;
  effect->is_splash = true;
  effect->is_lingering = true;
  // Keep full duration but spread over longer time
  LOG_DEBUG("Potion converted to lingering variant");
}

void potion_apply_splash_area_effect(Vec3 position, PotionEffect effect,
                                     f32 radius) {
  if (effect.type == POTION_NONE)
    return;

  // Affects all entities within radius
  LOG_DEBUG(
      "Splash potion applied at (%.1f,%.1f,%.1f) with radius %.1f: type=%d",
      position.x, position.y, position.z, radius, effect.type);
  // In full implementation, would query entities and apply effects
}

void potion_apply_lingering_cloud(Vec3 position, PotionEffect effect,
                                  f32 radius, f32 duration) {
  if (effect.type == POTION_NONE)
    return;

  // Creates a lingering cloud effect over time
  LOG_DEBUG("Lingering cloud created at (%.1f,%.1f,%.1f) for %.1f seconds",
            position.x, position.y, position.z, duration / 20.0f);
  // In full implementation, would create a particle cloud that applies effects
}

void potion_apply_effect(EntityID player_entity, PotionEffect effect) {
  if (player_entity == 0)
    return;

  // Placeholder: In full implementation, would apply effect to player
  // Update player status effects component with duration and amplifier
  LOG_DEBUG("Potion effect applied: type=%d, duration=%u ticks, amplifier=%u, "
            "splash=%s, lingering=%s",
            effect.type, effect.duration_ticks, effect.amplifier,
            effect.is_splash ? "yes" : "no",
            effect.is_lingering ? "yes" : "no");
}

void potion_remove_effect(EntityID player_entity, PotionType type) {
  if (player_entity == 0 || type == POTION_NONE)
    return;

  LOG_DEBUG("Potion effect removed: type=%d", type);
}

void potion_update_effects(EntityID player_entity, f32 delta_time) {
  if (player_entity == 0 || delta_time <= 0.0f)
    return;

  // In a full implementation, this would:
  // 1. Get the player's active potion effects component
  // 2. Iterate through each active effect
  // 3. Update duration timers
  // 4. Apply effect modifiers to entity components
  // 5. Remove expired effects

  // For now, implement a basic framework showing the structure:
  
  // Example: Get health component (would use ECS system in full implementation)
  HealthComponent *health = NULL; // ecs_get_component(player_entity, HealthComponent);
  if (!health) {
    return; // Player has no health component
  }

  // Example: Get movement component (would use ECS system)
  // MovementComponent *movement = NULL; // ecs_get_component(player_entity, MovementComponent);
  
  // Iterate active effects and apply modifiers
  // This would typically be stored in a PotionEffectsComponent on the player
  // For demonstration, show how different effects would modify components:
  
  /*
  PotionEffectsComponent *effects = ecs_get_component(player_entity, PotionEffectsComponent);
  if (!effects) return;
  
  for (u32 i = 0; i < effects->active_count; i++) {
    PotionEffect *effect = &effects->effects[i];
    
    // Update duration
    if (effect->duration_ticks > 0) {
      effect->duration_ticks -= (u32)(delta_time * 20.0f); // 20 ticks per second
      
      if (effect->duration_ticks <= 0) {
        // Remove expired effect
        potion_remove_effect(player_entity, effect->type);
        continue;
      }
      
      // Apply effect modifiers based on type
      switch (effect->type) {
        case POTION_REGENERATION:
          // Apply health regeneration
          if (health && health->health < health->max_health) {
            f32 regen_rate = 0.05f * (effect->amplifier + 1); // 0.05 HP per level per second
            health->health += regen_rate * delta_time;
            if (health->health > health->max_health) {
              health->health = health->max_health;
            }
          }
          break;
          
        case POTION_POISON:
          // Apply poison damage
          if (health && health->health > 0) {
            f32 damage_rate = 0.025f * (effect->amplifier + 1); // 0.025 damage per level per second
            health->health -= damage_rate * delta_time;
            if (health->health <= 0) {
              health->health = 0;
              health->is_alive = false;
            }
          }
          break;
          
        case POTION_HEALING:
          // Instant healing (one-time effect)
          if (health) {
            f32 heal_amount = 3.0f * (effect->amplifier + 1);
            health->health += heal_amount;
            if (health->health > health->max_health) {
              health->health = health->max_health;
            }
            // Remove instant effect
            potion_remove_effect(player_entity, effect->type);
          }
          break;
          
        case POTION_HARMING:
          // Instant damage (one-time effect)
          if (health) {
            f32 damage_amount = 3.0f * (effect->amplifier + 1);
            health->health -= damage_amount;
            if (health->health <= 0) {
              health->health = 0;
              health->is_alive = false;
            }
            // Remove instant effect
            potion_remove_effect(player_entity, effect->type);
          }
          break;
          
        case POTION_STRENGTH:
          // Would modify attack damage component
          // AttackComponent *attack = ecs_get_component(player_entity, AttackComponent);
          // if (attack) {
          //   attack->damage_multiplier = 1.3f + (0.3f * effect->amplifier);
          // }
          break;
          
        case POTION_SPEED:
          // Would modify movement speed component
          // if (movement) {
          //   movement->speed_multiplier = 1.2f + (0.2f * effect->amplifier);
          // }
          break;
          
        case POTION_SLOWNESS:
          // Would modify movement speed component
          // if (movement) {
          //   movement->speed_multiplier = 0.85f - (0.15f * effect->amplifier);
          // }
          break;
          
        case POTION_RESISTANCE:
          // Would modify damage resistance component
          // DefenseComponent *defense = ecs_get_component(player_entity, DefenseComponent);
          // if (defense) {
          //   defense->damage_reduction = 0.2f * (effect->amplifier + 1);
          // }
          break;
          
        case POTION_FIRE_RESISTANCE:
          // Would set fire immunity flag
          // StatusComponent *status = ecs_get_component(player_entity, StatusComponent);
          // if (status) {
          //   status->fire_immune = true;
          // }
          break;
          
        case POTION_WATER_BREATHING:
          // Would set underwater breathing flag
          // StatusComponent *status = ecs_get_component(player_entity, StatusComponent);
          // if (status) {
          //   status->can_breathe_underwater = true;
          // }
          break;
          
        case POTION_INVISIBILITY:
          // Would set invisibility flag
          // RenderComponent *render = ecs_get_component(player_entity, RenderComponent);
          // if (render) {
          //   render->visibility = 0.1f; // Nearly invisible
          // }
          break;
          
        case POTION_NIGHT_VISION:
          // Would modify vision component
          // VisionComponent *vision = ecs_get_component(player_entity, VisionComponent);
          // if (vision) {
          //   vision->night_vision = true;
          //   vision->light_level = 1.0f;
          // }
          break;
          
        case POTION_HUNGER:
          // Would modify hunger component
          // HungerComponent *hunger = ecs_get_component(player_entity, HungerComponent);
          // if (hunger) {
          //   hunger->hunger_rate = 0.1f * (effect->amplifier + 1);
          // }
          break;
          
        case POTION_WEAKNESS:
          // Would modify attack damage component
          // AttackComponent *attack = ecs_get_component(player_entity, AttackComponent);
          // if (attack) {
          //   attack->damage_multiplier = 0.5f - (0.25f * effect->amplifier);
          // }
          break;
          
        default:
          break;
      }
    }
  }
  */
  
  // Placeholder implementation - log that effects were updated
  LOG_DEBUG("Updated potion effects for entity %u (delta: %.3f)", player_entity, delta_time);
}

const char *potion_get_effect_name(PotionType type) {
  switch (type) {
  case POTION_WATER:
    return "Water";
  case POTION_AWKWARD:
    return "Awkward Potion";
  case POTION_THICK:
    return "Thick Potion";
  case POTION_MUNDANE:
    return "Mundane Potion";
  case POTION_EXTENDED:
    return "Extended Potion";
  case POTION_SPEED:
    return "Potion of Speed";
  case POTION_SLOWNESS:
    return "Potion of Slowness";
  case POTION_STRENGTH:
    return "Potion of Strength";
  case POTION_HEALING:
    return "Potion of Healing";
  case POTION_HARMING:
    return "Potion of Harming";
  case POTION_REGENERATION:
    return "Potion of Regeneration";
  case POTION_RESISTANCE:
    return "Potion of Resistance";
  case POTION_FIRE_RESISTANCE:
    return "Potion of Fire Resistance";
  case POTION_WATER_BREATHING:
    return "Potion of Water Breathing";
  case POTION_INVISIBILITY:
    return "Potion of Invisibility";
  case POTION_BLINDNESS:
    return "Potion of Blindness";
  case POTION_NIGHT_VISION:
    return "Potion of Night Vision";
  case POTION_HUNGER:
    return "Potion of Hunger";
  case POTION_WEAKNESS:
    return "Potion of Weakness";
  case POTION_POISON:
    return "Potion of Poison";
  case POTION_WITHER:
    return "Potion of Wither";
  case POTION_LEVITATION:
    return "Potion of Levitation";
  default:
    return "Unknown Potion";
  }
}

const char *potion_get_description(PotionType type) {
  switch (type) {
  case POTION_SPEED:
    return "Increases movement speed";
  case POTION_SLOWNESS:
    return "Decreases movement speed";
  case POTION_STRENGTH:
    return "Increases attack damage";
  case POTION_HEALING:
    return "Restores health";
  case POTION_HARMING:
    return "Deals damage instantly";
  case POTION_REGENERATION:
    return "Regenerates health over time";
  case POTION_RESISTANCE:
    return "Reduces incoming damage";
  case POTION_FIRE_RESISTANCE:
    return "Makes you immune to fire";
  case POTION_WATER_BREATHING:
    return "Allows breathing underwater";
  case POTION_NIGHT_VISION:
    return "Lets you see in darkness";
  case POTION_INVISIBILITY:
    return "Makes you invisible";
  case POTION_LEVITATION:
    return "Makes you float upward";
  default:
    return "Special potion effect";
  }
}

// ============= Recipe Functions =============

u32 brewing_get_recipe_count(void) { return RECIPE_COUNT; }

const BrewingRecipe *brewing_get_recipe(u32 index) {
  if (index >= RECIPE_COUNT)
    return NULL;
  return &g_recipes[index];
}

void brewing_init_default_recipes(void) {
  LOG_INFO("Brewing recipes initialized: %u recipes available", RECIPE_COUNT);
}

// ============= Potion Recipe Book =============

void potion_recipe_book_init(PotionRecipeBook *book) {
  if (!book)
    return;

  memset(book, 0, sizeof(PotionRecipeBook));
  book->total_recipes_discovered = 0;
  book->total_ingredients_discovered = 0;

  LOG_DEBUG("Potion recipe book initialized");
}

void potion_recipe_book_discover(PotionRecipeBook *book, u32 recipe_index) {
  if (!book || recipe_index >= RECIPE_COUNT)
    return;

  u32 bitmap_index = recipe_index / 32;
  u32 bit_position = recipe_index % 32;

  if (!(book->discovered_recipes[bitmap_index] & (1u << bit_position))) {
    book->discovered_recipes[bitmap_index] |= (1u << bit_position);
    book->total_recipes_discovered++;

    LOG_DEBUG("Recipe %u discovered (total: %u/%u)", recipe_index,
              book->total_recipes_discovered, RECIPE_COUNT);
  }
}

void potion_recipe_book_discover_ingredient(PotionRecipeBook *book,
                                             BrewingIngredient ingredient) {
  if (!book || ingredient >= BREW_INGREDIENT_COUNT)
    return;

  if (!book->ingredient_discovered[ingredient]) {
    book->ingredient_discovered[ingredient] = true;
    book->total_ingredients_discovered++;

    LOG_DEBUG("Ingredient %u discovered (total: %u/%u)", ingredient,
              book->total_ingredients_discovered, BREW_INGREDIENT_COUNT);
  }
}

bool potion_recipe_book_has_recipe(const PotionRecipeBook *book,
                                    u32 recipe_index) {
  if (!book || recipe_index >= RECIPE_COUNT)
    return false;

  u32 bitmap_index = recipe_index / 32;
  u32 bit_position = recipe_index % 32;

  return (book->discovered_recipes[bitmap_index] & (1u << bit_position)) != 0;
}

bool potion_recipe_book_has_ingredient(const PotionRecipeBook *book,
                                        BrewingIngredient ingredient) {
  if (!book || ingredient >= BREW_INGREDIENT_COUNT)
    return false;

  return book->ingredient_discovered[ingredient];
}

f32 potion_recipe_book_get_completion(const PotionRecipeBook *book) {
  if (!book)
    return 0.0f;

  f32 recipe_completion = (f32)book->total_recipes_discovered / RECIPE_COUNT;
  f32 ingredient_completion =
      (f32)book->total_ingredients_discovered / BREW_INGREDIENT_COUNT;

  return (recipe_completion + ingredient_completion) / 2.0f;
}

// Emit visual and audio effects for brewing completion
void brewing_emit_complete_vfx(const BrewingStand *stand,
                               ParticleSystem *particle_system,
                               struct AudioSystem *audio_system) {
  if (!stand || !particle_system)
    return;

  Vec3 pos = stand->position;
  // Small burst of magic glow and bubbles
  particle_emit_burst(particle_system, PARTICLE_TYPE_MAGIC_GLOW, pos,
                      (Vec3){0.0f, 0.2f, 0.0f}, 0.15f, 12, 1.2f);
  particle_emit_burst(particle_system, PARTICLE_TYPE_BUBBLE, pos,
                      (Vec3){0.0f, 0.1f, 0.0f}, 0.12f, 8, 0.9f);

  // Play brewing complete sound if audio system is available and initialized
  if (audio_system && audio_system->initialized) {
    audio_play_sound_2d(audio_system, SOUND_CRAFTING_SUCCESS, 1.0f,
                        SOUND_CATEGORY_BLOCK);
  }
}

// Emit periodic progress particles (bubbles/steam)
void brewing_emit_progress_vfx(const BrewingStand *stand,
                               ParticleSystem *particle_system) {
  if (!stand || !particle_system)
    return;

  Vec3 pos = stand->position;
  // Emit a few steam/smoke particles upwards based on progress
  f32 intensity = stand->brew_progress; // 0.0 - 1.0
  u32 count = (u32)(1 + intensity * 4);
  particle_emit_burst(particle_system, PARTICLE_TYPE_SMOKE, pos,
                      (Vec3){0.0f, 0.08f, 0.0f}, 0.05f, count, 1.0f);
}
