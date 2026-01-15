#pragma once

#include <stdbool.h>
#include <stdint.h>

// Basic type definitions
#ifndef u32
typedef uint32_t u32;
typedef float f32;
#endif

// Input Constants
#define INPUT_KEY_COUNT 512
#define INPUT_MOUSE_BUTTON_COUNT 8
#define INPUT_GAMEPAD_COUNT 4
#define INPUT_MAX_TOUCHES 10

// Action Enums (Migrated from legacy)
typedef enum {
  INPUT_ACTION_MOVE_FORWARD,
  INPUT_ACTION_MOVE_BACKWARD,
  INPUT_ACTION_MOVE_LEFT,
  INPUT_ACTION_MOVE_RIGHT,
  INPUT_ACTION_JUMP,
  INPUT_ACTION_SPRINT,
  INPUT_ACTION_CROUCH,
  INPUT_ACTION_INTERACT,
  INPUT_ACTION_ATTACK,
  INPUT_ACTION_BLOCK,
  INPUT_ACTION_INVENTORY,
  INPUT_ACTION_CRAFT,
  INPUT_ACTION_BUILD,
  INPUT_ACTION_DESTROY,
  INPUT_ACTION_USE_ITEM,
  INPUT_ACTION_DROP_ITEM,
  // Hotbar
  INPUT_ACTION_HOTBAR_1,
  INPUT_ACTION_HOTBAR_2,
  INPUT_ACTION_HOTBAR_3,
  INPUT_ACTION_HOTBAR_4,
  INPUT_ACTION_HOTBAR_5,
  INPUT_ACTION_HOTBAR_6,
  INPUT_ACTION_HOTBAR_7,
  INPUT_ACTION_HOTBAR_8,
  INPUT_ACTION_HOTBAR_9,
  INPUT_ACTION_HOTBAR_PREV,
  INPUT_ACTION_HOTBAR_NEXT,
  // UI/Misc
  INPUT_ACTION_MENU,
  INPUT_ACTION_ENTER_VEHICLE,
  INPUT_ACTION_EXIT_VEHICLE,
  INPUT_ACTION_CHAT,
  INPUT_ACTION_DEBUG,
  INPUT_ACTION_SCREENSHOT,
  INPUT_ACTION_MAP,
  INPUT_ACTION_LOOK,
  INPUT_ACTION_ZOOM,
  INPUT_ACTION_RADIAL_MENU,
  INPUT_ACTION_COUNT
} InputAction;

// Opaque Handle
typedef struct InputContext InputContext;

// Initialization
InputContext *input_init(void);
void input_shutdown(InputContext *ctx);
void input_update(InputContext *ctx, f32 delta_time);

// Action Queries
bool input_is_action_pressed(InputContext *ctx, InputAction action);
bool input_is_action_held(InputContext *ctx, InputAction action);
bool input_is_action_released(InputContext *ctx, InputAction action);

// Raw Input Queries
bool input_is_key_down(InputContext *ctx, u32 key_code);
bool input_is_mouse_button_down(InputContext *ctx, u32 button);
void input_get_mouse_position(InputContext *ctx, f32 *x, f32 *y);
void input_get_mouse_delta(InputContext *ctx, f32 *dx, f32 *dy);
f32 input_get_mouse_scroll(InputContext *ctx);

// Binding Management
void input_bind_key(InputContext *ctx, InputAction action, u32 key);
void input_bind_mouse(InputContext *ctx, InputAction action, u32 button);
const char *input_action_get_name(InputAction action);

// Event Injection (for Platform Backend)
void input_inject_key(InputContext *ctx, u32 key, bool pressed);
void input_inject_mouse_button(InputContext *ctx, u32 button, bool pressed);
void input_inject_mouse_move(InputContext *ctx, f32 x, f32 y);
void input_inject_mouse_scroll(InputContext *ctx, f32 scroll);
