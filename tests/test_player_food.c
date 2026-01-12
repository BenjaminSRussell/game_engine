#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common.h>

#include "inventory/inventory.h"
#include "inventory/item_registry.h"
#include "player/player.h"
#include "player/player_food.h"

// Mock definitions
// Restore globals
void *g_audio_system = NULL;
void *g_particle_system = NULL;
// Mock dependencies
// Note: Declarations are provided by headers, we just implement them here.

void player_trigger_action_animation(Player *player, SpiritAnimState state,
                                     f32 duration) {}
void audio_play_sound_2d(void *system, int sound_id, f32 volume, int category) {
}
void particle_emit_burst(void *system, int type, Vec3 pos, Vec3 vel, f32 spread,
                         int count, f32 life) {}

// Correct signature for status_effect_add
bool status_effect_add(StatusEffectManager *manager, StatusEffectType type,
                       f32 duration, f32 magnitude) {
  return true;
}

// ECS mock
void *ecs_get_component(struct World *world, Entity entity, u32 component_id) {
  return NULL;
}

// Test helper: Setup player with food
void setup_test_player(Player *player, u32 food_id, u16 count, f32 quality) {
  memset(player, 0, sizeof(Player));
  inventory_init(&player->inventory);

  // Add food to slot 0
  inventory_set_slot(&player->inventory, 0, food_id, count);

  // Set quality if supported
  player->inventory.slots[0].quality_modifier = quality;

  // Set initial stats
  player->hunger = 10.0f; // Half hungry
  player->saturation = 0.0f;
}

// Test 1: Verify Food Spoilage
bool test_food_spoilage(ItemRegistry *registry) {
  printf("Running test_food_spoilage...\n");

  // Define a food that spoils in 10 seconds
  u32 food_id = 2001;
  ExtendedItemDefinition food_def = {0};
  // food_def.base.id = food_id; // id is not in ItemDefinition from inventory.h
  strncpy(food_def.base.name, "Test Apple", sizeof(food_def.base.name) - 1);
  food_def.base.max_stack_size = 64;
  food_def.base.item_type = ITEM_TYPE_FOOD;
  food_def.properties.food.spoil_time = 10.0f;
  food_def.properties.food.hunger_restored = 4.0f;
  item_registry_register(registry, food_id, &food_def);

  Player player;
  setup_test_player(&player, food_id, 5, 1.0f);

  // Simulate 5 seconds (50% spoiled)
  player_update_inventory_spoilage(&player, 5.0f, registry);

  if (fabs(player.inventory.slots[0].spoil_progress - 0.5f) > 0.01f) {
    printf("FAILED: Spoil progress expected 0.5, got %f\n",
           player.inventory.slots[0].spoil_progress);
    return false;
  }

  // Simulate another 6 seconds (Total 11s > 10s)
  player_update_inventory_spoilage(&player, 6.0f, registry);

  // Should be spoiled
  // Assuming ITEM_SPOILED_FOOD is 663
  if (player.inventory.slots[0].item_id != ITEM_SPOILED_FOOD) {
    printf("FAILED: Item ID expected %d (Spoiled), got %d\n", ITEM_SPOILED_FOOD,
           player.inventory.slots[0].item_id);
    return false;
  }

  printf("PASSED\n");
  return true;
}

// Test 2: Verify Quality Effects
bool test_food_quality(ItemRegistry *registry) {
  printf("Running test_food_quality...\n");

  u32 food_id = 2002;
  ExtendedItemDefinition food_def = {0};
  // food_def.base.id = food_id;
  strncpy(food_def.base.name, "Super Food", sizeof(food_def.base.name) - 1);
  food_def.base.max_stack_size = 64;
  food_def.base.item_type = ITEM_TYPE_FOOD;
  food_def.properties.food.hunger_restored = 8.0f;
  food_def.properties.food.saturation_modifier =
      0.5f; // Ends up as 4.0 saturation
  food_def.properties.food.eat_duration = 1.0f;
  food_def.properties.food.quality = 1.0f; // Base quality
  item_registry_register(registry, food_id, &food_def);

  Player player;
  // High quality food (1.5x)
  setup_test_player(&player, food_id, 1, 1.5f);

  // Start eating
  player.eating_state.is_eating = true;
  player.eating_state.food_item_id = food_id;
  player.eating_state.eat_duration = 1.0f;
  player.eating_state.slot_index = 0;

  // Finish eating directly
  player_finish_eating(&player, registry);

  // Expected:
  // Hunger restored = 8.0 * 1.5 = 12.0
  // Initial hunger = 10.0
  // Final hunger = 22.0 -> clamped to 20.0

  // Saturation gained = 8.0 * 0.5 * 1.5 = 6.0
  // Final saturation = 6.0

  if (fabs(player.hunger - 20.0f) > 0.01f) {
    printf("FAILED: Hunger expected 20.0, got %f\n", player.hunger);
    return false;
  }

  // Check actual amount restored (since clamped, hard to tell exact calc from
  // final value alone) Let's rely on saturation
  if (fabs(player.saturation - 6.0f) > 0.01f) {
    printf("FAILED: Saturation expected 6.0, got %f\n", player.saturation);
    return false;
  }

  // Verify stats
  if (fabs(player.food_stats.total_hunger_restored - 12.0f) > 0.01f) {
    printf("FAILED: Stat hunger restored expected 12.0, got %f\n",
           player.food_stats.total_hunger_restored);
    return false;
  }

  printf("PASSED\n");
  return true;
}

int main() {
  printf("=== Test Player Food ===\n");

  ItemRegistry registry = {0};
  item_registry_init(&registry);

  // Register Spoiled Food (663) - Required for spoilage logic
  ExtendedItemDefinition spoiled_def = {0};
  strncpy(spoiled_def.base.name, "Spoiled Food",
          sizeof(spoiled_def.base.name) - 1);
  spoiled_def.base.max_stack_size = 64;
  spoiled_def.base.item_type =
      ITEM_TYPE_FOOD; // Or MISC, but FOOD ensures it fits logic
  item_registry_register(&registry, 663, &spoiled_def);

  bool all_passed = true;
  all_passed &= test_food_spoilage(&registry);
  all_passed &= test_food_quality(&registry);

  // Cleanup
  // item_registry_free(&registry); // not implemented in mock usually

  if (all_passed) {
    printf("All tests passed!\n");
    return 0;
  } else {
    printf("Some tests failed.\n");
    return 1;
  }
}
