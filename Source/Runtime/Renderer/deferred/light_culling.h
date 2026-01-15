/**
 * =================================================================================================
 *                              CLUSTERED LIGHT CULLING
 * =================================================================================================
 *
 * Purpose: Divides the frustum into 3D clusters (froxels) and assigns lights to them.
 * This allows the deferred lighting pass to only shade pixels with lights that actually affect them.
 */

#pragma once

#include <core/types.h>

/**
 * Constants for cluster grid dimensions
 */
#define CLUSTER_GRID_X 16
#define CLUSTER_GRID_Y 9
#define CLUSTER_GRID_Z 24
#define MAX_LIGHTS_PER_CLUSTER 128

/**
 * A single cluster (froxel) in the grid
 */
typedef struct {
    u32 light_count;
    u32 light_indices[MAX_LIGHTS_PER_CLUSTER];
} LightCluster;

/**
 * The full cluster grid
 */
typedef struct {
    LightCluster clusters[CLUSTER_GRID_X][CLUSTER_GRID_Y][CLUSTER_GRID_Z];
    
    // Bounds and configuration
    f32 near_z;
    f32 far_z;
    f32 grid_scale_z;
    f32 grid_bias_z;
} LightClusterGrid;

/**
 * Builds the cluster grid based on the current camera and light list.
 */
void light_culling_build_clusters(void *camera, void *light_list);

/**
 * Binds the cluster grid data to the GPU (SSBO).
 */
void light_culling_bind_resources(u32 shader_id);
