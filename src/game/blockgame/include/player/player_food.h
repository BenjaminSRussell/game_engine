// include/player/player_food.h
//
// Purpose: Defines the food consumption system for players.

#ifndef PLAYER_FOOD_H
#define PLAYER_FOOD_H

#include "../game_common.h"
#include "../inventory/item_registry.h"

// Forward declaration to break circular dependency
struct PlayerComponent;

typedef struct {
  u32 total_items_eaten;
  f32 total_hunger_restored;
  f32 total_saturation_gained;
  u32 effects_applied;
} FoodStats;

// Eating state
typedef struct {
  bool is_eating;
  u32 food_item_id;
  f32 eat_progress; // 0.0 to 1.0
  f32 eat_duration; // Total time to eat
  f32 eat_timer;    // Current time
  u32 slot_index;   // Inventory slot being consumed from
} EatingState;

// Check if player can eat a food item
bool player_can_eat(struct PlayerComponent *player, u32 food_item_id,
                    const ItemRegistry *item_registry);

// Start eating food from inventory slot
bool player_start_eating(struct PlayerComponent *player, u32 slot_index,
                         const ItemRegistry *item_registry);

// Update eating progress (call every frame)
void player_update_eating(struct PlayerComponent *player, struct World *world,
                          f32 delta_time, const ItemRegistry *item_registry);

// Finish eating (applies hunger/saturation/effects)
void player_finish_eating(struct PlayerComponent *player,
                          const ItemRegistry *item_registry);

// Cancel eating in progress
void player_cancel_eating(struct PlayerComponent *player);

// Get current eating state
EatingState *player_get_eating_state(struct PlayerComponent *player);

// Update inventory spoilage (decay items)
void player_update_inventory_spoilage(struct PlayerComponent *player,
                                      f32 delta_time,
                                      const ItemRegistry *item_registry);

#endif // PLAYER_FOOD_H
