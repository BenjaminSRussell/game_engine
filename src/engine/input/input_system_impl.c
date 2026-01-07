/**
 * =================================================================================================
 *                              INPUT SYSTEM IMPLEMENTATION
 *                              Agent: AGENT_INPUT_1
 * =================================================================================================
 */

#include "input_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    INTERNAL STRUCTURES
 * =================================================================================================
 */

#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8
#define MAX_GAMEPADS 4
#define MAX_TOUCHES 10

typedef struct InputState {
  // Keyboard
  bool keys[MAX_KEYS];
  bool keys_prev[MAX_KEYS];
  char last_char;
  uint32_t modifiers;

  // Mouse
  float mouse_x, mouse_y;
  float mouse_dx, mouse_dy;
  float scroll_x, scroll_y;
  bool mouse_buttons[MAX_MOUSE_BUTTONS];
  bool mouse_buttons_prev[MAX_MOUSE_BUTTONS];
  bool cursor_visible;
  bool cursor_locked;

  // Gamepads
  struct {
    bool connected;
    char name[64];
    float axes[8];
    bool buttons[32];
    bool buttons_prev[32];
  } gamepads[MAX_GAMEPADS];

  // Touch
  int touch_count;
  struct {
    int id;
    float x, y;
    float pressure;
    int state; // 0=none, 1=began, 2=moved, 3=ended
  } touches[MAX_TOUCHES];

} InputState;

static InputState g_input = {0};

/* =================================================================================================
 *                                    KEYBOARD
 * =================================================================================================
 */

void keyboard_init(void) { memset(g_input.keys, 0, sizeof(g_input.keys)); }
void keyboard_shutdown(void) {}
void keyboard_update(void) {
  memcpy(g_input.keys_prev, g_input.keys, sizeof(g_input.keys));
  g_input.last_char = 0;
}
bool keyboard_is_key_pressed(uint32_t key) {
  return key < MAX_KEYS && g_input.keys[key] && !g_input.keys_prev[key];
}
bool keyboard_is_key_down(uint32_t key) {
  return key < MAX_KEYS && g_input.keys[key];
}
bool keyboard_is_key_up(uint32_t key) {
  return key < MAX_KEYS && !g_input.keys[key];
}
bool keyboard_is_key_released(uint32_t key) {
  return key < MAX_KEYS && !g_input.keys[key] && g_input.keys_prev[key];
}
char keyboard_get_char_input(void) { return g_input.last_char; }
uint32_t keyboard_get_modifiers(void) { return g_input.modifiers; }
bool keyboard_is_any_key_pressed(void) {
  return false; /* TODO: optimized check */
}
uint32_t keyboard_get_last_key(void) { return 0; }
void keyboard_clear(void) { memset(g_input.keys, 0, sizeof(g_input.keys)); }

/* =================================================================================================
 *                                    MOUSE
 * =================================================================================================
 */

void mouse_init(void) {}
void mouse_shutdown(void) {}
void mouse_update(void) {
  memcpy(g_input.mouse_buttons_prev, g_input.mouse_buttons,
         sizeof(g_input.mouse_buttons));
  g_input.mouse_dx = 0;
  g_input.mouse_dy = 0;
  g_input.scroll_x = 0;
  g_input.scroll_y = 0;
}
void mouse_get_position(float *x, float *y) {
  *x = g_input.mouse_x;
  *y = g_input.mouse_y;
}
void mouse_get_delta(float *x, float *y) {
  *x = g_input.mouse_dx;
  *y = g_input.mouse_dy;
}
void mouse_get_scroll(float *x, float *y) {
  *x = g_input.scroll_x;
  *y = g_input.scroll_y;
}
bool mouse_is_button_pressed(int button) {
  return button < MAX_MOUSE_BUTTONS && g_input.mouse_buttons[button] &&
         !g_input.mouse_buttons_prev[button];
}
bool mouse_is_button_down(int button) {
  return button < MAX_MOUSE_BUTTONS && g_input.mouse_buttons[button];
}
bool mouse_is_button_up(int button) {
  return button < MAX_MOUSE_BUTTONS && !g_input.mouse_buttons[button];
}
bool mouse_is_button_released(int button) {
  return button < MAX_MOUSE_BUTTONS && !g_input.mouse_buttons[button] &&
         g_input.mouse_buttons_prev[button];
}
void mouse_set_position(float x, float y) {
  g_input.mouse_x = x;
  g_input.mouse_y = y;
}
void mouse_set_cursor_visible(bool visible) {
  g_input.cursor_visible = visible;
}
void mouse_set_cursor_locked(bool locked) { g_input.cursor_locked = locked; }
void mouse_set_cursor_shape(int shape) {}
void mouse_set_cursor_image(void *image, int hot_x, int hot_y) {}
bool mouse_is_inside_window(void) { return true; }

