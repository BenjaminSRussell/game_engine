// include/input/controls.h
//
// Purpose: Defines the public API and data structures for managing player input
// and control bindings within the game. This header provides a comprehensive
// system for mapping physical input events (keyboard keys, mouse movements,
// controller buttons) to logical game actions. It tracks the state of various
// input devices and offers functions to query action states, manage bindings,
// and handle control settings.
//
// Public APIs:
// - `InputAction`: Enumeration defining a wide range of distinct game actions
//   (e.g., `MOVE_FORWARD`, `ATTACK`, `INVENTORY`, `MENU`).
// - `InputState`: Structure to hold the current state of all input devices
//   (keyboard keys, mouse position/delta/scroll, controller
//   sticks/triggers/buttons) and the derived states of game actions (pressed,
//   held, released). It also stores key and mouse bindings.
// - `input_init`: Initializes the input system, setting up default states.
// - `input_update`: Updates the input state each frame, processing raw input
//   and deriving action states.
// - Input device handling: `input_set_key`, `input_set_mouse_button`,
//   `input_set_mouse_position`, `input_set_mouse_delta`,
//   `input_set_mouse_scroll` for external input event processing.
// - Action queries: `input_is_action_pressed`, `input_is_action_held`,
// `input_is_action_released`
//   to check the state of specific game actions.
// - Key/mouse binding: `input_bind_key`, `input_bind_mouse`,
// `input_get_key_binding`
//   for custom remapping of controls.
// - Control settings management: `controls_set_defaults`, `controls_load`,
// `controls_save`
//   for persistent storage of control configurations.
// - `input_calculate_movement`: Calculates player movement based on current
// input and game settings.
//
// Ownership: The `InputState` instance manages its internal arrays for key,
// mouse, and action states. It uses `GameConfig` (aliased as `ControlSettings`)
// for configuration.
//
// Invariants:
// - An `InputState` must be initialized with `input_init` before processing
// input.
// - `input_update` should be called once per frame to maintain accurate action
// states.
// - `InputAction` values must be unique and within the bounds defined by
// `INPUT_ACTION_COUNT`.
// - Key and mouse bindings are stored internally and applied during
// `input_update`.
//
#ifndef CONTROLS_H
#define CONTROLS_H

#include <common.h>
#include <config/config.h>
#include <math/vec2.h>
#include <math/vec3.h>

typedef GameConfig ControlSettings;

// Key storage size (large enough for GLFW key codes).
#define INPUT_KEY_COUNT 512

// Common key codes (match GLFW key values).
#define INPUT_KEY_ESCAPE 256
#define INPUT_KEY_TAB 258
#define INPUT_KEY_F2 291
#define INPUT_KEY_F3 292
#define INPUT_KEY_LEFT_SHIFT 340
#define INPUT_KEY_LEFT_CONTROL 341

// Input action types
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

// Input state
typedef struct InputState {
  bool keys[INPUT_KEY_COUNT];
  bool mouse_buttons[8];
  f32 mouse_x;
  f32 mouse_y;
  f32 mouse_delta_x;
  f32 mouse_delta_y;
  f32 mouse_scroll;

  // Controller state
  Vec2 left_stick;
  Vec2 right_stick;
  f32 left_trigger;
  f32 right_trigger;
  u32 controller_buttons;
  u32 controller_dpad;

  // Input timing (frame-based)
  u32 frame_index;
  u32 action_last_pressed_frame[INPUT_ACTION_COUNT];
  u32 action_last_released_frame[INPUT_ACTION_COUNT];
  u32 action_hold_frames[INPUT_ACTION_COUNT];

  // Haptics
  bool haptics_enabled;
  f32 haptic_low;
  f32 haptic_high;

  // Action states
  bool actions[INPUT_ACTION_COUNT];
  bool actions_pressed[INPUT_ACTION_COUNT];
  bool actions_released[INPUT_ACTION_COUNT];

  // Key bindings
  u32 key_bindings[INPUT_ACTION_COUNT];
  u32 mouse_binding[INPUT_ACTION_COUNT];
} InputState;

