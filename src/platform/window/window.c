#include <GLFW/glfw3.h>
#include "engine/include/core/logger.h"
#include <core/window.h>
#include <stdlib.h>

// Global window count to manage GLFW init/terminate
static int g_window_count = 0;

bool window_init(Window *window, u32 width, u32 height, const char *title,
                 bool fullscreen) {
  if (!window)
    return false;

  if (g_window_count == 0) {
    if (!glfwInit()) {
      LOG_ERROR("Failed to initialize GLFW");
      return false;
    }
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWmonitor *monitor = fullscreen ? glfwGetPrimaryMonitor() : NULL;
  window->handle = glfwCreateWindow(width, height, title, monitor, NULL);

  if (!window->handle) {
    LOG_ERROR("Failed to create GLFW window");
    if (g_window_count == 0)
      glfwTerminate();
    return false;
  }

  window->width = width;
  window->height = height;
  window->title = title;
  window->fullscreen = fullscreen;
  window->vsync = true;     // Default
  window->resizable = true; // Default
  window->user_data = NULL;

  glfwMakeContextCurrent(window->handle);
  glfwSetWindowUserPointer(window->handle, window);

  g_window_count++;
  return true;
}

bool window_init_hosted(Window *window, void *native_handle, u32 width, u32 height) {
    if (!window) return false;
    
    // No GLFW init needed for hosted
    window->handle = NULL; // No GLFW handle
    window->native_handle = native_handle;
    window->is_hosted = true;
    window->width = width;
    window->height = height;
    window->title = "Hosted";
    window->fullscreen = false;
    window->vsync = true;
    window->resizable = true;
    window->user_data = NULL;
    
    return true;
}

void window_shutdown(Window *window) {
  if (window) {
      if (window->is_hosted) {
          // Nothing to destroy for host, mostly
          window->native_handle = NULL;
          return;
      }
      
      if (window->handle) {
        glfwDestroyWindow(window->handle);
        window->handle = NULL;
        g_window_count--;

        if (g_window_count <= 0) {
          glfwTerminate();
        }
      }
  }
}

bool window_should_close(const Window *window) {
  if (window && window->is_hosted) return false; // Host controls lifecycle
  return window && window->handle ? glfwWindowShouldClose(window->handle)
                                  : true;
}

void window_poll_events(void) { 
    // If we have mixed windows (some hosted, some not), we still poll
    if (g_window_count > 0) {
        glfwPollEvents(); 
    }
}

void window_swap_buffers(Window *window) {
  if (window && !window->is_hosted && window->handle) {
    glfwSwapBuffers(window->handle);
  }
  // Hosted usually handles swap externally or via Metal
}

void window_set_title(Window *window, const char *title) {
  if (window && !window->is_hosted && window->handle) {
    glfwSetWindowTitle(window->handle, title);
    window->title = title;
  }
}

void window_set_size(Window *window, u32 width, u32 height) {
  if (window) {
      window->width = width;
      window->height = height;
      if (!window->is_hosted && window->handle) {
        glfwSetWindowSize(window->handle, width, height);
      }
  }
}

void window_get_size(const Window *window, u32 *width, u32 *height) {
  if (window) {
      if (window->is_hosted) {
          if (width) *width = window->width;
          if (height) *height = window->height;
          return;
      }
      
      if (window->handle) {
        int w, h;
        glfwGetWindowSize(window->handle, &w, &h);
        if (width)
          *width = (u32)w;
        if (height)
          *height = (u32)h;
      }
  }
}

void window_set_fullscreen(Window *window, bool fullscreen) {
  // Implementation omitted for brevity/time constraints relative to current
  // errors Would require getting monitor, setting window monitor, etc.
  window->fullscreen = fullscreen;
}

void window_set_vsync(Window *window, bool vsync) {
  if (window) {
      window->vsync = vsync;
      if (!window->is_hosted && window->handle) {
        glfwSwapInterval(vsync ? 1 : 0);
      }
  }
}

void window_set_user_data(Window *window, void *data) {
  if (window) {
    window->user_data = data;
  }
}

void *window_get_user_data(const Window *window) {
  return window ? window->user_data : NULL;
}

void *window_create_vulkan_surface(Window *window, void *instance) {
  // Stub for now, return NULL or implement if Vulkan is available
  return NULL;
}

void window_set_resize_callback(Window *window, WindowResizeCallback callback) {
  // Stub
}

void window_set_close_callback(Window *window, WindowCloseCallback callback) {
  // Stub
}

void window_set_focus_callback(Window *window, WindowFocusCallback callback) {
  // Stub
}
