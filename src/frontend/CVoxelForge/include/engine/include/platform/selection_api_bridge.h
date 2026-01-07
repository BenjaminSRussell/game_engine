// Selection API Bridge
// Exposes editor selection system to VoxelForgeStudio

#ifndef SELECTION_API_BRIDGE_H
#define SELECTION_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Selection System API
// ============================================================================

/// Select an entity
/// @param entity_id Entity to select
/// @param additive If true, add to current selection. If false, replace
/// selection.
void selection_select(uint64_t entity_id, bool additive);

/// Deselect an entity
/// @param entity_id Entity to deselect
void selection_deselect(uint64_t entity_id);

/// Clear all selection
void selection_clear(void);

/// Check if an entity is selected
bool selection_is_selected(uint64_t entity_id);

/// Get number of selected entities
uint32_t selection_get_count(void);

/// Get list of selected entities
/// @param entities Output array
/// @param max_count Maximum number of entities to write
/// @return Number of entities written
uint32_t selection_get_selected(uint64_t *entities, uint32_t max_count);

/// Get primary selected entity (first or last selected)
uint64_t selection_get_primary(void);

/// Perform a raycast selection
/// @param ray_origin_x X coordinate of ray origin
/// @param ray_origin_y Y coordinate of ray origin
/// @param ray_origin_z Z coordinate of ray origin
/// @param ray_dir_x X coordinate of ray direction
/// @param ray_dir_y Y coordinate of ray direction
/// @param ray_dir_z Z coordinate of ray direction
/// @return ID of hit entity (or 0 if none)
uint64_t selection_raycast(float ray_origin_x, float ray_origin_y,
                           float ray_origin_z, float ray_dir_x, float ray_dir_y,
                           float ray_dir_z);

// ============================================================================
// Events
// ============================================================================

/// Selection changed callback
typedef void (*SelectionChangedCallback)(uint32_t count);

/// Register selection change callback
void selection_register_callback(SelectionChangedCallback callback);

#ifdef __cplusplus
}
#endif

#endif // SELECTION_API_BRIDGE_H
