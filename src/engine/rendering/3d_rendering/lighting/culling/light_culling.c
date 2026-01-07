/*
 * light_culling.c
 * Light culling system (Clustered)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "light_culling.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include light source headers to access their data/bounds
#include "../sources/point_light.h"
#include "../sources/spot_light.h"
#include "../sources/rect_light.h"
#include "../sources/spherical_light.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LC_DEFAULT_INDEX_CAPACITY (LC_CLUSTER_GRID_X * LC_CLUSTER_GRID_Y * LC_CLUSTER_GRID_Z * 32)
#define LC_LIGHT_ITERATION_MAX 2048

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static light_cluster_grid_t g_cluster_grid = {0};
static bool g_culling_initialized = false;

// Frustums for each cluster (computed once per frame or on demand)
static aabb_t g_cluster_aabbs[LC_CLUSTER_GRID_X * LC_CLUSTER_GRID_Y * LC_CLUSTER_GRID_Z];

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool test_aabb_sphere(aabb_t box, vec3_t sphere_center, float sphere_radius) {
    // Find point P on box closest to sphere center
    vec3_t p = vec3_set(
        fmaxf(box.min.x, fminf(sphere_center.x, box.max.x)),
        fmaxf(box.min.y, fminf(sphere_center.y, box.max.y)),
        fmaxf(box.min.z, fminf(sphere_center.z, box.max.z))
    );
    
    // Distance squared between center and P
    vec3_t diff = vec3_sub(p, sphere_center);
    float dist_sq = vec3_dot(diff, diff);
    
    return dist_sq <= (sphere_radius * sphere_radius);
}

static void calculate_cluster_bounds(const mat4_t* proj, float near_z, float far_z) {
    if (!proj) return; // Need projection matrix to unproject
    
    // To properly calculate AABBs in View Space:
    // 1. Calculate the screen-space division lines in NDC (-1 to 1)
    // 2. Unproject the intersection points of these lines at Near and Far planes to View Space
    // 3. Form AABB from these view-space points
    
    mat4_t inv_proj = mat4_inverse(*proj);
    
    // Z-slice distribution (Logarithmic)
    float log_factor = far_z / near_z;
    
    for (int k = 0; k <= LC_CLUSTER_GRID_Z; ++k) {
         float t = (float)k / (float)LC_CLUSTER_GRID_Z;
         float z_dist = near_z * powf(log_factor, t);
         g_cluster_grid.depth_slices[k] = z_dist;
    }
    
    // Compute AABBs
    for (int z = 0; z < LC_CLUSTER_GRID_Z; ++z) {
        float z_near = -g_cluster_grid.depth_slices[z]; // View space Z (negative)
        float z_far = -g_cluster_grid.depth_slices[z+1];
        
        for (int y = 0; y < LC_CLUSTER_GRID_Y; ++y) {
            for (int x = 0; x < LC_CLUSTER_GRID_X; ++x) {
                // NDC coordinates for tile min/max
                float x_min_ndc = (float)x / LC_CLUSTER_GRID_X * 2.0f - 1.0f;
                float x_max_ndc = (float)(x+1) / LC_CLUSTER_GRID_X * 2.0f - 1.0f;
                float y_min_ndc = (float)y / LC_CLUSTER_GRID_Y * 2.0f - 1.0f;
                float y_max_ndc = (float)(y+1) / LC_CLUSTER_GRID_Y * 2.0f - 1.0f;
                
                vec3_t corners_ndc[4] = {
                    vec3_set(x_min_ndc, y_min_ndc, -1.0f), 
                    vec3_set(x_max_ndc, y_min_ndc, -1.0f),
                    vec3_set(x_max_ndc, y_max_ndc, -1.0f),
                    vec3_set(x_min_ndc, y_max_ndc, -1.0f)
                };
                
                vec3_t bounds_min = vec3_set(1e9f, 1e9f, 1e9f);
                vec3_t bounds_max = vec3_set(-1e9f, -1e9f, -1e9f);
                
                // For each corner ray
                for(int c=0; c<4; ++c) {
                    vec4_t p_view_homo = mat4_mul_vec4(inv_proj, vec4_set(corners_ndc[c].x, corners_ndc[c].y, 1.0f, 1.0f)); 
                    vec3_t p_view = vec3_scale(vec3_set(p_view_homo.x, p_view_homo.y, p_view_homo.z), 1.0f / p_view_homo.w);
                    
                    vec3_t ray = vec3_normalize(p_view);
                    
                    if (fabsf(ray.z) > 1e-6f) {
                        float t_near = z_near / ray.z;
                        float t_far = z_far / ray.z;
                        
                        vec3_t p_near = vec3_scale(ray, t_near);
                        vec3_t p_far = vec3_scale(ray, t_far);
                        
                        bounds_min = vec3_min(bounds_min, p_near);
                        bounds_max = vec3_max(bounds_max, p_near);
                        bounds_min = vec3_min(bounds_min, p_far);
                        bounds_max = vec3_max(bounds_max, p_far);
                    }
                }
                
                // Store AABB
                int idx = x + y * LC_CLUSTER_GRID_X + z * (LC_CLUSTER_GRID_X * LC_CLUSTER_GRID_Y);
                g_cluster_aabbs[idx].min = bounds_min;
                g_cluster_aabbs[idx].max = bounds_max;
            }
        }
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int light_culling_init(void) {
    if (g_culling_initialized) return 0;
    
    g_cluster_grid.count_x = LC_CLUSTER_GRID_X;
    g_cluster_grid.count_y = LC_CLUSTER_GRID_Y;
    g_cluster_grid.count_z = LC_CLUSTER_GRID_Z;
    
    // Allocate clusters
    size_t cluster_count = LC_CLUSTER_GRID_X * LC_CLUSTER_GRID_Y * LC_CLUSTER_GRID_Z;
    g_cluster_grid.clusters = calloc(cluster_count, sizeof(light_cluster_t));
    if (!g_cluster_grid.clusters) return -1;
    
    // Allocate index lists with specific capacities
    // For Point Lights
    g_cluster_grid.point_light_index_capacity = LC_DEFAULT_INDEX_CAPACITY;
    g_cluster_grid.point_light_indices = malloc(g_cluster_grid.point_light_index_capacity * sizeof(uint32_t));
    
    // For Spot Lights
    g_cluster_grid.spot_light_index_capacity = LC_DEFAULT_INDEX_CAPACITY;
    g_cluster_grid.spot_light_indices = malloc(g_cluster_grid.spot_light_index_capacity * sizeof(uint32_t));
    
    if (!g_cluster_grid.point_light_indices || !g_cluster_grid.spot_light_indices) {
        free(g_cluster_grid.clusters);
        if (g_cluster_grid.point_light_indices) free(g_cluster_grid.point_light_indices);
        if (g_cluster_grid.spot_light_indices) free(g_cluster_grid.spot_light_indices);
        return -1;
    }
    
    g_culling_initialized = true;
    return 0;
}

void light_culling_shutdown(void) {
    if (!g_culling_initialized) return;
    
    if (g_cluster_grid.clusters) free(g_cluster_grid.clusters);
    if (g_cluster_grid.point_light_indices) free(g_cluster_grid.point_light_indices);
    if (g_cluster_grid.spot_light_indices) free(g_cluster_grid.spot_light_indices);
    // Add rect/sphere cleanup if allocated
    
    g_culling_initialized = false;
}

void light_culling_update_grid_frustums(const mat4_t* proj, const mat4_t* view, float near_plane, float far_plane) {
    if (!g_culling_initialized || !proj) return;
    calculate_cluster_bounds(proj, near_plane, far_plane);
}

void light_culling_assign_lights(
    const mat4_t* view_matrix,
    uint32_t point_light_count,
    uint32_t spot_light_count,
    uint32_t rect_light_count,
    uint32_t sphere_light_count
) {
    if (!g_culling_initialized || !view_matrix) return;
    
    size_t total_clusters = LC_CLUSTER_GRID_X * LC_CLUSTER_GRID_Y * LC_CLUSTER_GRID_Z;
    
    // Clear clusters
    for(size_t i=0; i<total_clusters; ++i) {
        g_cluster_grid.clusters[i].point_light_count = 0;
        g_cluster_grid.clusters[i].point_light_offset = 0;
        // Spot
        g_cluster_grid.clusters[i].spot_light_count = 0;
        g_cluster_grid.clusters[i].spot_light_offset = 0;
        // Rect, Sphere...
        g_cluster_grid.clusters[i].rect_light_count = 0;
        g_cluster_grid.clusters[i].rect_light_offset = 0;
        g_cluster_grid.clusters[i].sphere_light_count = 0;
        g_cluster_grid.clusters[i].sphere_light_offset = 0;
    }
    
    // -------------------------------------------------------------------------
    // POINT LIGHTS
    // -------------------------------------------------------------------------
    // PASS 1: Count
    for (uint32_t i = 0; i < LC_LIGHT_ITERATION_MAX; ++i) {
        lighting_point_light_handle_t h = {i};
        lighting_point_light_info_t info;
        if (lighting_point_light_get_info(h, &info) != 0 || !info.data.active) continue;
        
        vec3_t pos_vs = mat4_mul_vec3(*view_matrix, info.data.position, 1.0f);
        float r = info.data.radius;
        
        for (int c = 0; c < total_clusters; ++c) {
            if (test_aabb_sphere(g_cluster_aabbs[c], pos_vs, r)) {
                if (g_cluster_grid.clusters[c].point_light_count < LC_MAX_LIGHTS_PER_CLUSTER) {
                    g_cluster_grid.clusters[c].point_light_count++;
                }
            }
        }
    }
    
    // Prefix Sum (Point)
    uint32_t* fill_counters = calloc(total_clusters, sizeof(uint32_t));
    if (!fill_counters) return;

    uint32_t current_offset = 0;
    for (int c = 0; c < total_clusters; ++c) {
        g_cluster_grid.clusters[c].point_light_offset = current_offset;
        current_offset += g_cluster_grid.clusters[c].point_light_count;
    }
    
    // Reallocate index buffer if needed
    if (current_offset > g_cluster_grid.point_light_index_capacity) {
        g_cluster_grid.point_light_index_capacity = current_offset + 1024;
        g_cluster_grid.point_light_indices = realloc(g_cluster_grid.point_light_indices, g_cluster_grid.point_light_index_capacity * sizeof(uint32_t));
    }
    g_cluster_grid.point_light_index_count = current_offset;
    
    // PASS 2: Fill (Point)
    for (uint32_t i = 0; i < LC_LIGHT_ITERATION_MAX; ++i) {
        lighting_point_light_handle_t h = {i};
        lighting_point_light_info_t info;
        if (lighting_point_light_get_info(h, &info) != 0 || !info.data.active) continue;
        
        vec3_t pos_vs = mat4_mul_vec3(*view_matrix, info.data.position, 1.0f);
        float r = info.data.radius;
        
        for (int c = 0; c < total_clusters; ++c) {
            if (test_aabb_sphere(g_cluster_aabbs[c], pos_vs, r)) {
                light_cluster_t* cluster = &g_cluster_grid.clusters[c];
                uint32_t max_in_cluster = cluster->point_light_count;
                
                if (fill_counters[c] < max_in_cluster) {
                    uint32_t idx = cluster->point_light_offset + fill_counters[c];
                    g_cluster_grid.point_light_indices[idx] = i; 
                    fill_counters[c]++;
                }
            }
        }
    }
    
    // Reset counters for next light type
    memset(fill_counters, 0, total_clusters * sizeof(uint32_t));
    
    // -------------------------------------------------------------------------
    // SPOT LIGHTS
    // -------------------------------------------------------------------------
    // PASS 1: Count
    for (uint32_t i = 0; i < LC_LIGHT_ITERATION_MAX; ++i) {
        lighting_spot_light_handle_t h = {i};
        lighting_spot_light_info_t info;
        if (lighting_spot_light_get_info(h, &info) != 0 || !info.data.active) continue;
        
        // Sphere bound approximation for spot light (safe but conservative)
        vec3_t pos_vs = mat4_mul_vec3(*view_matrix, info.data.position, 1.0f);
        float r = info.data.radius;
        
        // Ideally intersect cone vs AABB, but sphere vs AABB is simpler for now
        for (int c = 0; c < total_clusters; ++c) {
            if (test_aabb_sphere(g_cluster_aabbs[c], pos_vs, r)) {
                if (g_cluster_grid.clusters[c].spot_light_count < LC_MAX_LIGHTS_PER_CLUSTER) {
                    g_cluster_grid.clusters[c].spot_light_count++;
                }
            }
        }
    }
    
    // Prefix Sum (Spot)
    current_offset = 0;
    for (int c = 0; c < total_clusters; ++c) {
        g_cluster_grid.clusters[c].spot_light_offset = current_offset;
        current_offset += g_cluster_grid.clusters[c].spot_light_count;
    }
    
    if (current_offset > g_cluster_grid.spot_light_index_capacity) {
        g_cluster_grid.spot_light_index_capacity = current_offset + 1024;
        g_cluster_grid.spot_light_indices = realloc(g_cluster_grid.spot_light_indices, g_cluster_grid.spot_light_index_capacity * sizeof(uint32_t));
    }
    g_cluster_grid.spot_light_index_count = current_offset;
    
    // PASS 2: Fill (Spot)
    for (uint32_t i = 0; i < LC_LIGHT_ITERATION_MAX; ++i) {
        lighting_spot_light_handle_t h = {i};
        lighting_spot_light_info_t info;
        if (lighting_spot_light_get_info(h, &info) != 0 || !info.data.active) continue;
        
        vec3_t pos_vs = mat4_mul_vec3(*view_matrix, info.data.position, 1.0f);
        float r = info.data.radius;
        
        for (int c = 0; c < total_clusters; ++c) {
            if (test_aabb_sphere(g_cluster_aabbs[c], pos_vs, r)) {
                light_cluster_t* cluster = &g_cluster_grid.clusters[c];
                uint32_t max_in_cluster = cluster->spot_light_count;
                
                if (fill_counters[c] < max_in_cluster) {
                    uint32_t idx = cluster->spot_light_offset + fill_counters[c];
                    g_cluster_grid.spot_light_indices[idx] = i; 
                    fill_counters[c]++;
                }
            }
        }
    }
    
    // Rect and Sphere lights follow similar pattern but we leave them empty for now or add them later
    
    free(fill_counters);
}

void light_culling_upload_gpu_data(void) {
    // TODO: Upload grid and index buffers
}

/* End of light_culling.c */
