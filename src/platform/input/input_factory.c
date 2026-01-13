#include <platform/input/controls.h>
#include <platform/input/input.h>
#include <stdlib.h>
#include <string.h>

// Wrapper to map InputSystem calls to InputState calls if needed
// For now, providing a minimal implementation to satisfy the linker and engine
// lifecycle

typedef struct {
  InputState state;
} GLFWInputData;

static bool glfw_input_init(InputSystem *sys, const InputConfig *config) {
  if (!sys || !sys->platform_data)
    return false;
  GLFWInputData *data = (GLFWInputData *)sys->platform_data;
  input_init(&data->state);
  (void)config;
  return true;
}

static void glfw_input_shutdown(InputSystem *sys) {
  if (sys && sys->platform_data) {
    // Platform specific shutdown if needed
  }
}

static void glfw_input_update(InputSystem *sys, f32 delta_time) {
  if (!sys || !sys->platform_data)
    return;
  GLFWInputData *data = (GLFWInputData *)sys->platform_data;
  input_update(&data->state);
  (void)delta_time;
}

// Stub for polling
static void glfw_input_poll_events(InputSystem *sys) { (void)sys; }

InputSystem *create_glfw_input_system(void) {
  InputSystem *sys = (InputSystem *)calloc(1, sizeof(InputSystem));
  if (!sys)
    return NULL;

  sys->platform_data = calloc(1, sizeof(GLFWInputData));
  if (!sys->platform_data) {
    free(sys);
    return NULL;
  }

  sys->init = glfw_input_init;
  sys->shutdown = glfw_input_shutdown;
  sys->update = glfw_input_update;
  sys->poll_events = glfw_input_poll_events;

  return sys;
}

// ----------------------------------------------------------------------------
// Host Input Implementation (External Injection)
// ----------------------------------------------------------------------------

typedef struct {
  InputState state;
} HostInputData;

static bool host_input_init(InputSystem *sys, const InputConfig *config) {
  if (!sys || !sys->platform_data) return false;
  HostInputData *data = (HostInputData *)sys->platform_data;
  input_init(&data->state);
  return true;
}

static void host_input_shutdown(InputSystem *sys) {
  // Nothing to cleanup for host default
}

static void host_input_update(InputSystem *sys, f32 delta_time) {
  if (!sys || !sys->platform_data) return;
  HostInputData *data = (HostInputData *)sys->platform_data;
  input_update(&data->state);
}

static void host_input_poll_events(InputSystem *sys) {
    // Events are injected externally, no polling needed
}

InputSystem *create_host_input_system(void) {
  InputSystem *sys = (InputSystem *)calloc(1, sizeof(InputSystem));
  if (!sys) return NULL;

  sys->platform_data = calloc(1, sizeof(HostInputData));
  if (!sys->platform_data) {
    free(sys);
    return NULL;
  }

  sys->init = host_input_init;
  sys->shutdown = host_input_shutdown;
  sys->update = host_input_update;
  sys->poll_events = host_input_poll_events;
  
  return sys;
}

InputConfig input_create_default_config(void) {
  InputConfig config = {0};
  config.enable_mouse = true;
  config.enable_keyboard = true;
  config.enable_gamepad = true;
  config.mouse_sensitivity = 1.0f;
  config.gamepad_deadzone = 0.1f;
  return config;
}
