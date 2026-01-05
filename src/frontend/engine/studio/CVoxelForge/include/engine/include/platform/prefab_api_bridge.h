// Prefab API Bridge
// Exposes prefab system to VoxelForgeStudio

#ifndef PREFAB_API_BRIDGE_H
#define PREFAB_API_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Prefab System API
// ============================================================================

/// Create a prefab from an entity
/// @param entity_id The ID of the entity to create prefab from
/// @param path Path to save the prefab asset
/// @return true if successful
bool prefab_create_from_entity(uint64_t entity_id, const char *path);

/// Instantiate a prefab into the scene
/// @param path Path to the prefab asset
/// @param position_x X coordinate
/// @param position_y Y coordinate
/// @param position_z Z coordinate
/// @return ID of the instantiated entity (or 0 if failed)
uint64_t prefab_instantiate(const char *path, float position_x,
                            float position_y, float position_z);

/// Save changes to a prefab
/// @param prefab_id ID of the prefab asset (if loaded) or entity representing
/// it? Typically we apply changes from an instance back to the prefab asset.
/// @param instance_id The instance with changes
/// @return true if successful
bool prefab_apply_changes(const char *path, uint64_t instance_id);

/// Revert an instance to match its prefab
/// @param instance_id The instance to revert
/// @return true if successful
bool prefab_revert_instance(uint64_t instance_id);

/// Check if an entity is a prefab instance
bool prefab_is_instance(uint64_t entity_id);

/// Get the path of the prefab source for an instance
/// @param entity_id The instance ID
/// @return Path string (must be copied) or NULL
const char *prefab_get_source_path(uint64_t entity_id);

/// Break prefab connection (unpack)
/// @param entity_id The instance to unpack
bool prefab_unpack(uint64_t entity_id);

#ifdef __cplusplus
}
#endif

#endif // PREFAB_API_BRIDGE_H
