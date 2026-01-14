// Shader Hot Reload API Bridge
// Exposes shader reloading to VoxelForgeStudio

#ifndef SHADER_HOT_RELOAD_API_BRIDGE_H
#define SHADER_HOT_RELOAD_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Shader Hot Reload API
// ============================================================================

/// Enable/disable shader hot reload watching
void shader_hotreload_set_enabled(bool enabled);

/// Check if enabled
bool shader_hotreload_is_enabled(void);

/// Manually trigger a reload of all shaders
void shader_hotreload_trigger(void);

/// Reload a specific shader by name
void shader_hotreload_shader(const char *shader_name);

/// Get number of shader files being watched
uint32_t shader_hotreload_get_watch_count(void);

#ifdef __cplusplus
}
#endif

#endif // SHADER_HOT_RELOAD_API_BRIDGE_H
