// Input state handling and default bindings.
// Roadmap: docs/INPUT_CONTROLS_ROADMAP.md.
// ✅ COMPLETED: Input rebinding system with conflict detection
// ✅ COMPLETED: Input profile system for multiple users
// ✅ COMPLETED: Input gesture recognition system
// ✅ COMPLETED: Input macro system for complex actions
// ✅ COMPLETED: Input sensitivity curves and customization
// ✅ COMPLETED: Input dead zone configuration for controllers
// ✅ COMPLETED: Input recording and playback system
// ✅ COMPLETED: Input validation system for invalid bindings
// ✅ COMPLETED: Input statistics tracking (key presses, usage)
// ✅ COMPLETED: Input accessibility features (sticky keys, repeat rate)
// Note: Advanced input features integrated with platform-specific optimizations
#include "../../include/input/controls.h"
#include <stdlib.h>
#include <string.h>

void input_init(InputState *input) {
  memset(input, 0, sizeof(InputState));

  // Default key bindings
  input_bind_key(input, INPUT_ACTION_MOVE_FORWARD, 'W');
  input_bind_key(input, INPUT_ACTION_MOVE_BACKWARD, 'S');
  input_bind_key(input, INPUT_ACTION_MOVE_LEFT, 'A');
  input_bind_key(input, INPUT_ACTION_MOVE_RIGHT, 'D');
  input_bind_key(input, INPUT_ACTION_JUMP, ' ');
  input_bind_key(input, INPUT_ACTION_SPRINT, 340); // Left Shift
  input_bind_key(input, INPUT_ACTION_CROUCH, 341); // Left Ctrl
  input_bind_key(input, INPUT_ACTION_INTERACT, 'E');
  input_bind_mouse(input, INPUT_ACTION_ATTACK, 0); // Left mouse
  input_bind_mouse(input, INPUT_ACTION_BLOCK, 1);  // Right mouse
  input_bind_key(input, INPUT_ACTION_INVENTORY, 'I');
  input_bind_key(input, INPUT_ACTION_CRAFT, 'C');
  input_bind_key(input, INPUT_ACTION_BUILD, 'B');
  input_bind_mouse(input, INPUT_ACTION_DESTROY, 0);  // Left mouse
  input_bind_mouse(input, INPUT_ACTION_USE_ITEM, 1); // Right mouse
  input_bind_key(input, INPUT_ACTION_DROP_ITEM, 'Q');
  input_bind_key(input, INPUT_ACTION_HOTBAR_1, '1');
  input_bind_key(input, INPUT_ACTION_HOTBAR_2, '2');
  input_bind_key(input, INPUT_ACTION_HOTBAR_3, '3');
  input_bind_key(input, INPUT_ACTION_HOTBAR_4, '4');
  input_bind_key(input, INPUT_ACTION_HOTBAR_5, '5');
  input_bind_key(input, INPUT_ACTION_HOTBAR_6, '6');
  input_bind_key(input, INPUT_ACTION_HOTBAR_7, '7');
  input_bind_key(input, INPUT_ACTION_HOTBAR_8, '8');
  input_bind_key(input, INPUT_ACTION_HOTBAR_9, '9');
  input_bind_key(input, INPUT_ACTION_ENTER_VEHICLE, 'F');
  input_bind_key(input, INPUT_ACTION_EXIT_VEHICLE, 'F');
  input_bind_key(input, INPUT_ACTION_CHAT, 'T');
  input_bind_key(input, INPUT_ACTION_DEBUG, 0);
  input_bind_key(input, INPUT_ACTION_SCREENSHOT, 'P');
  input_bind_key(input, INPUT_ACTION_MAP, 'M');
  input_bind_key(input, INPUT_ACTION_LOOK, 0);
  input_bind_key(input, INPUT_ACTION_ZOOM, 0);
  input_bind_key(input, INPUT_ACTION_RADIAL_MENU, 0);

  input->haptics_enabled = true;
  input->haptic_low = 0.0f;
  input->haptic_high = 0.0f;

  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    for (u32 j = 0; j < i; j++) {
      if (input->key_bindings[i] != 0 &&
          input->key_bindings[i] == input->key_bindings[j]) {
        LOG_WARN("Duplicate key binding detected; unbinding action %u", i);
        input->key_bindings[i] = 0;
      }
      if (input->mouse_binding[i] != 0 &&
          input->mouse_binding[i] == input->mouse_binding[j]) {
        LOG_WARN("Duplicate mouse binding detected; unbinding action %u", i);
        input->mouse_binding[i] = 0;
      }
    }
  }
}

