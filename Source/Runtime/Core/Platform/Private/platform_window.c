#include "../Public/unified_platform.h"
#include "platform_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <GLFW/glfw3.h>
#include <string.h>

// Global state tracking for GLFW
static int g_window_count = 0;

PlatformContext *platform_init(PlatformSubsystem subsystems) {
  PlatformContext *ctx =
      (PlatformContext *)UNIFIED_ALLOC(sizeof(PlatformContext));
  if (!ctx) {
    LOG_ERROR(LOG_CAT_SYSTEM, "Failed to allocate platform context");
    return NULL;
  }
  memset(ctx, 0, sizeof(PlatformContext));

  if (subsystems & PLATFORM_SUBSYSTEM_WINDOW) {
    if (!glfwInit()) {
      LOG_ERROR(LOG_CAT_SYSTEM, "Failed to initialize GLFW");
      UNIFIED_FREE(ctx);
      return NULL;
    }
    ctx->initialized_subsystems |= PLATFORM_SUBSYSTEM_WINDOW;
  }

  // Input init handled via window creation mostly in GLFW,
  // or separate input system init if needed.

  LOG_INFO(LOG_CAT_SYSTEM, "Platform system initialized");
  return ctx;
}

void platform_shutdown(PlatformContext *ctx) {
  if (!ctx)
    return;

  if (ctx->initialized_subsystems & PLATFORM_SUBSYSTEM_WINDOW) {
    glfwTerminate();
  }

  UNIFIED_FREE(ctx);
  LOG_INFO(LOG_CAT_SYSTEM, "Platform system shutdown");
}

void platform_update(PlatformContext *ctx) {
  if (!ctx)
    return;

  // Poll events for window system
  if (ctx->initialized_subsystems & PLATFORM_SUBSYSTEM_WINDOW) {
    glfwPollEvents();
  }
}

// =================================================================================================
// WINDOW IMPLEMENTATION
// =================================================================================================

PlatformWindow *platform_window_create(PlatformContext *ctx,
                                       const WindowConfig *config) {
  if (!ctx || !config)
    return NULL;

  PlatformWindow *window =
      (PlatformWindow *)UNIFIED_ALLOC(sizeof(PlatformWindow));
  if (!window)
    return NULL;
  memset(window, 0, sizeof(PlatformWindow));

  window->width = config->width;
  window->height = config->height;
  window->vsync = config->vsync;
  window->fullscreen = config->fullscreen;

  // Deep copy title
  if (config->title) {
    size_t len = strlen(config->title);
    window->title = (char *)UNIFIED_ALLOC(len + 1);
    strcpy(window->title, config->title);
  } else {
    window->title = NULL; // Safe default
  }

  // Hosted Mode check
  if (config->native_handle) {
    window->is_hosted = true;
    window->native_handle = config->native_handle;
    // No GLFW creation for hosted mode usually, or specific setup
    LOG_INFO(LOG_CAT_SYSTEM, "Created hosted window");
  } else {
    // Standard GLFW Window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor *monitor = config->fullscreen ? glfwGetPrimaryMonitor() : NULL;
    window->glfw_handle = glfwCreateWindow(
        config->width, config->height, config->title ? config->title : "Engine",
        monitor, NULL);

    if (!window->glfw_handle) {
      LOG_ERROR(LOG_CAT_SYSTEM, "Failed to create GLFW window");
      if (window->title)
        UNIFIED_FREE(window->title);
      UNIFIED_FREE(window);
      return NULL;
    }

    glfwMakeContextCurrent(window->glfw_handle);
    glfwSwapInterval(config->vsync ? 1 : 0);

    // Store pointer for callbacks
    glfwSetWindowUserPointer(window->glfw_handle, window);

    g_window_count++;
  }

  // Track as main window if first one
  if (!ctx->main_window) {
    ctx->main_window = window;
  }

  return window;
}

void platform_window_destroy(PlatformWindow *window) {
  if (!window)
    return;

  if (!window->is_hosted && window->glfw_handle) {
    glfwDestroyWindow(window->glfw_handle);
    g_window_count--;
  }

  if (window->title) {
    UNIFIED_FREE(window->title);
  }

  UNIFIED_FREE(window);
}

bool platform_window_should_close(PlatformWindow *window) {
  if (!window)
    return true;
  if (window->is_hosted)
    return false; // Host controls lifecycle
  return glfwWindowShouldClose(window->glfw_handle);
}

void platform_window_poll_events(void) { glfwPollEvents(); }

void platform_window_swap_buffers(PlatformWindow *window) {
  if (window && !window->is_hosted && window->glfw_handle) {
    glfwSwapBuffers(window->glfw_handle);
  }
}

void *platform_window_get_native_handle(PlatformWindow *window) {
  if (!window)
    return NULL;
  // If hosted, return the handle we were given.
  // If GLFW, on Windows this might be HWND, on Mac NSWindow.
  // requires defining GLFW_EXPOSE_NATIVE_XX macros before include.
  // For now returning the hosted handle or NULL if not hosted/implemented.
  return window->native_handle;
}

void platform_window_get_size(PlatformWindow *window, u32 *width, u32 *height) {
  if (!window)
    return;

  if (window->is_hosted) {
    if (width)
      *width = window->width;
    if (height)
      *height = window->height;
  } else if (window->glfw_handle) {
    int w, h;
    glfwGetWindowSize(window->glfw_handle, &w, &h);
    if (width)
      *width = (u32)w;
    if (height)
      *height = (u32)h;
    // Update cached values
    window->width = (u32)w;
    window->height = (u32)h;
  }
}

void platform_window_set_title(PlatformWindow *window, const char *title) {
  if (!window || !title)
    return;

  // Update stored title
  if (window->title)
    UNIFIED_FREE(window->title);
  size_t len = strlen(title);
  window->title = (char *)UNIFIED_ALLOC(len + 1);
  strcpy(window->title, title);

  if (!window->is_hosted && window->glfw_handle) {
    glfwSetWindowTitle(window->glfw_handle, title);
  }
}

void platform_window_set_fullscreen(PlatformWindow *window, bool fullscreen) {
  if (!window)
    return;
  // Implementation requires monitor logic, simplified for now
  window->fullscreen = fullscreen;
  // TODO: Implement actual GLFW switch
  LOG_WARN(LOG_CAT_SYSTEM, "Fullscreen switching not fully implemented yet");
}
