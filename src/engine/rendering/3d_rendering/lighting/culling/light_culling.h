/*
 * light_culling.h
 * Light culling system (Clustered & Tile-based)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHT_CULLING_H
#define LIGHTING_LIGHT_CULLING_H

#include "../../math/vec3.h"
#include "../../math/mat4.h"
#include "../../math/aabb.h"
#include "../../math/frustum.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DEFINITIONS
 * ============================================================================ */

#define LC_CLUSTER_GRID_X 16
#define LC_CLUSTER_GRID_Y 9
#define LC_CLUSTER_GRID_Z 24
#define LC_MAX_LIGHTS_PER_CLUSTER 256
#define LC_MAX_LIGHTS_TOTAL 2048

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct light_cluster {
    uint32_t point_light_count;
    uint32_t point_light_offset; // Offset into global light index list
    
    uint32_t spot_light_count;
    uint32_t spot_light_offset;
    
    uint32_t rect_light_count;
    uint32_t rect_light_offset;
    
    uint32_t sphere_light_count;
    uint32_t sphere_light_offset;
} light_cluster_t;

typedef struct light_cluster_grid {
    // Grid dimensions
    uint32_t count_x;
    uint32_t count_y;
    uint32_t count_z;
    
    // View space depth slices
    float depth_slices[LC_CLUSTER_GRID_Z + 1];
    
    // Clusters flat array (x + y*width + z*width*height)
    light_cluster_t* clusters;
    
    // Global index list for all clusters
    uint32_t* point_light_indices;
    uint32_t point_light_index_count;
    uint32_t point_light_index_capacity;
    
    uint32_t* spot_light_indices;
    uint32_t spot_light_index_count;
    uint32_t spot_light_index_capacity;
    
    // ... rect/sphere indices ...
    
    // GPU buffer handles (void* for abstraction)
    void* gpu_cluster_buffer;
    void* gpu_light_index_buffer;
} light_cluster_grid_t;

/* ============================================================================
 * API
 * ============================================================================ */

int light_culling_init(void);
void light_culling_shutdown(void);

/**
 * Update the cluster grid based on the camera view
 */
void light_culling_update_grid_frustums(const mat4_t* proj, const mat4_t* view, float near_plane, float far_plane);

/**
 * Assign lights to clusters
 * This performs CPU-side culling and populates the grid structures
 */
void light_culling_assign_lights(
    const mat4_t* view_matrix,
    // Accessors to get light data (passed as function pointers or context)
    // For now we might assume access to global light contexts or pass arrays
    uint32_t point_light_count,
    uint32_t spot_light_count,
    uint32_t rect_light_count,
    uint32_t sphere_light_count
    // internal logic will query specific lights
);

/**
 * Upload culling data to GPU
 */
void light_culling_upload_gpu_data(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHT_CULLING_H */
