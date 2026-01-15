#ifndef DEBUG_VISUALIZATION_H
#define DEBUG_VISUALIZATION_H

#include <simd/simd.h>
#include <stdbool.h>
#include "editor/debug/debug_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct light_system light_system_t;
typedef struct shadow_system shadow_system_t;

// BVH Node Structure
typedef struct bvh_node {
    struct bvh_node* left;
    struct bvh_node* right;
    struct bvh_node* parent;
    
    // Bounding volume
    simd_float3 bounds_min;
    simd_float3 bounds_max;
    
    // Node properties
    bool is_leaf;
    int object_count;
    
    // For leaf nodes: object indices
    int* object_indices;
    int object_capacity;
} bvh_node_t;

// Visualization System Configuration
typedef struct debug_viz_config {
    bool show_lights;
    bool show_shadow_frustums;
    bool show_probe_grid;
    bool show_cluster_grid;
    bool show_bvh;
    bool show_gbuffer_channels;
    float light_sphere_radius;
    simd_float4 light_color;
    simd_float4 shadow_frustum_color;
    simd_float4 probe_color;
    simd_float4 bvh_color;
} debug_viz_config_t;

// Visualization Functions
void debug_viz_init_config(debug_viz_config_t* config);
void debug_viz_draw_lights(debug_renderer_t* dbg, const light_system_t* lights, const debug_viz_config_t* config);
void debug_viz_draw_shadow_frustums(debug_renderer_t* dbg, const shadow_system_t* shadows, const debug_viz_config_t* config);
void debug_viz_draw_probe_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int grid_size_x, int grid_size_y, int grid_size_z, float spacing, const debug_viz_config_t* config);
void debug_viz_draw_cluster_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int clusters_x, int clusters_y, int clusters_z, float cluster_size, const debug_viz_config_t* config);
void debug_viz_draw_bvh(debug_renderer_t* dbg, const bvh_node_t* root, const debug_viz_config_t* config, int max_depth);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_VISUALIZATION_H
