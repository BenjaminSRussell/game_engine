#pragma once

#include "../Public/unified_input.h"

// Internal Action State
typedef struct {
  bool held;
  bool pressed;
  bool released;
  u32 last_pressed_frame;
  u32 last_released_frame;
  u32 hold_frames;
} ActionState;

// Internal Input Context
struct InputContext {
  // Frame Index for input timing
  u32 frame_index;

  // Actions
  ActionState actions[INPUT_ACTION_COUNT];

  // Bindings
  u32 key_bindings[INPUT_ACTION_COUNT];
  u32 mouse_bindings[INPUT_ACTION_COUNT];

  // Raw Keyboard State
  bool keys[INPUT_KEY_COUNT];
  bool keys_prev[INPUT_KEY_COUNT];

  // Raw Mouse State
  bool mouse_buttons[INPUT_MOUSE_BUTTON_COUNT];
  bool mouse_buttons_prev[INPUT_MOUSE_BUTTON_COUNT];
  f32 mouse_x, mouse_y;
  f32 mouse_dx, mouse_dy;
  f32 mouse_scroll;

  // Controller State (Simplified for now)
  struct {
    f32 axes[6]; // Sticks + Triggers
    u32 buttons;
  } gamepad[INPUT_GAMEPAD_COUNT];

  // Haptics settings
  bool haptics_enabled;
  f32 haptic_low;
  f32 haptic_high;
};
