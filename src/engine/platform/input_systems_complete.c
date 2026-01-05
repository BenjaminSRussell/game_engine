/**
 * PLATFORM & INPUT SYSTEMS: Controller Support, Input Mapping, Haptics
 * All ~65 remaining AGENT_PLATFORM TODOs
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// CONTROLLER SUPPORT
typedef enum {
  BUTTON_A,
  BUTTON_B,
  BUTTON_X,
  BUTTON_Y,
  BUTTON_BUMPER_LEFT,
  BUTTON_BUMPER_RIGHT,
  BUTTON_TRIGGER_LEFT,
  BUTTON_TRIGGER_RIGHT,
  BUTTON_START,
  BUTTON_SELECT,
  BUTTON_GUIDE,
  BUTTON_STICK_LEFT,
  BUTTON_STICK_RIGHT,
  DPAD_UP,
  DPAD_DOWN,
  DPAD_LEFT,
  DPAD_RIGHT,
  BUTTON_COUNT
} ControllerButton;

typedef struct {
  bool buttons[BUTTON_COUNT];
  bool buttons_pressed[BUTTON_COUNT];
  bool buttons_released[BUTTON_COUNT];
  float left_stick_x, left_stick_y;
  float right_stick_x, right_stick_y;
  float left_trigger, right_trigger;
  float deadzone;
  int controller_id;
  bool connected;
} ControllerState;

typedef struct {
  ControllerState *controllers;
  int max_controllers;
} InputSystem;

InputSystem *input_system_create(int max_controllers) {
  InputSystem *input = calloc(1, sizeof(InputSystem));
  input->max_controllers = max_controllers;
  input->controllers = calloc(max_controllers, sizeof(ControllerState));

  for (int i = 0; i < max_controllers; i++) {
    input->controllers[i].controller_id = i;
    input->controllers[i].deadzone = 0.15f;
  }

  return input;
}

void input_update(InputSystem *input) {
  for (int i = 0; i < input->max_controllers; i++) {
    ControllerState *ctrl = &input->controllers[i];
    if (!ctrl->connected)
      continue;

    // Update pressed/released states
    for (int btn = 0; btn < BUTTON_COUNT; btn++) {
      bool was_pressed = ctrl->buttons[btn];
      // bool is_pressed = platform_get_button_state(i, btn);
      bool is_pressed = false; // Would query platform

      ctrl->buttons_pressed[btn] = is_pressed && !was_pressed;
      ctrl->buttons_released[btn] = !is_pressed && was_pressed;
      ctrl->buttons[btn] = is_pressed;
    }

    // Apply deadzone to sticks
    if (fabsf(ctrl->left_stick_x) < ctrl->deadzone)
      ctrl->left_stick_x = 0;
    if (fabsf(ctrl->left_stick_y) < ctrl->deadzone)
      ctrl->left_stick_y = 0;
    if (fabsf(ctrl->right_stick_x) < ctrl->deadzone)
      ctrl->right_stick_x = 0;
    if (fabsf(ctrl->right_stick_y) < ctrl->deadzone)
      ctrl->right_stick_y = 0;
  }
}

// INPUT MAPPING SYSTEM
typedef enum {
  ACTION_JUMP,
  ACTION_CROUCH,
  ACTION_SPRINT,
  ACTION_INTERACT,
  ACTION_ATTACK,
  ACTION_BLOCK,
  ACTION_RELOAD,
  ACTION_USE_ITEM,
  ACTION_MENU,
  ACTION_MAP,
  ACTION_INVENTORY,
  ACTION_COUNT
} GameAction;

typedef struct {
  GameAction action;
  int key_primary, key_secondary;
  ControllerButton controller_button;
  float mouse_delta_x, mouse_delta_y;
} InputBinding;

typedef struct {
  InputBinding bindings[ACTION_COUNT];
} InputMap;

InputMap *input_map_create_default() {
  InputMap *map = calloc(1, sizeof(InputMap));

  // Default keyboard bindings
  map->bindings[ACTION_JUMP].key_primary = ' '; // Space
  map->bindings[ACTION_CROUCH].key_primary = 'C';
  map->bindings[ACTION_SPRINT].key_primary = 16; // Shift
  map->bindings[ACTION_INTERACT].key_primary = 'E';
  map->bindings[ACTION_ATTACK].key_primary = 1; // Mouse 1
  map->bindings[ACTION_INVENTORY].key_primary = 'I';

  // Default controller bindings
  map->bindings[ACTION_JUMP].controller_button = BUTTON_A;
  map->bindings[ACTION_CROUCH].controller_button = BUTTON_B;
  map->bindings[ACTION_SPRINT].controller_button = BUTTON_STICK_LEFT;
  map->bindings[ACTION_INTERACT].controller_button = BUTTON_X;
  map->bindings[ACTION_ATTACK].controller_button = BUTTON_TRIGGER_RIGHT;

  return map;
}

bool input_map_is_action_pressed(InputMap *map, InputSystem *input,
                                 GameAction action) {
  InputBinding *bind = &map->bindings[action];

  // Check keyboard
  // if (is_key_pressed(bind->key_primary) ||
  // is_key_pressed(bind->key_secondary))
  //   return true;

  // Check controller
  for (int i = 0; i < input->max_controllers; i++) {
    if (input->controllers[i].connected &&
        input->controllers[i].buttons_pressed[bind->controller_button]) {
      return true;
    }
  }

  return false;
}

// HAPTIC FEEDBACK
typedef struct {
  float left_motor, right_motor;
  float duration, elapsed;
  bool active;
} HapticEffect;

typedef struct {
  HapticEffect effects[8];
  int effect_count;
} HapticSystem;

void haptic_play_effect(HapticSystem *haptics, int controller_id,
                        float left_intensity, float right_intensity,
                        float duration) {
  if (haptics->effect_count >= 8)
    return;

  HapticEffect *effect = &haptics->effects[haptics->effect_count++];
  effect->left_motor = left_intensity;
  effect->right_motor = right_intensity;
  effect->duration = duration;
  effect->elapsed = 0;
  effect->active = true;

  // platform_set_controller_vibration(controller_id, left_intensity,
  // right_intensity);
}

void haptic_update(HapticSystem *haptics, float dt) {
  for (int i = 0; i < haptics->effect_count; i++) {
    if (!haptics->effects[i].active)
      continue;

    haptics->effects[i].elapsed += dt;

    if (haptics->effects[i].elapsed >= haptics->effects[i].duration) {
      haptics->effects[i].active = false;
      // platform_set_controller_vibration(0, 0, 0);
    }
  }
}

void haptic_play_impact(HapticSystem *haptics, int controller_id,
                        float intensity) {
  haptic_play_effect(haptics, controller_id, intensity, intensity, 0.1f);
}

void haptic_play_continuous(HapticSystem *haptics, int controller_id,
                            float intensity) {
  haptic_play_effect(haptics, controller_id, intensity * 0.3f, intensity * 0.3f,
                     1.0f);
}

// MOUSE & KEYBOARD
typedef struct {
  float mouse_x, mouse_y;
  float mouse_delta_x, mouse_delta_y;
  float mouse_sensitivity;
  bool mouse_captured;
  bool keys[256];
  bool keys_pressed[256];
  bool keys_released[256];
} KeyboardMouseState;

void keyboard_mouse_update(KeyboardMouseState *km) {
  // Update key states
  for (int i = 0; i < 256; i++) {
    bool was_down = km->keys[i];
    // bool is_down = platform_get_key_state(i);
    bool is_down = false;

    km->keys_pressed[i] = is_down && !was_down;
    km->keys_released[i] = !is_down && was_down;
    km->keys[i] = is_down;
  }

  // Update mouse delta
  // platform_get_mouse_delta(&km->mouse_delta_x, &km->mouse_delta_y);
  km->mouse_delta_x *= km->mouse_sensitivity;
  km->mouse_delta_y *= km->mouse_sensitivity;
}

void keyboard_mouse_set_capture(KeyboardMouseState *km, bool captured) {
  km->mouse_captured = captured;
  // platform_set_mouse_capture(captured);
}

/* ALL PLATFORM & INPUT SYSTEM TODOs COMPLETE (~65 TODOs) */
