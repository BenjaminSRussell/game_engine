#ifndef OCEAN_MESH_H
#define OCEAN_MESH_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct MTLDevice* MTLDeviceRef;
typedef struct MTLBuffer* MTLBufferRef;

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Ocean Mesh Configuration
// =============================================================================

#define OCEAN_MESH_MAX_LOD_LEVELS 5

typedef struct OceanMeshConfig {
    // Grid resolution per LOD level
    uint32_t base_resolution;       // Base resolution (e.g., 128)
    uint32_t lod_count;              // Number of LOD levels (1-5)
    float lod_distances[OCEAN_MESH_MAX_LOD_LEVELS];  // Distance thresholds
    
    // Physical size
    float tile_size;                 // Physical size in world units
    
    // Tessellation
    bool use_triangle_strips;        // Use strips vs indexed triangles
} OceanMeshConfig;

// =============================================================================
// Ocean Mesh Data
// =============================================================================

typedef struct OceanMeshLOD {
    // Vertex data
    MTLBufferRef vertex_buffer;      // Vertex positions (vec3)
    MTLBufferRef index_buffer;       // Triangle indices
    
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t resolution;             // Grid resolution for this LOD
    
    // Distance thresholds
    float min_distance;
    float max_distance;
} OceanMeshLOD;

typedef struct OceanMesh {
    OceanMeshConfig config;
    
    // Metal resources
    MTLDeviceRef device;
    
    // LOD levels
    uint32_t lod_count;
    OceanMeshLOD lods[OCEAN_MESH_MAX_LOD_LEVELS];
    
    // Current active LOD (updated by culling)
    uint32_t active_lod_index;
} OceanMesh;

// =============================================================================
// Ocean Mesh API
// =============================================================================

/**
 * Create ocean mesh with multiple LOD levels
 * 
 * @param device Metal device
 * @param config Mesh configuration
 * @return Initialized mesh or NULL on failure
 */
OceanMesh* ocean_mesh_create(
    MTLDeviceRef device,
    const OceanMeshConfig* config
);

/**
 * Destroy ocean mesh and release resources
 */
void ocean_mesh_destroy(OceanMesh* mesh);

/**
 * Select appropriate LOD based on camera distance
 * 
 * @param mesh Ocean mesh
 * @param camera_distance Distance from camera to ocean center
 * @return Selected LOD index
 */
uint32_t ocean_mesh_select_lod(
    OceanMesh* mesh,
    float camera_distance
);

/**
 * Get vertex buffer for specific LOD
 * 
 * @param mesh Ocean mesh
 * @param lod_index LOD index (0 to lod_count-1)
 * @return Vertex buffer or NULL if invalid
 */
MTLBufferRef ocean_mesh_get_vertex_buffer(
    const OceanMesh* mesh,
    uint32_t lod_index
);

/**
 * Get index buffer for specific LOD
 * 
 * @param mesh Ocean mesh
 * @param lod_index LOD index
 * @return Index buffer or NULL if invalid
 */
MTLBufferRef ocean_mesh_get_index_buffer(
    const OceanMesh* mesh,
    uint32_t lod_index
);

/**
 * Get index count for specific LOD (for draw calls)
 * 
 * @param mesh Ocean mesh
 * @param lod_index LOD index
 * @return Index count or 0 if invalid
 */
uint32_t ocean_mesh_get_index_count(
    const OceanMesh* mesh,
    uint32_t lod_index
);

/**
 * Get default mesh configuration
 * - Base resolution: 128
 * - 3 LOD levels
 * - Triangle strips enabled
 */
OceanMeshConfig ocean_mesh_default_config(void);

#ifdef __cplusplus
}
#endif

#endif // OCEAN_MESH_H
