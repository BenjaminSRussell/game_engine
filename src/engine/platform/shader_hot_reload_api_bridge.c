// Shader Hot Reload API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/shader_hot_reload_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static bool g_enabled = true;

void shader_hotreload_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_INFO("Shader Hot Reload: %s", enabled ? "Enabled" : "Disabled");
}

bool shader_hotreload_is_enabled(void) { return g_enabled; }

void shader_hotreload_trigger(void) {
  LOG_INFO("Triggering full shader reload");
  // In real implementation, reload all shaders
}

void shader_hotreload_shader(const char *shader_name) {
  if (!shader_name)
    return;
  LOG_INFO("Reloading shader: %s", shader_name);
  // In real implementation, reload specific shader
}

uint32_t shader_hotreload_get_watch_count(void) {
  return 0; // Mock: would return actual count
}
