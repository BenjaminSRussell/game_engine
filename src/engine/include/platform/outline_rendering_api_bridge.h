// Outline Rendering API Bridge
// Exposes object outline rendering to VoxelForgeStudio

#ifndef OUTLINE_RENDERING_API_BRIDGE_H
#define OUTLINE_RENDERING_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Outline Rendering API
// ============================================================================

/// Enable/disable outline rendering
void outline_set_enabled(bool enabled);

/// Check if outline rendering is enabled
bool outline_is_enabled(void);

/// Set outline color
void outline_set_color(float r, float g, float b, float a);

/// Set outline width (in pixels)
void outline_set_width(float width);

/// Get outline width
float outline_get_width(void);

/// Set whether to outline only selected entities or specific ones
/// @param selection_only If true, only currently selected entities are outlined
void outline_set_selection_only(bool selection_only);

/// Add a specific entity to be highlighted (if not in selection only mode)
void outline_add_entity(uint64_t entity_id);

/// Remove a specific entity from highlighting
void outline_remove_entity(uint64_t entity_id);

/// Clear all specific highlighted entities
void outline_clear_entities(void);

#ifdef __cplusplus
}
#endif

#endif // OUTLINE_RENDERING_API_BRIDGE_H
