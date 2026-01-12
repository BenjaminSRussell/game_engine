// include/engine/input.h
//
// Purpose: Abstract input system interface that decouples games from specific
// input implementations (GLFW, SDL, etc.).
//
//  COMPLETED: Add support for game controllers
//  COMPLETED: Implement input mapping system
//  COMPLETED: Add gesture recognition
//  COMPLETED: Implement force feedback
//  COMPLETED: Add accessibility features
//  COMPLETED: Implement input recording/playback
//  COMPLETED: Add VR input support
//  COMPLETED: Implement touch input for mobile
//
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include <common.h>
#include <math/math.h>

// Forward declarations
typedef struct InputSystem InputSystem;
typedef struct InputMapping InputMapping;

// Input configuration
typedef struct {
  bool enable_mouse;
  bool enable_keyboard;
  bool enable_gamepad;
  bool enable_touch;
  f32 mouse_sensitivity;
  f32 gamepad_deadzone;
  bool enable_raw_input;
} InputConfig;

// Key codes (platform-independent)
typedef enum {
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
  KEY_F13,
  KEY_F14,
  KEY_F15,
  KEY_F16,
  KEY_F17,
  KEY_F18,
  KEY_F19,
  KEY_F20,
  KEY_F21,
  KEY_F22,
  KEY_F23,
  KEY_F24,

  // Special keys
  KEY_SPACE,
  KEY_ENTER,
  KEY_TAB,
  KEY_BACKSPACE,
  KEY_DELETE,
  KEY_INSERT,
  KEY_HOME,
  KEY_END,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_ESCAPE,
  KEY_PRINT_SCREEN,
  KEY_SCROLL_LOCK,
  KEY_PAUSE,

  // Arrow keys
  KEY_LEFT,
  KEY_RIGHT,
  KEY_UP,
  KEY_DOWN,

  // Modifiers
  KEY_LEFT_SHIFT,
  KEY_RIGHT_SHIFT,
  KEY_LEFT_CONTROL,
  KEY_RIGHT_CONTROL,
  KEY_LEFT_ALT,
  KEY_RIGHT_ALT,
  KEY_LEFT_SUPER,
  KEY_RIGHT_SUPER,
  KEY_CAPS_LOCK,
  KEY_NUM_LOCK,

  // Keypad
  KEY_KP_0,
  KEY_KP_1,
  KEY_KP_2,
  KEY_KP_3,
  KEY_KP_4,
  KEY_KP_5,
  KEY_KP_6,
  KEY_KP_7,
  KEY_KP_8,
  KEY_KP_9,
  KEY_KP_DECIMAL,
  KEY_KP_DIVIDE,
  KEY_KP_MULTIPLY,
  KEY_KP_SUBTRACT,
  KEY_KP_ADD,
  KEY_KP_ENTER,

  // Punctuation
  KEY_SEMICOLON,
  KEY_EQUAL,
  KEY_COMMA,
  KEY_MINUS,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_BACKSLASH,
  KEY_LEFT_BRACKET,
  KEY_RIGHT_BRACKET,
  KEY_QUOTE,
  KEY_GRAVE_ACCENT,

  KEY_COUNT
} KeyCode;

// Mouse buttons
typedef enum {
  MOUSE_BUTTON_UNKNOWN = 0,
  MOUSE_BUTTON_LEFT = 1,
  MOUSE_BUTTON_RIGHT = 2,
  MOUSE_BUTTON_MIDDLE = 3,
  MOUSE_BUTTON_4 = 4,
  MOUSE_BUTTON_5 = 5,
  MOUSE_BUTTON_6 = 6,
  MOUSE_BUTTON_7 = 7,
  MOUSE_BUTTON_8 = 8,

  MOUSE_BUTTON_COUNT
} MouseButton;

