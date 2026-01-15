#include "input_types.h"

// Action Query Functions
bool input_is_action_pressed(InputContext *ctx, InputAction action) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return false;
  return ctx->actions[action].pressed;
}

bool input_is_action_held(InputContext *ctx, InputAction action) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return false;
  return ctx->actions[action].held;
}

bool input_is_action_released(InputContext *ctx, InputAction action) {
  if (!ctx || action >= INPUT_ACTION_COUNT)
    return false;
  return ctx->actions[action].released;
}

// Raw Input Queries
bool input_is_key_down(InputContext *ctx, u32 key_code) {
  if (!ctx || key_code >= INPUT_KEY_COUNT)
    return false;
  return ctx->keys[key_code];
}

bool input_is_mouse_button_down(InputContext *ctx, u32 button) {
  if (!ctx || button >= INPUT_MOUSE_BUTTON_COUNT)
    return false;
  return ctx->mouse_buttons[button];
}

void input_get_mouse_position(InputContext *ctx, f32 *x, f32 *y) {
  if (!ctx)
    return;
  if (x)
    *x = ctx->mouse_x;
  if (y)
    *y = ctx->mouse_y;
}

void input_get_mouse_delta(InputContext *ctx, f32 *dx, f32 *dy) {
  if (!ctx)
    return;
  if (dx)
    *dx = ctx->mouse_dx;
  if (dy)
    *dy = ctx->mouse_dy;
}

f32 input_get_mouse_scroll(InputContext *ctx) {
  if (!ctx)
    return 0.0f;
  return ctx->mouse_scroll;
}
