// Hot Reload API Bridge
// Exposes asset hot reloading to VoxelForgeStudio

#ifndef HOT_RELOAD_API_BRIDGE_H
#define HOT_RELOAD_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Hot Reload API
// ============================================================================

/// Enable/disable hot reloading
void hotreload_set_enabled(bool enabled);

/// Check if hot reloading is enabled
bool hotreload_is_enabled(void);

/// Watch a directory for changes
void hotreload_watch_directory(const char *path);

/// Unwatch a directory
void hotreload_unwatch_directory(const char *path);

/// Check if a directory is currently watched
bool hotreload_is_directory_watched(const char *path);

/// Trigger a manual reload of all assets
void hotreload_trigger_reload(void);

/// Get the number of watched directories
uint32_t hotreload_get_watched_count(void);

/// Get list of watched directories
/// @param paths Array of string buffers
/// @param max_count Maximum number of paths
/// @return Number of paths written
uint32_t hotreload_get_watched_directories(char (*paths)[256],
                                           uint32_t max_count);

/// Callback for when an asset changes
typedef void (*HotReloadCallback)(const char *asset_path,
                                  const char *event_type);

/// Register callback for file changes
void hotreload_register_callback(HotReloadCallback callback);

#ifdef __cplusplus
}
#endif

#endif // HOT_RELOAD_API_BRIDGE_H