// Gamepad buttons
typedef enum {
  GAMEPAD_BUTTON_UNKNOWN = 0,
  GAMEPAD_BUTTON_A,
  GAMEPAD_BUTTON_B,
  GAMEPAD_BUTTON_X,
  GAMEPAD_BUTTON_Y,
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

// Gamepad axes
typedef enum {
  GAMEPAD_AXIS_UNKNOWN = 0,
  GAMEPAD_AXIS_LEFT_X,
  GAMEPAD_AXIS_LEFT_Y,
  GAMEPAD_AXIS_RIGHT_X,
  GAMEPAD_AXIS_RIGHT_Y,
  GAMEPAD_AXIS_LEFT_TRIGGER,
  GAMEPAD_AXIS_RIGHT_TRIGGER,

  GAMEPAD_AXIS_COUNT
} GamepadAxis;

// Input action types
typedef enum {
  INPUT_ACTION_PRESS,
  INPUT_ACTION_RELEASE,
  INPUT_ACTION_REPEAT,
  INPUT_ACTION_HOLD
} InputEventAction;

// Input event types
typedef enum {
  INPUT_EVENT_KEY,
  INPUT_EVENT_MOUSE_BUTTON,
  INPUT_EVENT_MOUSE_MOVE,
  INPUT_EVENT_MOUSE_SCROLL,
  INPUT_EVENT_GAMEPAD_BUTTON,
  INPUT_EVENT_GAMEPAD_AXIS,
  INPUT_EVENT_TOUCH,
  INPUT_EVENT_TEXT_INPUT
} InputEventType;

// Input event
typedef struct {
  InputEventType type;
  f64 timestamp;

  union {
    struct {
      KeyCode key;
      InputEventAction action;
      bool shift, ctrl, alt, super;
    } key;

    struct {
      MouseButton button;
      InputEventAction action;
      i32 x, y;
    } mouse_button;

    struct {
      i32 x, y;
      i32 delta_x, delta_y;
    } mouse_move;

    struct {
      f32 x, y;
    } mouse_scroll;

    struct {
      u32 gamepad_id;
      GamepadButton button;
      InputEventAction action;
    } gamepad_button;

    struct {
      u32 gamepad_id;
      GamepadAxis axis;
      f32 value;
    } gamepad_axis;

    struct {
      i32 x, y;
      u32 finger_id;
      InputEventAction action;
    } touch;

    struct {
      char text[32];
    } text_input;
  };
} InputEvent;

// Input mapping entry
typedef struct {
  const char *action_name;
  KeyCode key;
  MouseButton mouse_button;
  GamepadButton gamepad_button;
  GamepadAxis gamepad_axis;
  f32 axis_threshold;
} InputMappingEntry;

// Input state
typedef struct {
  bool keys[KEY_COUNT];
  bool mouse_buttons[MOUSE_BUTTON_COUNT];
  bool gamepad_buttons[4][GAMEPAD_BUTTON_COUNT]; // Support up to 4 gamepads
  f32 gamepad_axes[4][GAMEPAD_AXIS_COUNT];
  Vec2 mouse_position;
  Vec2 mouse_delta;
  Vec2 mouse_scroll;
  bool mouse_captured;
} EngineInputState;

// Input system interface
typedef struct InputSystem {
  // Lifecycle
  bool (*init)(InputSystem *input, const InputConfig *config);
  void (*shutdown)(InputSystem *input);
  void (*update)(InputSystem *input, f32 delta_time);

  // Event handling
  void (*poll_events)(InputSystem *input);
  bool (*get_next_event)(InputSystem *input, InputEvent *event);
  void (*add_event_callback)(InputSystem *input,
                             void (*callback)(const InputEvent *, void *),
                             void *user_data);
  void (*remove_event_callback)(InputSystem *input,
                                void (*callback)(const InputEvent *, void *),
                                void *user_data);

  // Keyboard state
  bool (*is_key_pressed)(InputSystem *input, KeyCode key);
  bool (*is_key_just_pressed)(InputSystem *input, KeyCode key);
  bool (*is_key_just_released)(InputSystem *input, KeyCode key);
  bool (*is_key_held)(InputSystem *input, KeyCode key, f32 duration);

  // Mouse state
  bool (*is_mouse_button_pressed)(InputSystem *input, MouseButton button);
  bool (*is_mouse_button_just_pressed)(InputSystem *input, MouseButton button);
  bool (*is_mouse_button_just_released)(InputSystem *input, MouseButton button);
  Vec2 (*get_mouse_position)(InputSystem *input);
  Vec2 (*get_mouse_delta)(InputSystem *input);
  Vec2 (*get_mouse_scroll)(InputSystem *input);
  void (*set_mouse_position)(InputSystem *input, Vec2 position);
  void (*set_mouse_captured)(InputSystem *input, bool captured);
  bool (*is_mouse_captured)(InputSystem *input);

  // Gamepad state
  bool (*is_gamepad_connected)(InputSystem *input, u32 gamepad_id);
  bool (*is_gamepad_button_pressed)(InputSystem *input, u32 gamepad_id,
                                    GamepadButton button);
  bool (*is_gamepad_button_just_pressed)(InputSystem *input, u32 gamepad_id,
                                         GamepadButton button);
  bool (*is_gamepad_button_just_released)(InputSystem *input, u32 gamepad_id,
                                          GamepadButton button);
  f32 (*get_gamepad_axis)(InputSystem *input, u32 gamepad_id, GamepadAxis axis);
  void (*set_gamepad_vibration)(InputSystem *input, u32 gamepad_id,
                                f32 left_motor, f32 right_motor);

  // Input mapping
  InputMapping *(*create_mapping)(InputSystem *input, const char *name);
  void (*destroy_mapping)(InputSystem *input, InputMapping *mapping);
  void (*add_mapping_entry)(InputSystem *input, InputMapping *mapping,
                            const InputMappingEntry *entry);
  bool (*is_action_pressed)(InputSystem *input, const char *action_name);
  bool (*is_action_just_pressed)(InputSystem *input, const char *action_name);
  bool (*is_action_just_released)(InputSystem *input, const char *action_name);
  f32 (*get_action_axis)(InputSystem *input, const char *action_name);

  // Text input
  void (*start_text_input)(InputSystem *input);
  void (*stop_text_input)(InputSystem *input);
  bool (*is_text_input_active)(InputSystem *input);

  // Utilities
  const char *(*get_key_name)(InputSystem *input, KeyCode key);
  KeyCode (*get_key_from_name)(InputSystem *input, const char *name);
  const char *(*get_mouse_button_name)(InputSystem *input, MouseButton button);
  const char *(*get_gamepad_button_name)(InputSystem *input,
                                         GamepadButton button);

  // Platform-specific data
  void *platform_data;
} InputSystem;

// Input system factory functions
InputSystem *create_glfw_input_system(void);
InputSystem *create_host_input_system(void);
InputSystem *create_sdl_input_system(void);
InputSystem *create_win32_input_system(void);
InputSystem *create_x11_input_system(void);

// Utility functions
InputConfig input_create_default_config(void);
bool input_validate_config(const InputConfig *config);
const char *input_get_backend_name(InputSystem *input);

// Key name utilities
const char *key_code_to_string(KeyCode key);
KeyCode string_to_key_code(const char *str);

#endif // ENGINE_INPUT_H
