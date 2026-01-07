/**
 * =================================================================================================
 *                              INPUT SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_INPUT_1
 * =================================================================================================
 */

#include "platform/input/input_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8
#define MAX_GAMEPADS 4
#define MAX_GAMEPAD_BUTTONS 32
#define MAX_GAMEPAD_AXES 8
#define MAX_TOUCHES 16
#define MAX_ACTIONS 128

/* =================================================================================================
 *                                    STATE STRUCTURES
 * =================================================================================================
 */

typedef struct KeyboardState {
  bool current[MAX_KEYS];
  bool previous[MAX_KEYS];
  char char_buffer[256];
  uint32_t char_count;
  uint32_t modifiers;
  int32_t last_key;
  bool initialized;
} KeyboardState;

typedef struct MouseState {
  float position[2];
  float previous_position[2];
  float delta[2];
  float scroll[2];
  bool current[MAX_MOUSE_BUTTONS];
  bool previous[MAX_MOUSE_BUTTONS];
  bool cursor_visible;
  bool cursor_locked;
  bool inside_window;
  bool initialized;
} MouseState;

typedef struct GamepadState {
  bool connected;
  char name[128];
  bool current_buttons[MAX_GAMEPAD_BUTTONS];
  bool previous_buttons[MAX_GAMEPAD_BUTTONS];
  float axes[MAX_GAMEPAD_AXES];
  float triggers[2];
  float deadzone;
  float vibration_left;
  float vibration_right;
} GamepadState;

typedef struct TouchPoint {
  uint32_t id;
  float position[2];
  float previous_position[2];
  float delta[2];
  float pressure;
  uint32_t phase; // 0=began, 1=moved, 2=stationary, 3=ended
} TouchPoint;

typedef struct TouchState {
  TouchPoint touches[MAX_TOUCHES];
  uint32_t touch_count;
  bool initialized;
} TouchState;

typedef struct InputAction {
  char name[64];
  uint32_t key_binding;
  uint32_t mouse_binding;
  uint32_t gamepad_binding;
  float value;
  bool triggered;
  bool active;
} InputAction;

typedef struct InputManager {
  KeyboardState keyboard;
  MouseState mouse;
  GamepadState gamepads[MAX_GAMEPADS];
  TouchState touch;
  InputAction actions[MAX_ACTIONS];
  uint32_t action_count;
  bool enabled;
  uint32_t last_input_device;
  bool initialized;
} InputManager;

static InputManager g_input = {0};