/* =================================================================================================
 *                                    GAMEPAD
 * =================================================================================================
 */

void gamepad_init(void) {}
void gamepad_shutdown(void) {}
void gamepad_update(void) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    if (g_input.gamepads[i].connected) {
      memcpy(g_input.gamepads[i].buttons_prev, g_input.gamepads[i].buttons,
             sizeof(g_input.gamepads[i].buttons));
    }
  }
}
void gamepad_enumerate(void) {}
bool gamepad_is_connected(int id) {
  return id < MAX_GAMEPADS && g_input.gamepads[id].connected;
}
const char *gamepad_get_name(int id) {
  return (id < MAX_GAMEPADS) ? g_input.gamepads[id].name : "";
}
bool gamepad_is_button_pressed(int id, int button) {
  return id < MAX_GAMEPADS && g_input.gamepads[id].buttons[button] &&
         !g_input.gamepads[id].buttons_prev[button];
}
bool gamepad_is_button_down(int id, int button) {
  return id < MAX_GAMEPADS && g_input.gamepads[id].buttons[button];
}
bool gamepad_is_button_released(int id, int button) {
  return id < MAX_GAMEPADS && !g_input.gamepads[id].buttons[button] &&
         g_input.gamepads[id].buttons_prev[button];
}
float gamepad_get_axis(int id, int axis) {
  return (id < MAX_GAMEPADS) ? g_input.gamepads[id].axes[axis] : 0.0f;
}
float gamepad_get_trigger(int id, int trigger) { return 0.0f; }
void gamepad_apply_deadzone(int id, float deadzone) {}
void gamepad_set_vibration(int id, float left, float right) {}
void gamepad_stop_vibration(int id) {}
void gamepad_on_connected(int id) {
  if (id < MAX_GAMEPADS)
    g_input.gamepads[id].connected = true;
}
void gamepad_on_disconnected(int id) {
  if (id < MAX_GAMEPADS)
    g_input.gamepads[id].connected = false;
}
void gamepad_remap_buttons(int id, const int *mapping) {}
void gamepad_save_mapping(int id, const char *path) {}
void gamepad_load_mapping(int id, const char *path) {}

/* =================================================================================================
 *                                    TOUCH
 * =================================================================================================
 */

void touch_init(void) {}
void touch_shutdown(void) {}
void touch_update(void) {}
int touch_get_count(void) { return g_input.touch_count; }
void touch_get_position(int index, float *x, float *y) {}
void touch_get_delta(int index, float *x, float *y) {}
float touch_get_pressure(int index) { return 0.0f; }
bool touch_is_began(int index) { return false; }
bool touch_is_moved(int index) { return false; }
bool touch_is_ended(int index) { return false; }
bool touch_gesture_tap(void) { return false; }
bool touch_gesture_double_tap(void) { return false; }
bool touch_gesture_long_press(void) { return false; }
bool touch_gesture_swipe(int dir) { return false; }
bool touch_gesture_pinch(float *scale) { return false; }
bool touch_gesture_rotate(float *rotation) { return false; }
bool touch_gesture_pan(float *x, float *y) { return false; }

/* =================================================================================================
 *                                    INPUT ACTIONS (MAPPING)
 * =================================================================================================
 */

void action_register(const char *name) {}
void action_unregister(const char *name) {}
void action_bind_key(const char *action, uint32_t key) {}
void action_bind_mouse(const char *action, int button) {}
void action_bind_gamepad(const char *action, int button) {}
void action_bind_touch(const char *action, int gesture) {}
void action_bind_composite(const char *action, const char *composite) {}
float action_get_value(const char *action) { return 0.0f; }
bool action_is_triggered(const char *action) { return false; }
float action_get_axis(const char *action) { return 0.0f; }
void action_get_vector2(const char *action, float *x, float *y) {
  *x = 0;
  *y = 0;
}
void action_set_context(const char *context) {}
void action_context_push(const char *context) {}
void action_context_pop(void) {}
void action_context_activate(const char *context, bool active) {}
void action_serialize_bindings(const char *path) {}
void action_deserialize_bindings(const char *path) {}
void action_reset_to_defaults(void) {}

/* =================================================================================================
 *                                    INPUT MANAGER
 * =================================================================================================
 */

void input_manager_init(void) {
  keyboard_init();
  mouse_init();
}
void input_manager_shutdown(void) {}
void input_manager_update(void) {
  keyboard_update();
  mouse_update();
  gamepad_update();
}
void input_manager_poll_events(void) {}
void input_manager_consume_event(void) {}
void input_manager_dispatch_event(void *event) {}
void input_manager_register_listener(void *listener) {}
void input_manager_unregister_listener(void *listener) {}
void input_manager_set_enabled(bool enabled) {}
bool input_manager_is_any_input(void) { return false; }
int input_manager_get_last_device(void) { return 0; }
void input_manager_rebind_ui(bool visible) {}
