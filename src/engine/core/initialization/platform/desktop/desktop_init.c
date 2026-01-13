#include <core/initialization/public/engine_init.h>
#include <core/logger.h>

// Forward declarations
bool platform_desktop_create_window(const WindowConfig *config);
void platform_desktop_destroy_window(void);

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char *description) {
  LOG_ERROR("GLFW Error (%d): %s", error, description);
}

bool platform_init(const EngineConfig *config) {
  LOG_INFO("Initializing Desktop Platform...");

  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit()) {
    LOG_FATAL("Failed to initialize GLFW");
    return false;
  }

  if (!platform_desktop_create_window(&config->window)) {
    glfwTerminate();
    return false;
  }

  return true;
}

void platform_shutdown(void) {
  platform_desktop_destroy_window();
  glfwTerminate();
  LOG_INFO("Desktop Platform Shutdown");
}

#else
// Stub for non-desktop platforms if this file is compiled
bool platform_init(const EngineConfig *config) { return false; }
void platform_shutdown(void) {}
#endif