/* =================================================================================================
 *                                    KEYBOARD IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement keyboard_init
bool keyboard_init(void) {
  memset(&g_input.keyboard, 0, sizeof(KeyboardState));
  g_input.keyboard.last_key = -1;
  g_input.keyboard.initialized = true;
  return true;
}

// DONE: Implement keyboard_shutdown
void keyboard_shutdown(void) {
  memset(&g_input.keyboard, 0, sizeof(KeyboardState));
}

// DONE: Implement keyboard_update
void keyboard_update(void) {
  memcpy(g_input.keyboard.previous, g_input.keyboard.current,
         sizeof(g_input.keyboard.current));
  g_input.keyboard.char_count = 0;
}

// DONE: Implement keyboard_is_key_pressed
bool keyboard_is_key_pressed(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return g_input.keyboard.current[key] && !g_input.keyboard.previous[key];
}

// DONE: Implement keyboard_is_key_down
bool keyboard_is_key_down(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return g_input.keyboard.current[key];
}

// DONE: Implement keyboard_is_key_up
bool keyboard_is_key_up(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return !g_input.keyboard.current[key];
}

// DONE: Implement keyboard_is_key_released
bool keyboard_is_key_released(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return !g_input.keyboard.current[key] && g_input.keyboard.previous[key];
}

// DONE: Implement keyboard_get_char_input
const char *keyboard_get_char_input(uint32_t *count) {
  if (count)
    *count = g_input.keyboard.char_count;
  return g_input.keyboard.char_buffer;
}

// DONE: Implement keyboard_get_modifiers
uint32_t keyboard_get_modifiers(void) { return g_input.keyboard.modifiers; }

// DONE: Implement keyboard_is_any_key_pressed
bool keyboard_is_any_key_pressed(void) {
  for (int i = 0; i < MAX_KEYS; i++) {
    if (keyboard_is_key_pressed(i))
      return true;
  }
  return false;
}

// DONE: Implement keyboard_get_last_key
int keyboard_get_last_key(void) { return g_input.keyboard.last_key; }

// DONE: Implement keyboard_clear
void keyboard_clear(void) {
  memset(g_input.keyboard.current, 0, sizeof(g_input.keyboard.current));
  memset(g_input.keyboard.previous, 0, sizeof(g_input.keyboard.previous));
}

// Internal: process key event
void keyboard_key_event(int key, bool down) {
  if (key < 0 || key >= MAX_KEYS)
    return;
  g_input.keyboard.current[key] = down;
  if (down)
    g_input.keyboard.last_key = key;
}

void keyboard_char_event(char c) {
  if (g_input.keyboard.char_count < 255) {
    g_input.keyboard.char_buffer[g_input.keyboard.char_count++] = c;
    g_input.keyboard.char_buffer[g_input.keyboard.char_count] = '\0';
  }
}

void keyboard_modifier_event(uint32_t mods) {
  g_input.keyboard.modifiers = mods;
}

/* =================================================================================================
 *                                    MOUSE IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement mouse_init
bool mouse_init(void) {
  memset(&g_input.mouse, 0, sizeof(MouseState));
  g_input.mouse.cursor_visible = true;
  g_input.mouse.initialized = true;
  return true;
}

// DONE: Implement mouse_shutdown
void mouse_shutdown(void) { memset(&g_input.mouse, 0, sizeof(MouseState)); }

// DONE: Implement mouse_update
void mouse_update(void) {
  memcpy(g_input.mouse.previous, g_input.mouse.current,
         sizeof(g_input.mouse.current));
  g_input.mouse.previous_position[0] = g_input.mouse.position[0];
  g_input.mouse.previous_position[1] = g_input.mouse.position[1];
  g_input.mouse.delta[0] = 0;
  g_input.mouse.delta[1] = 0;
  g_input.mouse.scroll[0] = 0;
  g_input.mouse.scroll[1] = 0;
}

// DONE: Implement mouse_get_position
void mouse_get_position(float *x, float *y) {
  if (x)
    *x = g_input.mouse.position[0];
  if (y)
    *y = g_input.mouse.position[1];
}

// DONE: Implement mouse_get_delta
void mouse_get_delta(float *dx, float *dy) {
  if (dx)
    *dx = g_input.mouse.delta[0];
  if (dy)
    *dy = g_input.mouse.delta[1];
}

// DONE: Implement mouse_get_scroll
void mouse_get_scroll(float *sx, float *sy) {
  if (sx)
    *sx = g_input.mouse.scroll[0];
  if (sy)
    *sy = g_input.mouse.scroll[1];
}

// DONE: Implement mouse_is_button_pressed
bool mouse_is_button_pressed(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return g_input.mouse.current[button] && !g_input.mouse.previous[button];
}

// DONE: Implement mouse_is_button_down
bool mouse_is_button_down(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return g_input.mouse.current[button];
}

// DONE: Implement mouse_is_button_up
bool mouse_is_button_up(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return !g_input.mouse.current[button];
}

// DONE: Implement mouse_is_button_released
bool mouse_is_button_released(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return !g_input.mouse.current[button] && g_input.mouse.previous[button];
}

// DONE: Implement mouse_set_cursor_visible
void mouse_set_cursor_visible(bool visible) {
  g_input.mouse.cursor_visible = visible;
  // Platform-specific implementation would go here
}

// DONE: Implement mouse_set_cursor_locked
void mouse_set_cursor_locked(bool locked) {
  g_input.mouse.cursor_locked = locked;
  // Platform-specific implementation would go here
}

// DONE: Implement mouse_is_inside_window
bool mouse_is_inside_window(void) { return g_input.mouse.inside_window; }

// Internal: process mouse events
void mouse_position_event(float x, float y) {
  g_input.mouse.delta[0] = x - g_input.mouse.position[0];
  g_input.mouse.delta[1] = y - g_input.mouse.position[1];
  g_input.mouse.position[0] = x;
  g_input.mouse.position[1] = y;
}

void mouse_button_event(int button, bool down) {
  if (button >= 0 && button < MAX_MOUSE_BUTTONS) {
    g_input.mouse.current[button] = down;
  }
}

void mouse_scroll_event(float sx, float sy) {
  g_input.mouse.scroll[0] = sx;
  g_input.mouse.scroll[1] = sy;
}

void mouse_enter_event(bool inside) { g_input.mouse.inside_window = inside; }

/* =================================================================================================
 *                                    GAMEPAD IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement gamepad_init
bool gamepad_init(void) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    memset(&g_input.gamepads[i], 0, sizeof(GamepadState));
    g_input.gamepads[i].deadzone = 0.15f;
  }
  return true;
}

// DONE: Implement gamepad_shutdown
void gamepad_shutdown(void) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    memset(&g_input.gamepads[i], 0, sizeof(GamepadState));
  }
}

// DONE: Implement gamepad_update
void gamepad_update(void) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    memcpy(g_input.gamepads[i].previous_buttons,
           g_input.gamepads[i].current_buttons,
           sizeof(g_input.gamepads[i].current_buttons));
  }
}

// DONE: Implement gamepad_is_connected
bool gamepad_is_connected(int index) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return false;
  return g_input.gamepads[index].connected;
}

// DONE: Implement gamepad_get_name
const char *gamepad_get_name(int index) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return NULL;
  return g_input.gamepads[index].name;
}

// DONE: Implement gamepad_is_button_pressed
bool gamepad_is_button_pressed(int index, int button) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return false;
  if (button < 0 || button >= MAX_GAMEPAD_BUTTONS)
    return false;
  GamepadState *gp = &g_input.gamepads[index];
  return gp->current_buttons[button] && !gp->previous_buttons[button];
}

// DONE: Implement gamepad_is_button_down
bool gamepad_is_button_down(int index, int button) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return false;
  if (button < 0 || button >= MAX_GAMEPAD_BUTTONS)
    return false;
  return g_input.gamepads[index].current_buttons[button];
}

// DONE: Implement gamepad_is_button_released
bool gamepad_is_button_released(int index, int button) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return false;
  if (button < 0 || button >= MAX_GAMEPAD_BUTTONS)
    return false;
  GamepadState *gp = &g_input.gamepads[index];
  return !gp->current_buttons[button] && gp->previous_buttons[button];
}

// DONE: Implement gamepad_get_axis
float gamepad_get_axis(int index, int axis) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return 0.0f;
  if (axis < 0 || axis >= MAX_GAMEPAD_AXES)
    return 0.0f;
  return g_input.gamepads[index].axes[axis];
}

// DONE: Implement gamepad_get_trigger
float gamepad_get_trigger(int index, int trigger) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return 0.0f;
  if (trigger < 0 || trigger >= 2)
    return 0.0f;
  return g_input.gamepads[index].triggers[trigger];
}

// DONE: Implement gamepad_apply_deadzone
float gamepad_apply_deadzone(float value, float deadzone) {
  if (value > -deadzone && value < deadzone)
    return 0.0f;
  float sign = value > 0 ? 1.0f : -1.0f;
  return sign * (fabsf(value) - deadzone) / (1.0f - deadzone);
}

// DONE: Implement gamepad_set_vibration
void gamepad_set_vibration(int index, float left, float right) {
  if (index < 0 || index >= MAX_GAMEPADS)
    return;
  g_input.gamepads[index].vibration_left = left;
  g_input.gamepads[index].vibration_right = right;
  // Platform-specific haptics would go here
}

// DONE: Implement gamepad_stop_vibration
void gamepad_stop_vibration(int index) {
  gamepad_set_vibration(index, 0.0f, 0.0f);
}

/* =================================================================================================
 *                                    INPUT ACTIONS
 * =================================================================================================
 */

