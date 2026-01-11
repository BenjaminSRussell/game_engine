// Destruction System API Bridge
// Exposes mesh fracturing and debris physics to VoxelForgeStudio

#ifndef DESTRUCTION_API_BRIDGE_H
#define DESTRUCTION_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Destruction System API
// ============================================================================

/// Create a fractured version of a mesh
/// @param source_mesh_id Original mesh to fracture
/// @param fragment_count Number of fragments to create
/// @return Fractured mesh ID
uint64_t destruction_create_fractured_mesh(uint64_t source_mesh_id,
                                           uint32_t fragment_count);

/// Trigger destruction on an entity
/// @param entity_id Entity to destroy
/// @param impact_point Impact location
/// @param force Explosion force
void destruction_trigger_break(uint64_t entity_id, float impact_x,
                               float impact_y, float impact_z, float force);

/// Set debris lifetime in seconds
void destruction_set_debris_lifetime(float seconds);

/// Get debris lifetime
float destruction_get_debris_lifetime(void);

/// Enable/disable destruction system
void destruction_set_enabled(bool enabled);

/// Check if enabled
bool destruction_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // DESTRUCTION_API_BRIDGE_H
