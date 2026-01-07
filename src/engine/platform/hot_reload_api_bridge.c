// Hot Reload API Bridge Implementation

#include "../include/core/hot_reload.h"
#include "../include/core/logger.h"
#include "../include/common.h"
#include "../include/platform/hot_reload_api_bridge.h"
#include <string.h>

// Global state
static bool g_enabled = true;
// Config state
static HotReloadConfig g_config = {
    .enable_code_hot_reload = true,
    .enable_asset_hot_reload = true,
    .watch_path = NULL
};
static HotReloadCallback g_callback = NULL;

// Forward declaration
static void internal_on_file_changed_adapter(const char *path, HotReloadEventType type);

void hotreload_set_enabled(bool enabled) {
  g_enabled = enabled;
  // No direct API to toggle at runtime for now, just update internal state
  g_config.enable_asset_hot_reload = enabled;
  LOG_INFO("Hot Reload: %s", enabled ? "Enabled" : "Disabled");
}

bool hotreload_is_enabled(void) { return g_enabled; }

void hotreload_watch_directory(const char *path) {
  if (!path)
    return;
  // The current simple hot reload system essentially watches one root or initializes a watcher
  // Mapping to init_watcher for now, though this might restart it
  hot_reload_init_watcher(path, internal_on_file_changed_adapter);
  LOG_INFO("Watching directory: %s", path);
}

void hotreload_unwatch_directory(const char *path) {
  if (!path)
    return;
  // Current API only supports shutting down the single watcher
  hot_reload_shutdown_watcher();
  LOG_INFO("Unwatching directory: %s", path);
}

// Stub for adapter
static void internal_on_file_changed_adapter(const char *path, HotReloadEventType type) {
    if (g_callback) {
        g_callback(path, "modified"); // Simplified mapping
    }
}

bool hotreload_is_directory_watched(const char *path) {
  // Stub
  return false; 
}

void hotreload_trigger_reload(void) {
  hot_reload_update();
  LOG_INFO("Triggered manual asset reload");
}

uint32_t hotreload_get_watched_count(void) {
  return 0; // Not supported by current API
}

uint32_t hotreload_get_watched_directories(char (*paths)[256],
                                           uint32_t max_count) {
  return 0;
}

void hotreload_register_callback(HotReloadCallback callback) {
  g_callback = callback;
}

// Internal callback from engine (simulated for now)
void internal_on_file_changed(const char *path, const char *event) {
  if (g_callback) {
    g_callback(path, event);
  }
}
