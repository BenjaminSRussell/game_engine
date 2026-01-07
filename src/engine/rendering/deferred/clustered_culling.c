/**
 * =================================================================================================
 *                           CLUSTERED LIGHT CULLING IMPLEMENTATION
 * =================================================================================================
 */

#include "clustered_culling.h"
#include <core/logger/logger.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* =================================================================================================
 *                                    HELPER FUNCTIONS
 * =================================================================================================
 */

/**
 * Converts NDC coordinates to view-space at a given depth.
 */
static simd_float3 ndc_to_view(f32 ndc_x, f32 ndc_y, f32 view_z, simd_float4x4 inv_proj) {
    // Create NDC point
    simd_float4 ndc = simd_make_float4(ndc_x, ndc_y, 0.0f, 1.0f);
    
    // Transform to view space
    simd_float4 view = simd_mul(inv_proj, ndc);
    
    // Perspective divide
    view = view / view.w;
    
    // Scale by depth
    return simd_make_float3(view.x * view_z, view.y * view_z, view_z);
}

/**
 * Calculates the AABB for a screen-space tile at a specific depth range.
 */
static ClusterAABB calculate_cluster_aabb(
    u32 tile_x, u32 tile_y, u32 depth_slice,
    const ClusterConfig *config,
    simd_float4x4 inv_proj,
    f32 slice_near, f32 slice_far)
{
    ClusterAABB aabb;
    
    // Calculate NDC bounds for this tile
    f32 tile_width = 2.0f / config->grid_dim_x;
    f32 tile_height = 2.0f / config->grid_dim_y;
    
    f32 ndc_min_x = -1.0f + tile_x * tile_width;
    f32 ndc_max_x = -1.0f + (tile_x + 1) * tile_width;
    f32 ndc_min_y = -1.0f + tile_y * tile_height;
    f32 ndc_max_y = -1.0f + (tile_y + 1) * tile_height;
    
    // Calculate 8 corners of the frustum tile
    simd_float3 corners[8];
    corners[0] = ndc_to_view(ndc_min_x, ndc_min_y, slice_near, inv_proj);
    corners[1] = ndc_to_view(ndc_max_x, ndc_min_y, slice_near, inv_proj);
    corners[2] = ndc_to_view(ndc_min_x, ndc_max_y, slice_near, inv_proj);
    corners[3] = ndc_to_view(ndc_max_x, ndc_max_y, slice_near, inv_proj);
    corners[4] = ndc_to_view(ndc_min_x, ndc_min_y, slice_far, inv_proj);
    corners[5] = ndc_to_view(ndc_max_x, ndc_min_y, slice_far, inv_proj);
    corners[6] = ndc_to_view(ndc_min_x, ndc_max_y, slice_far, inv_proj);
    corners[7] = ndc_to_view(ndc_max_x, ndc_max_y, slice_far, inv_proj);
    
    // Find min/max
    aabb.min = corners[0];
    aabb.max = corners[0];
    
    for (int i = 1; i < 8; i++) {
        aabb.min = simd_min(aabb.min, corners[i]);
        aabb.max = simd_max(aabb.max, corners[i]);
    }
    
    return aabb;
}

/**
 * Calculates depth slice boundaries.
 */
static f32 calculate_slice_depth(u32 slice_index, const ClusterConfig *config) {
    if (config->depth_slice_mode == CLUSTER_DEPTH_SLICE_MODE_LINEAR) {
        // Linear depth slicing
        f32 t = (f32)slice_index / (f32)config->grid_dim_z;
        return config->near_plane + t * (config->far_plane - config->near_plane);
    } else {
        // Exponential depth slicing (better distribution for perspective)
        f32 t = (f32)slice_index / (f32)config->grid_dim_z;
        return config->near_plane * powf(config->far_plane / config->near_plane, t);
    }
}

/* =================================================================================================
 *                                    PUBLIC API IMPLEMENTATION
 * =================================================================================================
 */

