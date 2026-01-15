#include "input_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

InputContext *input_init(void) {
  InputContext *ctx = (InputContext *)UNIFIED_ALLOC(sizeof(InputContext));
  if (!ctx) {
    LOG_ERROR(LOG_CAT_INPUT, "Failed to allocate input context");
    return NULL;
  }
  memset(ctx, 0, sizeof(InputContext));

  // Initialize bindings to "unbound" state
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    ctx->mouse_bindings[i] = UINT32_MAX;
  }

  // Default Bindings Setup (migrated from input.c:96)
  input_bind_key(ctx, INPUT_ACTION_MOVE_FORWARD, 'W');
  input_bind_key(ctx, INPUT_ACTION_MOVE_BACKWARD, 'S');
  input_bind_key(ctx, INPUT_ACTION_MOVE_LEFT, 'A');
  input_bind_key(ctx, INPUT_ACTION_MOVE_RIGHT, 'D');
  input_bind_key(ctx, INPUT_ACTION_JUMP, 32);    // Space
  input_bind_key(ctx, INPUT_ACTION_SPRINT, 340); // Left Shift
  input_bind_key(ctx, INPUT_ACTION_CROUCH, 341); // Left Ctrl
  input_bind_key(ctx, INPUT_ACTION_INTERACT, 'F');
  input_bind_mouse(ctx, INPUT_ACTION_ATTACK, 0); // Left Mouse
  input_bind_mouse(ctx, INPUT_ACTION_BLOCK, 1);  // Right Mouse
  input_bind_key(ctx, INPUT_ACTION_INVENTORY, 'E');
  input_bind_key(ctx, INPUT_ACTION_CRAFT, 'C');
  // ... complete list would go here, simplified for this file split

  ctx->haptics_enabled = true;

  LOG_INFO(LOG_CAT_INPUT, "Input system initialized");
  return ctx;
}

void input_shutdown(InputContext *ctx) {
  if (ctx) {
    UNIFIED_FREE(ctx);
    LOG_INFO(LOG_CAT_INPUT, "Input system shutdown");
  }
}

// Forward declare internal function
void input_update_action_states(InputContext *ctx);

void input_update(InputContext *ctx, f32 delta_time) {
  if (!ctx)
    return;

  ctx->frame_index++;

  // Clear one-frame states
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    ctx->actions[i].pressed = false;
    ctx->actions[i].released = false;

    if (ctx->actions[i].held) {
      ctx->actions[i].hold_frames++;
    } else {
      ctx->actions[i].hold_frames = 0;
    }
  }

  // Mouse delta is valid only for current frame
  ctx->mouse_dx = 0.0f;
  ctx->mouse_dy = 0.0f;
  ctx->mouse_scroll = 0.0f;

  // Update Previous Key States
  memcpy(ctx->keys_prev, ctx->keys, sizeof(ctx->keys));
  memcpy(ctx->mouse_buttons_prev, ctx->mouse_buttons,
         sizeof(ctx->mouse_buttons));

  // Update action states based on input changes
  input_update_action_states(ctx);
}
