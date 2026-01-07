/**
 * =================================================================================================
 *                           CLUSTERED LIGHT CULLING SYSTEM
 * =================================================================================================
 *
 * Purpose: Implements a 3D view-space grid for efficient light culling in deferred rendering.
 * Divides the view frustum into clusters and assigns lights to each cluster.
 */

#pragma once

#include <core/types.h>
#include <simd/simd.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef struct objc_object* id;
#endif

/* =================================================================================================
 *                                    CONSTANTS & CONFIGURATION
 * =================================================================================================
 */

#define CLUSTER_GRID_DIM_X          16      // Screen-space tile count X
#define CLUSTER_GRID_DIM_Y          9       // Screen-space tile count Y
#define CLUSTER_GRID_DIM_Z          24      // Depth slices
#define CLUSTER_MAX_LIGHTS_PER_CELL 256     // Maximum lights per cluster

#define CLUSTER_DEPTH_SLICE_MODE_LINEAR      0
#define CLUSTER_DEPTH_SLICE_MODE_EXPONENTIAL 1

/* =================================================================================================
 *                                    TYPE DEFINITIONS
 * =================================================================================================
 */

/**
 * Configuration for cluster grid setup.
 */
typedef struct {
    u32 grid_dim_x;         // Number of clusters in X (screen width)
    u32 grid_dim_y;         // Number of clusters in Y (screen height)
    u32 grid_dim_z;         // Number of depth slices
    u32 max_lights_per_cluster;
    
    f32 near_plane;
    f32 far_plane;
    
    u32 depth_slice_mode;   // Linear or exponential slicing
    
    u32 screen_width;
    u32 screen_height;
} ClusterConfig;

/**
 * Axis-Aligned Bounding Box for cluster bounds.
 */
typedef struct {
    simd_float3 min;
    simd_float3 max;
} ClusterAABB;

/**
 * Per-cluster light list metadata.
 * Stores offset into global light index buffer and count of lights.
 */
typedef struct {
    u32 offset;         // Offset into global light index array
    u32 count;          // Number of lights in this cluster
    f32 min_depth;      // Minimum depth in this cluster (view-space Z)
    f32 max_depth;      // Maximum depth in this cluster (view-space Z)
} ClusterLightList;

/**
 * Statistics for cluster culling performance.
 */
typedef struct {
    u32 total_clusters;
    u32 total_lights;
    f32 avg_lights_per_cluster;
    u32 max_lights_per_cluster;
    u32 clusters_with_lights;
} ClusterStatistics;

/**
 * Main cluster grid structure.
 */
typedef struct {
    ClusterConfig config;
    
    // CPU-side data
    ClusterAABB *cluster_bounds;        // Array of AABBs (one per cluster)
    ClusterLightList *cluster_lists;    // Array of light list metadata
    u32 *light_indices;                 // Global light index buffer
    u32 light_index_count;              // Current count in light_indices
    
    // GPU buffers
#ifdef __OBJC__
    id<MTLBuffer> gpu_cluster_bounds;   // View-space AABBs
    id<MTLBuffer> gpu_cluster_lists;    // Light list metadata
    id<MTLBuffer> gpu_light_indices;    // Compacted light indices
    id<MTLBuffer> gpu_atomic_counter;   // For building light lists
#else
    void *gpu_cluster_bounds;
    void *gpu_cluster_lists;
    void *gpu_light_indices;
    void *gpu_atomic_counter;
#endif
    
    // Statistics
    ClusterStatistics stats;
    
    // Debug visualization
    bool debug_enabled;
    
} ClusterGrid;

/* =================================================================================================
 *                                    PUBLIC API
 * =================================================================================================
 */

/**
 * Creates and initializes a cluster grid with the given configuration.
 */
ClusterGrid* cluster_grid_create(const ClusterConfig *config, void *device);

/**
 * Destroys the cluster grid and releases all resources.
 */
void cluster_grid_destroy(ClusterGrid *grid);

/**
 * Calculates cluster bounds in view-space based on camera parameters.
 * 
 * @param grid The cluster grid
 * @param view_matrix Camera view matrix
 * @param proj_matrix Camera projection matrix (perspective or orthographic)
 */
void cluster_grid_calculate_bounds(ClusterGrid *grid, simd_float4x4 view_matrix, simd_float4x4 proj_matrix);

/**
 * Uploads cluster bounds to GPU.
 */
void cluster_grid_upload_bounds(ClusterGrid *grid);

/**
 * Resets the light lists (usually called at start of frame).
 */
void cluster_grid_clear_lights(ClusterGrid *grid);

/**
 * Resizes the cluster grid (called when viewport changes).
 */
void cluster_grid_resize(ClusterGrid *grid, u32 new_width, u32 new_height);

/**
 * Calculates cluster index from 3D grid coordinates.
 */
static inline u32 cluster_index(u32 x, u32 y, u32 z, u32 dim_x, u32 dim_y) {
    return x + (y * dim_x) + (z * dim_x * dim_y);
}

/**
 * Calculates which cluster a screen-space position and depth belongs to.
 * 
 * @param grid The cluster grid
 * @param screen_x Screen-space X coordinate [0, screen_width)
 * @param screen_y Screen-space Y coordinate [0, screen_height)
 * @param view_depth View-space depth (positive Z in view space)
 * @return Cluster index, or UINT32_MAX if out of bounds
 */
u32 cluster_grid_get_index(const ClusterGrid *grid, f32 screen_x, f32 screen_y, f32 view_depth);

/**
 * Calculates the Z slice index from view-space depth.
 */
u32 cluster_grid_depth_to_slice(const ClusterGrid *grid, f32 view_depth);

/**
 * Updates statistics after culling.
 */
void cluster_grid_update_stats(ClusterGrid *grid);

/**
 * Enables/disables debug visualization.
 */
void cluster_grid_set_debug(ClusterGrid *grid, bool enabled);