ClusterGrid* cluster_grid_create(const ClusterConfig *config, void *device) {
    ClusterGrid *grid = (ClusterGrid*)malloc(sizeof(ClusterGrid));
    if (!grid) {
        LOG_ERROR("Failed to allocate cluster grid");
        return NULL;
    }
    
    memset(grid, 0, sizeof(ClusterGrid));
    grid->config = *config;
    
    u32 total_clusters = config->grid_dim_x * config->grid_dim_y * config->grid_dim_z;
    u32 max_light_indices = total_clusters * config->max_lights_per_cluster;
    
    // Allocate CPU-side buffers
    grid->cluster_bounds = (ClusterAABB*)malloc(total_clusters * sizeof(ClusterAABB));
    grid->cluster_lists = (ClusterLightList*)calloc(total_clusters, sizeof(ClusterLightList));
    grid->light_indices = (u32*)malloc(max_light_indices * sizeof(u32));
    
    if (!grid->cluster_bounds || !grid->cluster_lists || !grid->light_indices) {
        LOG_ERROR("Failed to allocate cluster CPU buffers");
        cluster_grid_destroy(grid);
        return NULL;
    }
    
#ifdef __OBJC__
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)device;
    
    // Allocate GPU buffers
    grid->gpu_cluster_bounds = [mtl_device newBufferWithLength:total_clusters * sizeof(ClusterAABB)
                                                        options:MTLResourceStorageModeShared];
    
    grid->gpu_cluster_lists = [mtl_device newBufferWithLength:total_clusters * sizeof(ClusterLightList)
                                                       options:MTLResourceStorageModeShared];
    
    grid->gpu_light_indices = [mtl_device newBufferWithLength:max_light_indices * sizeof(u32)
                                                       options:MTLResourceStorageModeShared];
    
    grid->gpu_atomic_counter = [mtl_device newBufferWithLength:sizeof(u32)
                                                        options:MTLResourceStorageModeShared];
    
    if (!grid->gpu_cluster_bounds || !grid->gpu_cluster_lists || 
        !grid->gpu_light_indices || !grid->gpu_atomic_counter) {
        LOG_ERROR("Failed to allocate cluster GPU buffers");
        cluster_grid_destroy(grid);
        return NULL;
    }
#endif
    
    grid->stats.total_clusters = total_clusters;
    grid->debug_enabled = false;
    
    LOG_INFO("Cluster grid created: %ux%ux%u = %u clusters", 
             config->grid_dim_x, config->grid_dim_y, config->grid_dim_z, total_clusters);
    
    return grid;
}

void cluster_grid_destroy(ClusterGrid *grid) {
    if (!grid) return;
    
    free(grid->cluster_bounds);
    free(grid->cluster_lists);
    free(grid->light_indices);
    
#ifdef __OBJC__
    // ARC will handle GPU buffer cleanup
#endif
    
    free(grid);
}

void cluster_grid_calculate_bounds(ClusterGrid *grid, simd_float4x4 view_matrix, simd_float4x4 proj_matrix) {
    simd_float4x4 inv_proj = simd_inverse(proj_matrix);
    
    u32 idx = 0;
    for (u32 z = 0; z < grid->config.grid_dim_z; z++) {
        f32 slice_near = calculate_slice_depth(z, &grid->config);
        f32 slice_far = calculate_slice_depth(z + 1, &grid->config);
        
        for (u32 y = 0; y < grid->config.grid_dim_y; y++) {
            for (u32 x = 0; x < grid->config.grid_dim_x; x++) {
                grid->cluster_bounds[idx] = calculate_cluster_aabb(
                    x, y, z, &grid->config, inv_proj, slice_near, slice_far
                );
                idx++;
            }
        }
    }
}

