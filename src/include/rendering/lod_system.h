// include/render/lod_system.h
//
// Purpose: Defines the Level of Detail (LOD) system for dynamically adjusting
// mesh complexity based on distance from camera. This system manages multiple
// LOD levels per chunk/object, automatically selecting the appropriate level
// based on view distance to optimize rendering performance.
//
// Public APIs:
// - `lod_system_init`: Initialize the LOD system with configuration
// - `lod_system_cleanup`: Free all LOD resources
// - `lod_system_register_mesh`: Register a mesh with multiple LOD levels
// - `lod_system_get_lod_level`: Get appropriate LOD level for a position
// - `lod_system_update`: Update LOD levels based on camera position
// - `lod_system_generate_lod`: Generate simplified mesh for given LOD level
//
// Ownership: The LOD system manages its own mesh copies and LOD metadata.
// Original meshes remain owned by the chunk/mesh system.
//
// Invariants:
// - LOD levels range from 0 (highest detail) to MAX_LOD_LEVELS-1 (lowest
// detail)
// - Distance thresholds must be monotonically increasing
// - Meshes must be registered before use
//
#ifndef LOD_SYSTEM_H
#define LOD_SYSTEM_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <include/rendering/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of LOD levels supported
#define MAX_LOD_LEVELS 4

// LOD level definitions
typedef enum {
  LOD_LEVEL_HIGH = 0,   // Full detail, close range
  LOD_LEVEL_MEDIUM = 1, // Medium detail, medium range
  LOD_LEVEL_LOW = 2,    // Low detail, far range
  LOD_LEVEL_MINIMAL = 3 // Minimal detail, very far range
} LODLevel;

// LOD configuration
typedef struct {
  f32 distances[MAX_LOD_LEVELS]; // Distance thresholds for each LOD level
  f32 hysteresis;                // Distance hysteresis to prevent LOD thrashing
  bool use_geometric_error;      // Use geometric error metric for LOD selection
  f32 target_error;              // Target geometric error in pixels
} LODConfig;

// LOD mesh entry
typedef struct {
  Mesh mesh;
  f32 geometric_error; // Maximum geometric error at this LOD level
  u32 triangle_count;
  bool is_generated;
} LODMeshEntry;

// LOD object (chunk or entity)
typedef struct {
  u64 id;                              // Unique identifier
  Vec3 position;                       // World position
  LODMeshEntry levels[MAX_LOD_LEVELS]; // LOD levels
  LODLevel current_level;              // Currently active LOD level
  f32 last_distance;                   // Last computed distance from camera
  bool needs_update;                   // Flag for pending LOD update
} LODObject;

// LOD system state
typedef struct {
  LODConfig config;
  LODObject *objects;
  u32 object_count;
  u32 object_capacity;
  Vec3 camera_position;
  bool enabled;
} LODSystem;

// Initialize LOD system
void lod_system_init(LODSystem *system, LODConfig config);

// Cleanup LOD system
void lod_system_cleanup(LODSystem *system);

// Register an object with LOD levels
u64 lod_system_register_object(LODSystem *system, Vec3 position,
                               Mesh *base_mesh);

// Unregister an object
void lod_system_unregister_object(LODSystem *system, u64 object_id);

// Update camera position and recalculate LOD levels
void lod_system_update(LODSystem *system, Vec3 camera_position);

// Get current LOD level for an object
LODLevel lod_system_get_level(LODSystem *system, u64 object_id);

// Get mesh for current LOD level
Mesh *lod_system_get_mesh(LODSystem *system, u64 object_id);

// Generate simplified mesh for a given LOD level
bool lod_generate_mesh(Mesh *source, Mesh *dest, LODLevel level,
                       f32 target_reduction);

// Calculate geometric error for mesh simplification
f32 lod_calculate_geometric_error(Mesh *original, Mesh *simplified);

// Mesh decimation using edge collapse
bool lod_decimate_mesh(Mesh *source, Mesh *dest, f32 reduction_factor);

// Optimize vertex cache for better GPU performance
void lod_optimize_vertex_cache(Mesh *mesh);

#ifdef __cplusplus
}
#endif

#endif // LOD_SYSTEM_H
