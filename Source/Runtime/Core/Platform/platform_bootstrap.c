#include "platform_bootstrap.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

// Internal state
static GLFWwindow *g_window_handle = NULL;
static int g_window_width = 1024;
static int g_window_height = 768;

// Error callback for GLFW
static void error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

bool platform_init(int width, int height, const char *title) {
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return false;
  }

  g_window_width = width;
  g_window_height = height;

  // Set window hints
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // For Metal/Vulkan/etc
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

  g_window_handle = glfwCreateWindow(width, height, title, NULL, NULL);

  if (!g_window_handle) {
    const char *error_desc;
    glfwGetError(&error_desc);
    fprintf(stderr, "Failed to create window: %s\n",
            error_desc ? error_desc : "Unknown error");
    return false;
  }

  // Center window
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  if (monitor) {
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (mode) {
      int xpos = (mode->width - width) / 2;
      int ypos = (mode->height - height) / 2;
      glfwSetWindowPos(g_window_handle, xpos, ypos);
    }
  }

  return true;
}

void platform_shutdown(void) {
  if (g_window_handle) {
    glfwDestroyWindow(g_window_handle);
    g_window_handle = NULL;
  }
  glfwTerminate();
}

void platform_poll_events(void) { glfwPollEvents(); }

double platform_get_time(void) { return glfwGetTime(); }

bool platform_window_should_close(void) {
  return g_window_handle ? glfwWindowShouldClose(g_window_handle) : true;
}

void *platform_get_window_handle(void) { return (void *)g_window_handle; }

WindowSize platform_get_window_size(void) {
  WindowSize size = {0};
  if (g_window_handle) {
    glfwGetFramebufferSize(g_window_handle, &size.width, &size.height);
  } else {
    size.width = g_window_width;
    size.height = g_window_height;
  }
  return size;
}
