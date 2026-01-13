// Console Commands API Bridge
// Exposes console/command system to VoxelForgeStudio

#ifndef CONSOLE_API_BRIDGE_H
#define CONSOLE_API_BRIDGE_H

#include "engine/include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Console API
// ============================================================================

/// Execute a console command
/// @param command Command string to execute
void console_execute(const char *command);

/// Print message to console
void console_print(const char *message);

/// Clear console history
void console_clear(void);

/// Get console history count
uint32_t console_get_history_count(void);

/// Get history entry at index
const char *console_get_history_entry(uint32_t index);

/// Enable/disable console
void console_set_enabled(bool enabled);

/// Check if console is enabled
bool console_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // CONSOLE_API_BRIDGE_H
