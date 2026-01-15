#include "input_types.h"

// Binding Management
void input_bind_key(InputContext *ctx, InputAction action, u32 key) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return;
  ctx->key_bindings[action] = key;
}

void input_bind_mouse(InputContext *ctx, InputAction action, u32 button) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return;
  ctx->mouse_bindings[action] = button;
}

// Helper: Check if action is triggered by current input state
static bool check_action_held(InputContext *ctx, InputAction action) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return false;

  // Check keyboard binding
  u32 key = ctx->key_bindings[action];
  if (key != 0 && key < INPUT_KEY_COUNT && ctx->keys[key]) {
    return true;
  }

  // Check mouse binding
  u32 button = ctx->mouse_bindings[action];
  if (button != UINT32_MAX && button < INPUT_MOUSE_BUTTON_COUNT &&
      ctx->mouse_buttons[button]) {
    return true;
  }

  return false;
}

// Update action state based on input changes
void input_update_action_states(InputContext *ctx) {
  if (!ctx)
    return;

  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    bool currently_held = check_action_held(ctx, (InputAction)i);
    bool was_held = ctx->actions[i].held;

    ctx->actions[i].held = currently_held;

    if (currently_held && !was_held) {
      // Just pressed
      ctx->actions[i].pressed = true;
      ctx->actions[i].last_pressed_frame = ctx->frame_index;
      ctx->actions[i].hold_frames = 1;
    } else if (!currently_held && was_held) {
      // Just released
      ctx->actions[i].released = true;
      ctx->actions[i].last_released_frame = ctx->frame_index;
      ctx->actions[i].hold_frames = 0;
    }
  }
}
