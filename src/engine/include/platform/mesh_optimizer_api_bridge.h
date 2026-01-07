// Mesh Optimizer API Bridge
// Exposes mesh optimization controls to VoxelForgeStudio

#ifndef MESH_OPTIMIZER_API_BRIDGE_H
#define MESH_OPTIMIZER_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Mesh Optimizer API
// ============================================================================

/// Optimize a mesh (vertex cache, overdraw, fetch)
/// @param mesh_id ID of mesh to optimize
void mesh_optimizer_optimize(uint64_t mesh_id);

/// Simplify a mesh to target triangle count
/// @param mesh_id ID of mesh
/// @param target_triangles Target triangle count
void mesh_optimizer_simplify(uint64_t mesh_id, uint32_t target_triangles);

/// Generate LOD chain for a mesh
/// @param mesh_id ID of mesh
/// @param lod_count Number of LOD levels to generate
void mesh_optimizer_generate_lods(uint64_t mesh_id, uint32_t lod_count);

/// Check if a mesh has been optimized
bool mesh_optimizer_is_optimized(uint64_t mesh_id);

#ifdef __cplusplus
}
#endif

#endif // MESH_OPTIMIZER_API_BRIDGE_H
