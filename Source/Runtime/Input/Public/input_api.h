/**
 * @file input_api.h
 * @brief Input subsystem API
 */

#ifndef VOXELFORGE_INPUT_API_H
#define VOXELFORGE_INPUT_API_H

#include "Core/Public/Math/Vector/vec2.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Key Codes
// ============================================================================

typedef enum KeyCode {
  KEY_UNKNOWN = 0,

  // Letters
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,

  // Numbers
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,

  // Function keys
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,

  // Special keys
  KEY_ESCAPE,
  KEY_TAB,
  KEY_CAPS_LOCK,
  KEY_SHIFT,
  KEY_CTRL,
  KEY_ALT,
  KEY_SPACE,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_DELETE,
  KEY_INSERT,
  KEY_HOME,
  KEY_END,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_UP,
  KEY_DOWN,
  KEY_PRINT_SCREEN,
  KEY_SCROLL_LOCK,
  KEY_PAUSE,

  // Punctuation
  KEY_MINUS,
  KEY_EQUALS,
  KEY_LEFT_BRACKET,
  KEY_RIGHT_BRACKET,
  KEY_BACKSLASH,
  KEY_SEMICOLON,
  KEY_APOSTROPHE,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_GRAVE,

  // Numpad
  KEY_NUMPAD_0,
  KEY_NUMPAD_1,
  KEY_NUMPAD_2,
  KEY_NUMPAD_3,
  KEY_NUMPAD_4,
  KEY_NUMPAD_5,
  KEY_NUMPAD_6,
  KEY_NUMPAD_7,
  KEY_NUMPAD_8,
  KEY_NUMPAD_9,
  KEY_NUMPAD_DIVIDE,
  KEY_NUMPAD_MULTIPLY,
  KEY_NUMPAD_SUBTRACT,
  KEY_NUMPAD_ADD,
  KEY_NUMPAD_ENTER,
  KEY_NUMPAD_DECIMAL,

  KEY_LEFT_SHIFT,
  KEY_RIGHT_SHIFT,
  KEY_LEFT_CTRL,
  KEY_RIGHT_CTRL,
  KEY_LEFT_ALT,
  KEY_RIGHT_ALT,
  KEY_LEFT_SUPER,
  KEY_RIGHT_SUPER,

  KEY_COUNT
} KeyCode;

// ============================================================================
// Mouse Buttons
// ============================================================================

typedef enum MouseButton {
  MOUSE_BUTTON_LEFT = 0,
  MOUSE_BUTTON_RIGHT = 1,
  MOUSE_BUTTON_MIDDLE = 2,
  MOUSE_BUTTON_4 = 3,
  MOUSE_BUTTON_5 = 4,
  MOUSE_BUTTON_COUNT
} MouseButton;

// ============================================================================
// Gamepad
// ============================================================================

typedef enum GamepadButton {
  GAMEPAD_BUTTON_A = 0, // Cross
  GAMEPAD_BUTTON_B,     // Circle
  GAMEPAD_BUTTON_X,     // Square
  GAMEPAD_BUTTON_Y,     // Triangle
  GAMEPAD_BUTTON_LEFT_BUMPER,
  GAMEPAD_BUTTON_RIGHT_BUMPER,
  GAMEPAD_BUTTON_BACK,
  GAMEPAD_BUTTON_START,
  GAMEPAD_BUTTON_GUIDE,
  GAMEPAD_BUTTON_LEFT_THUMB,
  GAMEPAD_BUTTON_RIGHT_THUMB,
  GAMEPAD_BUTTON_DPAD_UP,
  GAMEPAD_BUTTON_DPAD_RIGHT,
  GAMEPAD_BUTTON_DPAD_DOWN,
  GAMEPAD_BUTTON_DPAD_LEFT,
  GAMEPAD_BUTTON_COUNT
} GamepadButton;

typedef enum GamepadAxis {
  GAMEPAD_AXIS_LEFT_X = 0,
  GAMEPAD_AXIS_LEFT_Y,
  GAMEPAD_AXIS_RIGHT_X,
  GAMEPAD_AXIS_RIGHT_Y,
  GAMEPAD_AXIS_LEFT_TRIGGER,
  GAMEPAD_AXIS_RIGHT_TRIGGER,
  GAMEPAD_AXIS_COUNT
} GamepadAxis;

// ============================================================================
// Keyboard
// ============================================================================

VF_API b8 input_key_pressed(KeyCode key);  // Just pressed this frame
VF_API b8 input_key_released(KeyCode key); // Just released this frame
VF_API b8 input_key_down(KeyCode key);     // Currently held
VF_API b8 input_key_up(KeyCode key);       // Currently not held

// ============================================================================
// Mouse
// ============================================================================

VF_API b8 input_mouse_button_pressed(MouseButton button);
VF_API b8 input_mouse_button_released(MouseButton button);
VF_API b8 input_mouse_button_down(MouseButton button);
VF_API Vec2 input_mouse_position(void);
VF_API Vec2 input_mouse_delta(void);
VF_API f32 input_mouse_scroll(void);
VF_API void input_set_mouse_position(Vec2 position);
VF_API void input_set_cursor_visible(b8 visible);
VF_API void input_set_cursor_locked(b8 locked);
VF_API b8 input_is_cursor_visible(void);
VF_API b8 input_is_cursor_locked(void);

// ============================================================================
// Gamepad
// ============================================================================

VF_API b8 input_gamepad_connected(u32 index);
VF_API b8 input_gamepad_button_pressed(u32 index, GamepadButton button);
VF_API b8 input_gamepad_button_released(u32 index, GamepadButton button);
VF_API b8 input_gamepad_button_down(u32 index, GamepadButton button);
VF_API f32 input_gamepad_axis(u32 index, GamepadAxis axis);
VF_API void input_gamepad_set_vibration(u32 index, f32 left_motor,
                                        f32 right_motor);
VF_API void input_gamepad_set_deadzone(f32 deadzone);

// ============================================================================
// Text Input
// ============================================================================

VF_API void input_start_text_input(void);
VF_API void input_stop_text_input(void);
VF_API b8 input_is_text_input_active(void);
VF_API const char *
input_get_text_input(void); // Returns chars entered this frame

// ============================================================================
// Action Mapping
// ============================================================================

typedef u32 InputAction;

VF_API InputAction input_register_action(const char *name);
VF_API void input_bind_key(InputAction action, KeyCode key);
VF_API void input_bind_mouse(InputAction action, MouseButton button);
VF_API void input_bind_gamepad_button(InputAction action, GamepadButton button);
VF_API void input_bind_gamepad_axis(InputAction action, GamepadAxis axis,
                                    f32 threshold);
VF_API b8 input_action_pressed(InputAction action);
VF_API b8 input_action_released(InputAction action);
VF_API b8 input_action_down(InputAction action);
VF_API f32 input_action_value(InputAction action);

// ============================================================================
// System
// ============================================================================

VF_API VF_Result input_init(void);
VF_API void input_shutdown(void);
VF_API void input_update(void);
VF_API void input_clear(void);

// Platform event injection (internal)
VF_API void input_inject_key(KeyCode key, b8 pressed);
VF_API void input_inject_mouse_button(MouseButton button, b8 pressed);
VF_API void input_inject_mouse_move(f32 x, f32 y);
VF_API void input_inject_mouse_scroll(f32 delta);
VF_API void input_inject_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_INPUT_API_H