// Input profile storage (bindings only).
#define INPUT_PROFILE_NAME_MAX 32
#define INPUT_PROFILE_MAX 8

typedef struct InputProfile {
  char name[INPUT_PROFILE_NAME_MAX];
  u32 key_bindings[INPUT_ACTION_COUNT];
  u32 mouse_bindings[INPUT_ACTION_COUNT];
} InputProfile;

typedef struct InputProfiles {
  InputProfile profiles[INPUT_PROFILE_MAX];
  u32 count;
  char active_profile[INPUT_PROFILE_NAME_MAX];
} InputProfiles;

// Controller button bitmasks
#define CONTROLLER_BUTTON_A (1u << 0)
#define CONTROLLER_BUTTON_B (1u << 1)
#define CONTROLLER_BUTTON_X (1u << 2)
#define CONTROLLER_BUTTON_Y (1u << 3)
#define CONTROLLER_BUTTON_LB (1u << 4)
#define CONTROLLER_BUTTON_RB (1u << 5)
#define CONTROLLER_BUTTON_BACK (1u << 6)
#define CONTROLLER_BUTTON_START (1u << 7)
#define CONTROLLER_BUTTON_GUIDE (1u << 8)
#define CONTROLLER_BUTTON_LEFT_THUMB (1u << 9)
#define CONTROLLER_BUTTON_RIGHT_THUMB (1u << 10)

// Controller d-pad bitmasks
#define CONTROLLER_DPAD_UP (1u << 0)
#define CONTROLLER_DPAD_RIGHT (1u << 1)
#define CONTROLLER_DPAD_DOWN (1u << 2)
#define CONTROLLER_DPAD_LEFT (1u << 3)

struct GameConfig;

// Initialize input system
void input_init(InputState *input);
void input_update(InputState *input);

// Key/mouse handling
void input_set_key(InputState *input, u32 key, bool pressed);
void input_set_mouse_button(InputState *input, u32 button, bool pressed);
void input_set_mouse_position(InputState *input, f32 x, f32 y);
void input_set_mouse_delta(InputState *input, f32 dx, f32 dy);
void input_set_mouse_scroll(InputState *input, f32 scroll);
void input_set_controller_state(InputState *input, Vec2 left_stick,
                                Vec2 right_stick, f32 left_trigger,
                                f32 right_trigger, u32 buttons, u32 dpad);

// Action queries
bool input_is_action_pressed(InputState *input, InputAction action);
bool input_is_action_held(InputState *input, InputAction action);
bool input_is_action_released(InputState *input, InputAction action);

// Key binding
void input_bind_key(InputState *input, InputAction action, u32 key);
void input_bind_mouse(InputState *input, InputAction action, u32 button);
u32 input_get_key_binding(InputState *input, InputAction action);

// Profiles and persistence
const char *input_action_name(InputAction action);
bool input_action_from_name(const char *name, InputAction *out_action);
void input_profiles_init(InputProfiles *profiles);
void input_profiles_capture(InputProfiles *profiles, const char *name,
                            const InputState *input);
bool input_profiles_set_active(InputProfiles *profiles, const char *name);
void input_profiles_apply_active(InputState *input,
                                 const InputProfiles *profiles);
bool input_profiles_save(const InputProfiles *profiles, const char *filename);
bool input_profiles_load(InputProfiles *profiles, const char *filename);

// Control settings
void controls_set_defaults(struct GameConfig *settings);
void controls_load(struct GameConfig *settings, const char *filename);
void controls_save(struct GameConfig *settings, const char *filename);

// Smooth movement calculation
Vec3 input_calculate_movement(InputState *input, struct GameConfig *settings,
                              Vec3 forward, Vec3 right, f32 delta_time);

#endif // CONTROLS_H
