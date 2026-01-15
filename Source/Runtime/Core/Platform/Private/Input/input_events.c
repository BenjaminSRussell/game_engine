#include "input_types.h"

// Event Injection for Platform Backend
void input_inject_key(InputContext *ctx, u32 key, bool pressed) {
  if (!ctx || key >= INPUT_KEY_COUNT)
    return;
  ctx->keys[key] = pressed;
}

void input_inject_mouse_button(InputContext *ctx, u32 button, bool pressed) {
  if (!ctx || button >= INPUT_MOUSE_BUTTON_COUNT)
    return;
  ctx->mouse_buttons[button] = pressed;
}

void input_inject_mouse_move(InputContext *ctx, f32 x, f32 y) {
  if (!ctx)
    return;

  // Calculate delta
  ctx->mouse_dx = x - ctx->mouse_x;
  ctx->mouse_dy = y - ctx->mouse_y;

  // Update position
  ctx->mouse_x = x;
  ctx->mouse_y = y;
}

void input_inject_mouse_scroll(InputContext *ctx, f32 scroll) {
  if (!ctx)
    return;
  ctx->mouse_scroll = scroll;
}