// DONE: Implement action_register
int action_register(const char *name) {
  if (g_input.action_count >= MAX_ACTIONS)
    return -1;

  int id = g_input.action_count++;
  InputAction *action = &g_input.actions[id];

  memset(action, 0, sizeof(InputAction));
  strncpy(action->name, name, 63);
  action->active = true;

  return id;
}

// DONE: Implement action_unregister
void action_unregister(int id) {
  if (id < 0 || id >= (int)g_input.action_count)
    return;
  g_input.actions[id].active = false;
}

// DONE: Implement action_bind_key
void action_bind_key(int id, int key) {
  if (id < 0 || id >= (int)g_input.action_count)
    return;
  g_input.actions[id].key_binding = key;
}

// DONE: Implement action_bind_mouse
void action_bind_mouse(int id, int button) {
  if (id < 0 || id >= (int)g_input.action_count)
    return;
  g_input.actions[id].mouse_binding = button;
}

// DONE: Implement action_bind_gamepad
void action_bind_gamepad(int id, int button) {
  if (id < 0 || id >= (int)g_input.action_count)
    return;
  g_input.actions[id].gamepad_binding = button;
}

// DONE: Implement action_is_triggered
bool action_is_triggered(int id) {
  if (id < 0 || id >= (int)g_input.action_count)
    return false;
  InputAction *a = &g_input.actions[id];
  if (!a->active)
    return false;

  return keyboard_is_key_pressed(a->key_binding) ||
         mouse_is_button_pressed(a->mouse_binding) ||
         gamepad_is_button_pressed(0, a->gamepad_binding);
}

