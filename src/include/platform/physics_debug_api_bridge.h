// Physics Debug API Bridge
// Exposes physics debug visualization to VoxelForgeStudio

#ifndef PHYSICS_DEBUG_API_BRIDGE_H
#define PHYSICS_DEBUG_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Physics Debug Visualization API
// ============================================================================

/// Enable/disable all physics debug rendering
void physics_debug_set_enabled(bool enabled);

/// Check if physics debug rendering is enabled
bool physics_debug_is_enabled(void);

/// Show contact points between colliding objects
void physics_debug_show_contacts(bool enabled);

/// Show constraint connections (joints, etc.)
void physics_debug_show_constraints(bool enabled);

/// Show axis-aligned bounding boxes (AABBs)
void physics_debug_show_aabbs(bool enabled);

/// Show oriented bounding boxes (OBBs)
void physics_debug_show_obbs(bool enabled);

/// Show collision shapes (wireframe)
void physics_debug_show_shapes(bool enabled);

/// Show velocity vectors
void physics_debug_show_velocities(bool enabled);

/// Show force vectors
void physics_debug_show_forces(bool enabled);

/// Show center of mass markers
void physics_debug_show_center_of_mass(bool enabled);

/// Set color for contact point rendering (RGB 0-1)
void physics_debug_set_contact_color(float r, float g, float b);

/// Set color for constraint rendering (RGB 0-1)
void physics_debug_set_constraint_color(float r, float g, float b);

/// Set color for AABB rendering (RGB 0-1)
void physics_debug_set_aabb_color(float r, float g, float b);

/// Set color for velocity vector rendering (RGB 0-1)
void physics_debug_set_velocity_color(float r, float g, float b);

/// Set size of contact point markers
void physics_debug_set_contact_size(float size);

/// Set scale of velocity vectors
void physics_debug_set_velocity_scale(float scale);

/// Set scale of force vectors
void physics_debug_set_force_scale(float scale);

/// Get current debug statistics
typedef struct {
  uint32_t active_bodies;
  uint32_t sleeping_bodies;
  uint32_t contact_count;
  uint32_t constraint_count;
  uint32_t island_count;
} PhysicsDebugStats;

void physics_debug_get_stats(PhysicsDebugStats *stats);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_DEBUG_API_BRIDGE_H
