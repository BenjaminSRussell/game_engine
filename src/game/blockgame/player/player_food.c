// src/player/player_food.c
//
// Purpose: Implementation of the food consumption system for players.
// TODO: Implement food quality system affecting hunger restoration.
// TODO: Add food cooking system for better food items.
// TODO: Implement food spoilage system for realism.
// TODO: Add food recipe system for custom foods.
// TODO: Implement food sound effects system.
// TODO: Add food particle effects system.
// TODO: Implement food statistics tracking system.
// TODO: Add food tooltip system with nutrition info.
//
#include <audio/audio_system.h>
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <effects/vfx/particle_system.h>
#include <inventory/inventory.h>
#include <player/player.h>
#include <player/player_food.h>
#include <player/status_effects.h>
#include <stdlib.h>
#include <string.h>

extern AudioSystem *g_audio_system;
extern ParticleSystem *g_particle_system;

// Check if player can eat
bool player_can_eat(Player *player, u32 food_item_id,
                    const ItemRegistry *item_registry) {
  if (!player || !item_registry)
    return false;

  // Get item definition
  const ExtendedItemDefinition *item =
      item_registry_get(item_registry, food_item_id);
  if (!item || !item_is_food(item)) {
    return false;
  }

  // Can always eat if not at full hunger
  if (player->hunger < 20.0f) {
    return true;
  }

  // Golden apples and special foods can be eaten even when full
  if (item->properties.food.has_effects) {
    return true;
  }

  return false;
}

// Start eating food
bool player_start_eating(Player *player, u32 slot_index,
                         const ItemRegistry *item_registry) {
  if (!player || !item_registry)
    return false;

  // Can't eat if already eating
  if (player->eating_state.is_eating) {
    return false;
  }

  // Get item from slot
  InventorySlot slot;
  if (!inventory_get_slot(&player->inventory, slot_index, &slot)) {
    return false;
  }

  if (slot.count == 0) {
    return false;
  }

  // Check if it's food
  const ExtendedItemDefinition *item =
      item_registry_get(item_registry, slot.item_id);
  if (!item || !item_is_food(item)) {
    return false;
  }

  // Check if player can eat it
  if (!player_can_eat(player, slot.item_id, item_registry)) {
    return false;
  }

  // Start eating
  player->eating_state.is_eating = true;
  player->eating_state.food_item_id = slot.item_id;
  player->eating_state.eat_progress = 0.0f;
  player->eating_state.eat_duration = item->properties.food.eat_duration;
  player->eating_state.eat_timer = 0.0f;
  player->eating_state.slot_index = slot_index;
  player_trigger_action_animation(player, SPIRIT_ANIM_INTERACT,
                                  player->eating_state.eat_duration);

  LOG_DEBUG("Player started eating %s (duration: %.2fs)", item->base.name,
            item->properties.food.eat_duration);

  if (g_audio_system) {
    audio_play_sound_2d(g_audio_system, SOUND_ITEM_PICKUP, 1.0f,
                        SOUND_CATEGORY_PLAYER);
  }

  return true;
}

// Update eating progress
void player_update_eating(Player *player, struct World *world, f32 delta_time,
                          const ItemRegistry *item_registry) {
  if (!player || !player->eating_state.is_eating)
    return;

  // Update timer
  player->eating_state.eat_timer += delta_time;
  player->eating_state.eat_progress =
      player->eating_state.eat_timer / player->eating_state.eat_duration;

  // Emit particles
  if (g_particle_system && world) {
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)world, (Entity){player->entity_id, 0}, TRANSFORM_COMPONENT_ID);
    if (transform) {
      Vec3 pos = transform->position;
      pos.y += 1.5f; // Mouth height
      particle_emit_burst(g_particle_system, PARTICLE_TYPE_DEBRIS, pos,
                          vec3_zero(), 0.5f, 2, 0.5f);
    }
  }

  // Check if eating is complete
  if (player->eating_state.eat_progress >= 1.0f) {
    player_finish_eating(player, item_registry);
  }

  // Cancel eating if player is moving too fast (sprinting/flying)
  if (player->is_sprinting || player->is_flying) {
    player_cancel_eating(player);
    LOG_DEBUG("Eating cancelled - player is moving too fast");
  }
}

// Finish eating
void player_finish_eating(Player *player, const ItemRegistry *item_registry) {
  if (!player || !player->eating_state.is_eating)
    return;

  // Get food item
  const ExtendedItemDefinition *item =
      item_registry_get(item_registry, player->eating_state.food_item_id);
  if (!item || !item_is_food(item)) {
    player_cancel_eating(player);
    return;
  }

  const FoodProperties *food = &item->properties.food;

  // Restore hunger
  player->hunger += food->hunger_restored * food->quality;
  if (player->hunger > 20.0f) {
    player->hunger = 20.0f;
  }

  // Restore saturation
  f32 saturation_gained =
      food->hunger_restored * food->saturation_modifier * food->quality;
  player->saturation += saturation_gained;
  if (player->saturation > player->hunger) {
    player->saturation = player->hunger;
  }

  LOG_INFO("Player ate %s: +%.1f hunger, +%.1f saturation", item->base.name,
           food->hunger_restored * food->quality, saturation_gained);

  // Update food stats
  player->food_stats.total_items_eaten++;
  player->food_stats.total_hunger_restored +=
      food->hunger_restored * food->quality;
  player->food_stats.total_saturation_gained += saturation_gained;

  // Apply status effects if applicable
  if (food->has_effects && food->effect_chance > 0.0f) {
    // Roll for effect chance
    f32 roll = (f32)rand() / (f32)RAND_MAX;
    if (roll <= food->effect_chance) {
      player->food_stats.effects_applied++;
      // Map effect_id to status effect type
      // 0 = Poison, 1 = Regeneration
      if (food->effect_id == 0) {
        status_effect_add(&player->status_effects, STATUS_EFFECT_POISON, 30.0f,
                          1.0f);
        LOG_INFO("Food effect applied: Poison");
      } else if (food->effect_id == 1) {
        status_effect_add(&player->status_effects, STATUS_EFFECT_REGENERATION,
                          5.0f, 2.0f);
        LOG_INFO("Food effect applied: Regeneration II");
      }
    }
  }

  if (g_audio_system) {
    audio_play_sound_2d(g_audio_system, SOUND_PLAYER_HEAL, 1.0f,
                        SOUND_CATEGORY_PLAYER);
  }

  // Remove item from inventory
  inventory_remove_item(&player->inventory, player->eating_state.food_item_id,
                        1);

  // Clear eating state
  memset(&player->eating_state, 0, sizeof(EatingState));
  player->action_anim_timer = 0.0f;
}

// Cancel eating
void player_cancel_eating(Player *player) {
  if (!player)
    return;

  if (player->eating_state.is_eating) {
    LOG_DEBUG("Eating cancelled");
    memset(&player->eating_state, 0, sizeof(EatingState));
    player->action_anim_timer = 0.0f;
  }
}

// Get eating state
EatingState *player_get_eating_state(Player *player) {
  if (!player)
    return NULL;
  return &player->eating_state;
}