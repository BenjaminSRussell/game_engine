#ifndef PLATFORM_BOOTSTRAP_H
#define PLATFORM_BOOTSTRAP_H

#include <stdbool.h>
#include <stdint.h>

// Platform-specific window types
typedef void *GLFWwindow;

// Gamepad state structure
typedef struct {
    float axes[6];
    unsigned char buttons[15];
    int button_count;
    int axis_count;
} GLFWgamepadstate;

// GLFW key codes
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2
#define GLFW_MOUSE_BUTTON_LEFT 0
#define GLFW_MOUSE_BUTTON_RIGHT 1
#define GLFW_JOYSTICK_1 0
#define GLFW_KEY_LAST 348

// Gamepad buttons
#define GLFW_GAMEPAD_BUTTON_A 0
#define GLFW_GAMEPAD_BUTTON_B 1
#define GLFW_GAMEPAD_BUTTON_X 2
#define GLFW_GAMEPAD_BUTTON_Y 3
#define GLFW_GAMEPAD_BUTTON_LEFT_BUMPER 4
#define GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER 5
#define GLFW_GAMEPAD_BUTTON_BACK 6
#define GLFW_GAMEPAD_BUTTON_START 7
#define GLFW_GAMEPAD_BUTTON_GUIDE 8
#define GLFW_GAMEPAD_BUTTON_LEFT_THUMB 9
#define GLFW_GAMEPAD_BUTTON_RIGHT_THUMB 10
#define GLFW_GAMEPAD_BUTTON_DPAD_UP 11
#define GLFW_GAMEPAD_BUTTON_DPAD_RIGHT 12
#define GLFW_GAMEPAD_BUTTON_DPAD_DOWN 13
#define GLFW_GAMEPAD_BUTTON_DPAD_LEFT 14
#define GLFW_GAMEPAD_BUTTON_LAST GLFW_GAMEPAD_BUTTON_DPAD_LEFT

// Gamepad axes
#define GLFW_GAMEPAD_AXIS_LEFT_X 0
#define GLFW_GAMEPAD_AXIS_LEFT_Y 1
#define GLFW_GAMEPAD_AXIS_RIGHT_X 2
#define GLFW_GAMEPAD_AXIS_RIGHT_Y 3
#define GLFW_GAMEPAD_AXIS_LEFT_TRIGGER 4
#define GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER 5
#define GLFW_GAMEPAD_AXIS_LAST GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER

// Platform initialization/shutdown
int platform_init(void);
void platform_shutdown(void);

// Timing
double platform_get_time_seconds(void);

// Window management
GLFWwindow *platform_create_window(int width, int height, const char *title,
                                   void *monitor, GLFWwindow *share);
int platform_window_should_close(GLFWwindow *window);
void platform_close_window(GLFWwindow *window);
void platform_poll_events(void);
void platform_destroy_window(GLFWwindow *window);

// Keyboard input
int platform_get_key(GLFWwindow *window, int key);

// Mouse input
int platform_get_mouse_button(GLFWwindow *window, int button);
void platform_get_cursor_position(GLFWwindow *window, double *x, double *y);

// Gamepad input
int platform_is_joystick_gamepad(int joystick);
int platform_get_gamepad_state(int joystick, GLFWgamepadstate *state);

#endif // PLATFORM_BOOTSTRAP_H
