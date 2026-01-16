#include "Public/Input.h"
#include <stdbool.h>
#include <string.h>

// Simplified Input System
#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8

typedef struct InputState {
  bool keys_current[MAX_KEYS];
  bool keys_previous[MAX_KEYS];
  bool mouse_buttons_current[MAX_MOUSE_BUTTONS];
  bool mouse_buttons_previous[MAX_MOUSE_BUTTONS];
  float mouse_x;
  float mouse_y;
  bool initialized;
} InputState;

static InputState g_input_state = {0};

// Public API Implementation
bool Input_Init(void) {
  if (g_input_state.initialized) {
    return true;
  }

  memset(&g_input_state, 0, sizeof(InputState));
  g_input_state.initialized = true;

  return true;
}

void Input_Update(float delta_time) {
  (void)delta_time;

  if (!g_input_state.initialized)
    return;

  // Copy current state to previous
  memcpy(g_input_state.keys_previous, g_input_state.keys_current,
         sizeof(g_input_state.keys_current));
  memcpy(g_input_state.mouse_buttons_previous,
         g_input_state.mouse_buttons_current,
         sizeof(g_input_state.mouse_buttons_current));
}

bool Input_IsKeyPressed(int key_code) {
  if (key_code < 0 || key_code >= MAX_KEYS)
    return false;
  return g_input_state.keys_current[key_code] &&
         !g_input_state.keys_previous[key_code];
}

bool Input_IsKeyDown(int key_code) {
  if (key_code < 0 || key_code >= MAX_KEYS)
    return false;
  return g_input_state.keys_current[key_code];
}

bool Input_IsKeyReleased(int key_code) {
  if (key_code < 0 || key_code >= MAX_KEYS)
    return false;
  return !g_input_state.keys_current[key_code] &&
         g_input_state.keys_previous[key_code];
}

bool Input_IsMouseButtonPressed(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return g_input_state.mouse_buttons_current[button] &&
         !g_input_state.mouse_buttons_previous[button];
}

void Input_GetMousePosition(float *x, float *y) {
  if (x)
    *x = g_input_state.mouse_x;
  if (y)
    *y = g_input_state.mouse_y;
}

void Input_OnKey(int key, bool pressed) {
  if (key >= 0 && key < MAX_KEYS) {
    g_input_state.keys_current[key] = pressed;
  }
}

void Input_OnMouseButton(int button, bool pressed) {
  if (button >= 0 && button < MAX_MOUSE_BUTTONS) {
    g_input_state.mouse_buttons_current[button] = pressed;
  }
}

void Input_OnMouseMove(float x, float y) {
  g_input_state.mouse_x = x;
  g_input_state.mouse_y = y;
}

void Input_Shutdown(void) { g_input_state.initialized = false; }
