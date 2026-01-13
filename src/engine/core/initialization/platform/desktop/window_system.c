#include <core/initialization/public/engine_init.h>
#include <core/logger.h>

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
#include <GLFW/glfw3.h>

static GLFWwindow *g_window = NULL;

bool platform_desktop_create_window(const WindowConfig *config) {
  if (!config)
    return false;

  LOG_INFO(LOG_CAT_PLATFORM, "Creating Window: %s (%dx%d)", config->title,
           config->width, config->height);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, config->resizable ? GLFW_TRUE : GLFW_FALSE);

  g_window = glfwCreateWindow(
      (int)config->width, (int)config->height, config->title,
      config->fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);

  if (!g_window) {
    LOG_FATAL(LOG_CAT_PLATFORM, "Failed to create GLFW window");
    return false;
  }

  return true;
}

void platform_desktop_destroy_window(void) {
  if (g_window) {
    glfwDestroyWindow(g_window);
    g_window = NULL;
  }
}

void *platform_desktop_get_window_handle(void) { return g_window; }

#else
bool platform_desktop_create_window(const WindowConfig *config) {
  return false;
}
void platform_desktop_destroy_window(void) {}
void *platform_desktop_get_window_handle(void) { return NULL; }
#endif
