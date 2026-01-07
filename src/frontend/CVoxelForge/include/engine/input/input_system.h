/**
 * =================================================================================================
 *                              INPUT SYSTEM
 *                              Agent: AGENT_INPUT_1
 * =================================================================================================
 */

#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    KEYBOARD
 * =================================================================================================
 */

void keyboard_init(void);
void keyboard_shutdown(void);
void keyboard_update(void);
bool keyboard_is_key_pressed(uint32_t key);
bool keyboard_is_key_down(uint32_t key);
bool keyboard_is_key_up(uint32_t key);
bool keyboard_is_key_released(uint32_t key);
char keyboard_get_char_input(void);
uint32_t keyboard_get_modifiers(void);
bool keyboard_is_any_key_pressed(void);
uint32_t keyboard_get_last_key(void);
void keyboard_clear(void);

/* =================================================================================================
 *                                    MOUSE
 * =================================================================================================
 */

void mouse_init(void);
void mouse_shutdown(void);
void mouse_update(void);
void mouse_get_position(float *x, float *y);
void mouse_get_delta(float *x, float *y);
void mouse_get_scroll(float *x, float *y);
bool mouse_is_button_pressed(int button);
bool mouse_is_button_down(int button);
bool mouse_is_button_up(int button);
bool mouse_is_button_released(int button);
void mouse_set_position(float x, float y);
void mouse_set_cursor_visible(bool visible);
void mouse_set_cursor_locked(bool locked);
void mouse_set_cursor_shape(int shape);
void mouse_set_cursor_image(void *image, int hot_x, int hot_y);
bool mouse_is_inside_window(void);

/* =================================================================================================
 *                                    GAMEPAD
 * =================================================================================================
 */

void gamepad_init(void);
void gamepad_shutdown(void);
void gamepad_update(void);
void gamepad_enumerate(void);
bool gamepad_is_connected(int id);
const char *gamepad_get_name(int id);
bool gamepad_is_button_pressed(int id, int button);
bool gamepad_is_button_down(int id, int button);
bool gamepad_is_button_released(int id, int button);
float gamepad_get_axis(int id, int axis);
float gamepad_get_trigger(int id, int trigger);
void gamepad_apply_deadzone(int id, float deadzone);
void gamepad_set_vibration(int id, float left, float right);
void gamepad_stop_vibration(int id);
void gamepad_on_connected(int id);
void gamepad_on_disconnected(int id);
void gamepad_remap_buttons(int id, const int *mapping);
void gamepad_save_mapping(int id, const char *path);
void gamepad_load_mapping(int id, const char *path);

/* =================================================================================================
 *                                    TOUCH
 * =================================================================================================
 */

void touch_init(void);
void touch_shutdown(void);
void touch_update(void);
int touch_get_count(void);
void touch_get_position(int index, float *x, float *y);
void touch_get_delta(int index, float *x, float *y);
float touch_get_pressure(int index);
bool touch_is_began(int index);
bool touch_is_moved(int index);
bool touch_is_ended(int index);
bool touch_gesture_tap(void);
bool touch_gesture_double_tap(void);
bool touch_gesture_long_press(void);
bool touch_gesture_swipe(int dir);
bool touch_gesture_pinch(float *scale);
bool touch_gesture_rotate(float *rotation);
bool touch_gesture_pan(float *x, float *y);

/* =================================================================================================
 *                                    INPUT ACTIONS
 * =================================================================================================
 */

void action_register(const char *name);
void action_unregister(const char *name);
void action_bind_key(const char *action, uint32_t key);
void action_bind_mouse(const char *action, int button);
void action_bind_gamepad(const char *action, int button);
void action_bind_touch(const char *action, int gesture);
void action_bind_composite(const char *action, const char *composite);
float action_get_value(const char *action);
bool action_is_triggered(const char *action);
float action_get_axis(const char *action);
void action_get_vector2(const char *action, float *x, float *y);
void action_set_context(const char *context);
void action_context_push(const char *context);
void action_context_pop(void);
void action_context_activate(const char *context, bool active);
void action_serialize_bindings(const char *path);
void action_deserialize_bindings(const char *path);
void action_reset_to_defaults(void);

/* =================================================================================================
 *                                    INPUT MANAGER
 * =================================================================================================
 */

void input_manager_init(void);
void input_manager_shutdown(void);
void input_manager_update(void);
void input_manager_poll_events(void);
void input_manager_consume_event(void);
void input_manager_dispatch_event(void *event);
void input_manager_register_listener(void *listener);
void input_manager_unregister_listener(void *listener);
void input_manager_set_enabled(bool enabled);
bool input_manager_is_any_input(void);
int input_manager_get_last_device(void);
void input_manager_rebind_ui(bool visible);

#endif // INPUT_SYSTEM_H