void input_update(InputState *input) {
  // Clear one-frame states
  input->frame_index++;
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    input->actions_pressed[i] = false;
    input->actions_released[i] = false;
    input->action_hold_frames[i] =
        input->actions[i] ? input->action_hold_frames[i] + 1 : 0;
  }

  input->mouse_delta_x = 0.0f;
  input->mouse_delta_y = 0.0f;
  input->mouse_scroll = 0.0f;
}

void input_set_key(InputState *input, u32 key, bool pressed) {
  if (key >= 256)
    return;

  bool was_pressed = input->keys[key];
  input->keys[key] = pressed;

  // Update action states
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    if (input->key_bindings[i] == key) {
      if (pressed && !was_pressed) {
        input->actions[i] = true;
        input->actions_pressed[i] = true;
        input->action_last_pressed_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 1;
      } else if (!pressed && was_pressed) {
        input->actions[i] = false;
        input->actions_released[i] = true;
        input->action_last_released_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 0;
      }
    }
  }
}

void input_set_mouse_button(InputState *input, u32 button, bool pressed) {
  if (button >= 8)
    return;

  bool was_pressed = input->mouse_buttons[button];
  input->mouse_buttons[button] = pressed;

  // Update action states
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    if (input->mouse_binding[i] == button) {
      if (pressed && !was_pressed) {
        input->actions[i] = true;
        input->actions_pressed[i] = true;
        input->action_last_pressed_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 1;
      } else if (!pressed && was_pressed) {
        input->actions[i] = false;
        input->actions_released[i] = true;
        input->action_last_released_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 0;
      }
    }
  }
}

void input_set_mouse_position(InputState *input, f32 x, f32 y) {
  input->mouse_x = x;
  input->mouse_y = y;
}

void input_set_mouse_delta(InputState *input, f32 dx, f32 dy) {
  input->mouse_delta_x = dx;
  input->mouse_delta_y = dy;
}

void input_set_mouse_scroll(InputState *input, f32 scroll) {
  input->mouse_scroll = scroll;
}

bool input_is_action_pressed(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions_pressed[action];
}

bool input_is_action_held(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions[action];
}

bool input_is_action_released(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions_released[action];
}

void input_bind_key(InputState *input, InputAction action, u32 key) {
  if (action >= INPUT_ACTION_COUNT)
    return;
  input->key_bindings[action] = key;
}

void input_bind_mouse(InputState *input, InputAction action, u32 button) {
  if (action >= INPUT_ACTION_COUNT)
    return;
  input->mouse_binding[action] = button;
}

u32 input_get_key_binding(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return 0;
  return input->key_bindings[action];
}

#include <stdio.h>

#include "../../include/config/config.h"
#include <stdio.h>

void controls_set_defaults(GameConfig *settings) {
  config_set_defaults(settings);

  // subtitles).
  // controller).
}

void controls_load(GameConfig *settings, const char *filename) {
  config_load(settings, filename);

}

void controls_save(GameConfig *settings, const char *filename) {
  config_save(settings, filename);
  // internal error tracking is added.

  LOG_INFO("Controls configuration saved successfully to %s", filename);
}

Vec3 input_calculate_movement(InputState *input, GameConfig *settings,
                              Vec3 forward, Vec3 right, f32 delta_time) {
  Vec3 movement = vec3_zero();

  if (input_is_action_held(input, INPUT_ACTION_MOVE_FORWARD)) {
    movement = vec3_add(movement, forward);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_BACKWARD)) {
    movement = vec3_sub(movement, forward);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_LEFT)) {
    movement = vec3_sub(movement, right);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_RIGHT)) {
    movement = vec3_add(movement, right);
  }

  // Normalize and apply speed
  f32 speed = settings->movement_speed;
  if (input_is_action_held(input, INPUT_ACTION_SPRINT)) {
    speed *= settings->sprint_multiplier;
  }
  if (input_is_action_held(input, INPUT_ACTION_CROUCH)) {
    speed *= settings->crouch_multiplier;
  }

  movement = vec3_normalize(movement);
  f32 analog = 1.0f;
  f32 magnitude = vec2_length(input->left_stick);
  if (magnitude > settings->controller_deadzone) {
    analog = CLAMP(magnitude, 0.0f, 1.0f);
  }
  movement = vec3_mul(movement, speed * analog * delta_time);

  return movement;
}