void cluster_grid_upload_bounds(ClusterGrid *grid) {
#ifdef __OBJC__
    u32 total_clusters = grid->config.grid_dim_x * grid->config.grid_dim_y * grid->config.grid_dim_z;
    memcpy([grid->gpu_cluster_bounds contents], grid->cluster_bounds, 
           total_clusters * sizeof(ClusterAABB));
#endif
}

void cluster_grid_clear_lights(ClusterGrid *grid) {
    u32 total_clusters = grid->config.grid_dim_x * grid->config.grid_dim_y * grid->config.grid_dim_z;
    memset(grid->cluster_lists, 0, total_clusters * sizeof(ClusterLightList));
    grid->light_index_count = 0;
    
#ifdef __OBJC__
    // Reset atomic counter
    u32 zero = 0;
    memcpy([grid->gpu_atomic_counter contents], &zero, sizeof(u32));
#endif
}

void cluster_grid_resize(ClusterGrid *grid, u32 new_width, u32 new_height) {
    grid->config.screen_width = new_width;
    grid->config.screen_height = new_height;
    
    LOG_INFO("Cluster grid resized to %ux%u", new_width, new_height);
}

u32 cluster_grid_get_index(const ClusterGrid *grid, f32 screen_x, f32 screen_y, f32 view_depth) {
    // Calculate tile coordinates
    u32 tile_x = (u32)(screen_x / grid->config.screen_width * grid->config.grid_dim_x);
    u32 tile_y = (u32)(screen_y / grid->config.screen_height * grid->config.grid_dim_y);
    
    // Calculate depth slice
    u32 tile_z = cluster_grid_depth_to_slice(grid, view_depth);
    
    // Bounds check
    if (tile_x >= grid->config.grid_dim_x || 
        tile_y >= grid->config.grid_dim_y || 
        tile_z >= grid->config.grid_dim_z) {
        return UINT32_MAX;
    }
    
    return cluster_index(tile_x, tile_y, tile_z, 
                        grid->config.grid_dim_x, grid->config.grid_dim_y);
}

u32 cluster_grid_depth_to_slice(const ClusterGrid *grid, f32 view_depth) {
    if (view_depth < grid->config.near_plane) return 0;
    if (view_depth >= grid->config.far_plane) return grid->config.grid_dim_z - 1;
    
    if (grid->config.depth_slice_mode == CLUSTER_DEPTH_SLICE_MODE_LINEAR) {
        f32 t = (view_depth - grid->config.near_plane) / 
                (grid->config.far_plane - grid->config.near_plane);
        return (u32)(t * grid->config.grid_dim_z);
    } else {
        // Exponential
        f32 log_depth = logf(view_depth / grid->config.near_plane);
        f32 log_range = logf(grid->config.far_plane / grid->config.near_plane);
        f32 t = log_depth / log_range;
        return (u32)(t * grid->config.grid_dim_z);
    }
}

void cluster_grid_update_stats(ClusterGrid *grid) {
    u32 total_clusters = grid->config.grid_dim_x * grid->config.grid_dim_y * grid->config.grid_dim_z;
    u32 total_light_assignments = 0;
    u32 max_lights = 0;
    u32 clusters_with_lights = 0;
    
    for (u32 i = 0; i < total_clusters; i++) {
        u32 count = grid->cluster_lists[i].count;
        total_light_assignments += count;
        if (count > 0) clusters_with_lights++;
        if (count > max_lights) max_lights = count;
    }
    
    grid->stats.total_lights = total_light_assignments;
    grid->stats.max_lights_per_cluster = max_lights;
    grid->stats.avg_lights_per_cluster = (f32)total_light_assignments / (f32)total_clusters;
    grid->stats.clusters_with_lights = clusters_with_lights;
}

void cluster_grid_set_debug(ClusterGrid *grid, bool enabled) {
    grid->debug_enabled = enabled;
    LOG_INFO("Cluster grid debug visualization: %s", enabled ? "ENABLED" : "DISABLED");
}
