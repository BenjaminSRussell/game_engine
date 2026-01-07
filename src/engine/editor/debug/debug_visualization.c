#include "editor/debug/debug_visualization.h"
#include <stdlib.h>
#include <include/math/math.h>

void debug_viz_init_config(debug_viz_config_t* config) {
    if (!config) return;
    
    config->show_lights = false;
    config->show_shadow_frustums = false;
    config->show_probe_grid = false;
    config->show_cluster_grid = false;
    config->show_bvh = false;
    config->show_gbuffer_channels = false;
    
    config->light_sphere_radius = 0.2f;
    config->light_color = (simd_float4){1.0f, 1.0f, 0.0f, 0.8f}; // Yellow
    config->shadow_frustum_color = (simd_float4){1.0f, 0.0f, 1.0f, 0.5f}; // Magenta
    config->probe_color = (simd_float4){0.0f, 1.0f, 1.0f, 0.6f}; // Cyan
    config->bvh_color = (simd_float4){0.0f, 1.0f, 0.0f, 0.4f}; // Green
}

void debug_viz_draw_lights(debug_renderer_t* dbg, const light_system_t* lights, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_lights) return;
    
    // TODO: Iterate through actual light positions from light_system
    // For now, placeholder implementation
    // This would need integration with the actual light_system_t structure
}

void debug_viz_draw_shadow_frustums(debug_renderer_t* dbg, const shadow_system_t* shadows, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_shadow_frustums) return;
    
    // TODO: Get shadow camera matrices from shadow_system
    // Use debug_draw_frustum for each shadow cascade
}

void debug_viz_draw_probe_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int grid_size_x, int grid_size_y, int grid_size_z, float spacing, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_probe_grid) return;
    
    // Draw spheres at each probe position
    for (int x = 0; x < grid_size_x; x++) {
        for (int y = 0; y < grid_size_y; y++) {
            for (int z = 0; z < grid_size_z; z++) {
                simd_float3 probe_pos = grid_origin + simd_make_float3(
                    x * spacing,
                    y * spacing,
                    z * spacing
                );
                debug_draw_sphere(dbg, probe_pos, 0.1f, config->probe_color);
            }
        }
    }
}

void debug_viz_draw_cluster_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int clusters_x, int clusters_y, int clusters_z, float cluster_size, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_cluster_grid) return;
    
    // Draw AABBs for each cluster
    for (int x = 0; x < clusters_x; x++) {
        for (int y = 0; y < clusters_y; y++) {
            for (int z = 0; z < clusters_z; z++) {
                simd_float3 cluster_min = grid_origin + simd_make_float3(
                    x * cluster_size,
                    y * cluster_size,
                    z * cluster_size
                );
                simd_float3 cluster_max = cluster_min + simd_make_float3(
                    cluster_size,
                    cluster_size,
                    cluster_size
                );
                
                simd_float4 cluster_color = config->probe_color;
                cluster_color.w = 0.2f; // Very transparent
                debug_draw_aabb(dbg, cluster_min, cluster_max, cluster_color);
            }
        }
    }
}

// Recursive BVH drawing helper
static void debug_viz_draw_bvh_node(debug_renderer_t* dbg, const bvh_node_t* node, const debug_viz_config_t* config, int current_depth, int max_depth) {
    if (!node || current_depth > max_depth) return;
    
    // TODO: This needs actual BVH structure definition
    // For now, placeholder that would draw the AABB bounds of each node
    
    // Recursive calls for children would go here
}

void debug_viz_draw_bvh(debug_renderer_t* dbg, const bvh_node_t* root, const debug_viz_config_t* config, int max_depth) {
    if (!dbg || !config || !config->show_bvh) return;
    
    debug_viz_draw_bvh_node(dbg, root, config, 0, max_depth);
}