// DONE: Implement action_get_value
float action_get_value(int id) {
  if (id < 0 || id >= (int)g_input.action_count)
    return 0.0f;
  InputAction *a = &g_input.actions[id];
  if (!a->active)
    return 0.0f;

  if (keyboard_is_key_down(a->key_binding))
    return 1.0f;
  if (mouse_is_button_down(a->mouse_binding))
    return 1.0f;
  if (gamepad_is_button_down(0, a->gamepad_binding))
    return 1.0f;

  return 0.0f;
}

/* =================================================================================================
 *                                    INPUT MANAGER
 * =================================================================================================
 */

// DONE: Implement input_manager_init
bool input_manager_init(void) {
  memset(&g_input, 0, sizeof(InputManager));

  if (!keyboard_init())
    return false;
  if (!mouse_init())
    return false;
  if (!gamepad_init())
    return false;

  g_input.enabled = true;
  g_input.initialized = true;
  return true;
}

// DONE: Implement input_manager_shutdown
void input_manager_shutdown(void) {
  keyboard_shutdown();
  mouse_shutdown();
  gamepad_shutdown();
  memset(&g_input, 0, sizeof(InputManager));
}

// DONE: Implement input_manager_update
void input_manager_update(void) {
  if (!g_input.enabled)
    return;

  keyboard_update();
  mouse_update();
  gamepad_update();
}

// DONE: Implement input_manager_set_enabled
void input_manager_set_enabled(bool enabled) { g_input.enabled = enabled; }

// DONE: Implement input_manager_is_any_input
bool input_manager_is_any_input(void) {
  if (keyboard_is_any_key_pressed())
    return true;
  for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) {
    if (mouse_is_button_pressed(i))
      return true;
  }
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    if (!gamepad_is_connected(i))
      continue;
    for (int b = 0; b < MAX_GAMEPAD_BUTTONS; b++) {
      if (gamepad_is_button_pressed(i, b))
        return true;
    }
  }
  return false;
}

// DONE: Implement input_manager_get_last_device
uint32_t input_manager_get_last_device(void) {
  return g_input.last_input_device;
}
