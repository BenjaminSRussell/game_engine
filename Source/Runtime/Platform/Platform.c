#include "Public/Platform.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

static GLFWwindow *g_window = NULL;
static bool g_requested_exit = false;

// We use GLFW for cross-platform windowing.
// Implementing proper OpenGL 4.1 Core Profile for macOS.

bool Platform_Init(void) {
  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    return false;
  }

  // OpenGL 2.1 Compatibility (Legacy Mode for Immediate Debug Draw)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Disabled
  // for legacy glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Disabled
  // for legacy

  // Create Window
  g_window =
      glfwCreateWindow(1280, 720, "Ultimate Engine (Phase 13+5)", NULL, NULL);
  if (!g_window) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    return false;
  }

  // Make Context Current
  glfwMakeContextCurrent(g_window);

  // Verify Context
  if (glfwGetCurrentContext() == NULL) {
    printf("Failed to make OpenGL context current\n");
    glfwDestroyWindow(g_window);
    glfwTerminate();
    return false;
  }

  glfwSwapInterval(1); // VSync
  return true;
}

void Platform_Update(float delta_time) {
  (void)delta_time;
  if (!g_window)
    return;

  glfwPollEvents();
  if (glfwWindowShouldClose(g_window)) {
    g_requested_exit = true;
  }

  // Swap Buffers
  glfwSwapBuffers(g_window);
}

void Platform_Shutdown(void) {
  if (g_window) {
    glfwDestroyWindow(g_window);
    g_window = NULL;
  }
  glfwTerminate();
}

bool Platform_RequestedExit(void) { return g_requested_exit; }
