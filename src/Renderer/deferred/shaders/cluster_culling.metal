/**
 * =================================================================================================
 *                           CLUSTER LIGHT CULLING COMPUTE SHADER
 * =================================================================================================
 *
 * Purpose: GPU compute shader for assigning lights to view-space clusters.
 * Uses frustum-sphere and frustum-cone intersection tests.
 */

#include <metal_stdlib>
using namespace metal;

/* =================================================================================================
 *                                    STRUCTURES
 * =================================================================================================
 */

struct ClusterAABB {
    float3 min_bounds;
    float3 max_bounds;
};

struct ClusterLightList {
    uint offset;
    uint count;
    float min_depth;
    float max_depth;
};

struct Light {
    float3 position;
    float radius;
    float3 direction;
    float inner_cone;
    float3 color;
    float outer_cone;
    float intensity;
    uint type;              // 0=directional, 1=point, 2=spot
    uint cast_shadows;
    uint shadow_map_index;
    uint padding;
};

struct CullingUniforms {
    uint grid_dim_x;
    uint grid_dim_y;
    uint grid_dim_z;
    uint total_lights;
    uint max_lights_per_cluster;
};

/* =================================================================================================
 *                                    INTERSECTION TESTS
 * =================================================================================================
 */

/**
 * Tests if a sphere intersects an AABB.
 */
bool sphere_aabb_intersection(float3 sphere_center, float sphere_radius, ClusterAABB aabb) {
    // Find closest point on AABB to sphere center
    float3 closest_point = clamp(sphere_center, aabb.min_bounds, aabb.max_bounds);
    
    // Calculate distance from sphere center to closest point
    float3 diff = sphere_center - closest_point;
    float dist_sq = dot(diff, diff);
    
    return dist_sq <= (sphere_radius * sphere_radius);
}

/**
 * Tests if a cone (spotlight) intersects an AABB.
 * Simplified conservative test using sphere at cone tip.
 */
bool cone_aabb_intersection(float3 cone_tip, float3 cone_dir, float cone_radius, 
                           float cone_angle, ClusterAABB aabb) {
    // Conservative test: use sphere at tip
    // More accurate test would check cone geometry, but this is faster
    bool tip_in_sphere = sphere_aabb_intersection(cone_tip, cone_radius, aabb);
    if (tip_in_sphere) return true;
    
    // Also check if AABB center is within cone
    float3 aabb_center = (aabb.min_bounds + aabb.max_bounds) * 0.5f;
    float3 to_center = aabb_center - cone_tip;
    float dist = length(to_center);
    
    if (dist < cone_radius) {
        float3 to_center_norm = to_center / dist;
        float angle = acos(dot(to_center_norm, cone_dir));
        return angle <= cone_angle;
    }
    
    return false;
}

/* =================================================================================================
 *                                    COMPUTE KERNEL
 * =================================================================================================
 */

/**
 * Assigns lights to clusters based on spatial intersection.
 * 
 * Each cluster processes all lights and builds a compact light index list.
 */
kernel void cluster_light_culling(
    constant CullingUniforms &uniforms               [[buffer(0)]],
    const device ClusterAABB *cluster_bounds          [[buffer(1)]],
    const device Light *lights                        [[buffer(2)]],
    device ClusterLightList *cluster_lists            [[buffer(3)]],
    device uint *light_indices                        [[buffer(4)]],
    device atomic_uint *global_light_index_counter    [[buffer(5)]],
    uint3 cluster_id                                  [[thread_position_in_grid]])
{
    // Calculate linear cluster index
    uint cluster_index = cluster_id.x + 
                        (cluster_id.y * uniforms.grid_dim_x) + 
                        (cluster_id.z * uniforms.grid_dim_x * uniforms.grid_dim_y);
    
    // Bounds check
    if (cluster_id.x >= uniforms.grid_dim_x ||
        cluster_id.y >= uniforms.grid_dim_y ||
        cluster_id.z >= uniforms.grid_dim_z) {
        return;
    }
    
    // Load cluster AABB
    ClusterAABB aabb = cluster_bounds[cluster_index];
    
    // Local light list (compacted afterwards)
    uint local_lights[256]; // Max lights per cluster
    uint local_light_count = 0;
    
    // Test all lights against this cluster
    for (uint i = 0; i < uniforms.total_lights; i++) {
        Light light = lights[i];
        bool intersects = false;
        
        if (light.type == 0) {
            // Directional light - affects all clusters
            intersects = true;
        }
        else if (light.type == 1) {
            // Point light - sphere test
            intersects = sphere_aabb_intersection(light.position, light.radius, aabb);
        }
        else if (light.type == 2) {
            // Spot light - cone test
            intersects = cone_aabb_intersection(light.position, light.direction, 
                                               light.radius, light.outer_cone, aabb);
        }
        
        if (intersects && local_light_count < uniforms.max_lights_per_cluster) {
            local_lights[local_light_count++] = i;
        }
    }
    
    // Write lights to global buffer
    if (local_light_count > 0) {
        // Atomically allocate space in global light index buffer
        uint global_offset = atomic_fetch_add_explicit(global_light_index_counter, 
                                                       local_light_count, 
                                                       memory_order_relaxed);
        
        // Write light indices
        for (uint i = 0; i < local_light_count; i++) {
            light_indices[global_offset + i] = local_lights[i];
        }
        
        // Update cluster metadata
        cluster_lists[cluster_index].offset = global_offset;
        cluster_lists[cluster_index].count = local_light_count;
    } else {
        cluster_lists[cluster_index].offset = 0;
        cluster_lists[cluster_index].count = 0;
    }
}

/**
 * Depth reduction kernel - calculates min/max depth per cluster.
 * 
 * Reads the depth buffer and determines depth bounds for each cluster.
 */
kernel void cluster_depth_reduction(
    constant CullingUniforms &uniforms               [[buffer(0)]],
    texture2d<float, access::read> depth_texture      [[texture(0)]],
    device ClusterLightList *cluster_lists            [[buffer(1)]],
    uint3 cluster_id                                  [[thread_position_in_grid]])
{
    // Calculate linear cluster index
    uint cluster_index = cluster_id.x + 
                        (cluster_id.y * uniforms.grid_dim_x) + 
                        (cluster_id.z * uniforms.grid_dim_x * uniforms.grid_dim_y);
    
    if (cluster_id.x >= uniforms.grid_dim_x ||
        cluster_id.y >= uniforms.grid_dim_y ||
        cluster_id.z >= uniforms.grid_dim_z) {
        return;
    }
    
    // Calculate screen-space bounds for this cluster
    uint2 depth_size = uint2(depth_texture.get_width(), depth_texture.get_height());
    uint tile_width = depth_size.x / uniforms.grid_dim_x;
    uint tile_height = depth_size.y / uniforms.grid_dim_y;
    
    uint min_x = cluster_id.x * tile_width;
    uint max_x = min((cluster_id.x + 1) * tile_width, depth_size.x);
    uint min_y = cluster_id.y * tile_height;
    uint max_y = min((cluster_id.y + 1) * tile_height, depth_size.y);
    
    // Find min/max depth in this tile
    float min_depth = 1.0f;
    float max_depth = 0.0f;
    
    for (uint y = min_y; y < max_y; y++) {
        for (uint x = min_x; x < max_x; x++) {
            float depth = depth_texture.read(uint2(x, y)).r;
            min_depth = min(min_depth, depth);
            max_depth = max(max_depth, depth);
        }
    }
    
    // Store depth bounds
    cluster_lists[cluster_index].min_depth = min_depth;
    cluster_lists[cluster_index].max_depth = max_depth;
}
