#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct Character Character;
typedef struct Inventory Inventory;
typedef struct CharacterConfig CharacterConfig;
typedef struct ItemStack ItemStack;

// Character state
typedef enum {
  CHARACTER_STATE_IDLE,
  CHARACTER_STATE_WALKING,
  CHARACTER_STATE_RUNNING,
  CHARACTER_STATE_JUMPING,
  CHARACTER_STATE_FALLING,
  CHARACTER_STATE_DEAD
} CharacterState;

// Character configuration
struct CharacterConfig {
  float max_health;
  float move_speed;
  float run_speed;
  float jump_height;
};

// Item system
typedef uint32_t ItemID;

struct ItemStack {
  ItemID item_id;
  uint32_t count;
};

// Character management
Character *character_create(const CharacterConfig *config);
void character_update(Character *character, float delta_time);
void character_destroy(Character *character);

// Movement
void character_move(Character *character, float forward, float right);
void character_jump(Character *character);
void character_set_position(Character *character, float x, float y, float z);
void character_get_position(Character *character, float *out_x, float *out_y,
                            float *out_z);

// State
CharacterState character_get_state(Character *character);
float character_get_health(Character *character);
void character_take_damage(Character *character, float damage);
void character_heal(Character *character, float amount);

// Inventory management
Inventory *inventory_create(uint32_t slot_count);
void inventory_destroy(Inventory *inventory);

bool inventory_add_item(Inventory *inventory, ItemID item, uint32_t count);
bool inventory_remove_item(Inventory *inventory, ItemID item, uint32_t count);
uint32_t inventory_get_item_count(Inventory *inventory, ItemID item);
uint32_t inventory_get_slot_count(Inventory *inventory);
ItemStack *inventory_get_slot(Inventory *inventory, uint32_t slot_index);
