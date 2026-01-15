#include "input_system.h"
#include "../../Core/Platform/platform_bootstrap.h"
#include <GLFW/glfw3.h> // Needed for key constants and raw input
#include <stdio.h>

void input_system_init(InputState *state) {
  // Basic initialization if needed, mostly handled by input_init from existing
  // code
}

void input_system_update(InputState *state, void *window_handle) {
  if (!state || !window_handle)
    return;

  GLFWwindow *window = (GLFWwindow *)window_handle;

  // Use platform polling
  platform_poll_events();

  // Handle keyboard input
  u32 key_limit = INPUT_KEY_COUNT;
#ifdef GLFW_KEY_LAST
  if (key_limit > (u32)GLFW_KEY_LAST + 1u) {
    key_limit = (u32)GLFW_KEY_LAST + 1u;
  }
#endif

  for (u32 key = 0; key < key_limit; key++) {
    int key_state = glfwGetKey(window, key);
    input_set_key(state, key,
                  key_state == GLFW_PRESS || key_state == GLFW_REPEAT);
  }

  // Handle mouse input
  double mouse_x, mouse_y;
  glfwGetCursorPos(window, &mouse_x, &mouse_y);
  input_set_mouse_position(state, (f32)mouse_x, (f32)mouse_y);

  static f32 last_mouse_x = 0, last_mouse_y = 0;
  static bool first_mouse = true;
  if (first_mouse) {
    last_mouse_x = (f32)mouse_x;
    last_mouse_y = (f32)mouse_y;
    first_mouse = false;
  }

  input_set_mouse_delta(state, (f32)mouse_x - last_mouse_x,
                        (f32)mouse_y - last_mouse_y);
  last_mouse_x = (f32)mouse_x;
  last_mouse_y = (f32)mouse_y;

  int mouse_left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  int mouse_right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
  input_set_mouse_button(state, 0, mouse_left == GLFW_PRESS);
  input_set_mouse_button(state, 1, mouse_right == GLFW_PRESS);

  // Gamepad handling
  GLFWgamepadstate gamepad_state = {0};
  if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) &&
      glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad_state)) {

    // Map axes
    // (Simplified mapping - full mapping logic from monolith could be copied
    // here)
    // ...
  }
}
