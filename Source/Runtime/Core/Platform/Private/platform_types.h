#pragma once

#include "unified_platform.h"
// #include <GLFW/glfw3.h> // TODO: Add proper GLFW detection

// Forward declare for now
typedef struct GLFWwindow GLFWwindow;

// Internal Context Definition
struct PlatformContext {
  PlatformSubsystem initialized_subsystems;
  // Track windows, but simple handle for now
  PlatformWindow *main_window;
  bool is_hosting;
};

// Internal Window Definition
struct PlatformWindow {
  GLFWwindow *glfw_handle;
  void *native_handle; // For hosted mode (e.g. SwiftUI/QT parent)
  bool is_hosted;

  u32 width;
  u32 height;
  char *title;
  bool fullscreen;
  bool vsync;
};
