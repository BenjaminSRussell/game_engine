/**
 * =================================================================================================
 *                              MAIN DEFERRED LIGHTING PASS
 * =================================================================================================
 *
 * Purpose: Core lighting shader that integrates G-Buffer data, clustered light culling, 
 * PBR BRDF, light evaluation, and shadow sampling.
 */

#include <metal_stdlib>
using namespace metal;

// Add inclusions for our library functions (in practice these would be in separate files 
// and bundled or included via preprocessor if the environment supports it)
#include "pbr_brdf.metal"
#include "light_evaluation.metal"
#include "shadow_sampling.metal"

/* =================================================================================================
 *                                    STRUCTURES
 * =================================================================================================
 */

struct GBufferTextures {
    texture2d<float> albedo      [[texture(0)]];
    texture2d<float> normal      [[texture(1)]];
    texture2d<float> material    [[texture(2)]];
    texture2d<float> depth       [[texture(3)]];
    texture2d<float> emissive    [[texture(4)]];
};

struct ClusterData {
    const device ClusterAABB *bounds          [[buffer(0)]];
    const device ClusterLightList *lists      [[buffer(1)]];
    const device uint *light_indices          [[buffer(2)]];
};

struct FrameUniforms {
    float4x4 inv_view_proj;
    float4x4 view;
    float3 camera_pos;
    float screen_width;
    float screen_height;
    uint grid_dim_x;
    uint grid_dim_y;
    uint grid_dim_z;
    float near_plane;
    float far_plane;
    uint depth_slice_mode;
};

/* =================================================================================================
 *                                    UTILITIES
 * =================================================================================================
 */

/**
 * Reconstructs world-space position from depth and screen UV.
 */
float3 reconstruct_world_pos(float2 uv, float depth, float4x4 inv_view_proj) {
    float4 clip_pos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip_pos.y *= -1.0; // Flip Y for Metal
    
    float4 world_pos = inv_view_proj * clip_pos;
    return world_pos.xyz / world_pos.w;
}

/**
 * Calculates cluster index from screen position and view-space depth.
 */
uint get_cluster_index(float2 screen_pos, float view_z, constant FrameUniforms &frame) {
    uint tile_x = uint(screen_pos.x / frame.screen_width * frame.grid_dim_x);
    uint tile_y = uint(screen_pos.y / frame.screen_height * frame.grid_dim_y);
    
    uint tile_z = 0;
    if (frame.depth_slice_mode == 0) { // Linear
        float t = (view_z - frame.near_plane) / (frame.far_plane - frame.near_plane);
        tile_z = uint(saturate(t) * frame.grid_dim_z);
    } else { // Exponential
        float t = log(view_z / frame.near_plane) / log(frame.far_plane / frame.near_plane);
        tile_z = uint(saturate(t) * frame.grid_dim_z);
    }
    
    return tile_x + (tile_y * frame.grid_dim_x) + (tile_z * frame.grid_dim_x * frame.grid_dim_y);
}

/* =================================================================================================
 *                                    MAIN FRAGMENT SHADER
 * =================================================================================================
 */

fragment float4 deferred_lighting_pass(
    float4 screen_pos [[position]],
    constant FrameUniforms &frame                     [[buffer(0)]],
    constant GBufferTextures &gbuffer                 [[buffer(1)]],
    constant ClusterData &clusters                    [[buffer(2)]],
    constant Light *lights                            [[buffer(3)]],
    constant ShadowParams &shadow_params              [[buffer(4)]],
    depth2d_array<float> shadow_maps                  [[texture(5)]],
    sampler default_sampler                           [[sampler(0)]],
    sampler shadow_sampler                            [[sampler(1)]])
{
    float2 uv = screen_pos.xy / float2(frame.screen_width, frame.screen_height);
    
    // 1. Sample G-Buffer
    float depth = gbuffer.depth.sample(default_sampler, uv).r;
    if (depth >= 1.0) discard_fragment(); // Skip background
    
    float4 albedo_sample = gbuffer.albedo.sample(default_sampler, uv);
    float3 albedo = albedo_sample.rgb;
    
    float3 normal_encoded = gbuffer.normal.sample(default_sampler, uv).rgb;
    float3 normal = normalize(normal_encoded * 2.0 - 1.0);
    
    float4 material = gbuffer.material.sample(default_sampler, uv);
    float metallic = material.r;
    float roughness = material.g;
    float ao = material.b;
    
    float3 emissive = gbuffer.emissive.sample(default_sampler, uv).rgb;
    
    // 2. Reconstruct Geometry
    float3 world_pos = reconstruct_world_pos(uv, depth, frame.inv_view_proj);
    float4 view_pos = frame.view * float4(world_pos, 1.0);
    float view_z = -view_pos.z; // View-space depth (positive Z)
    
    float3 V = normalize(frame.camera_pos - world_pos);
    
    // 3. Clustered Light Lookup
    uint cluster_idx = get_cluster_index(screen_pos.xy, view_z, frame);
    ClusterLightList list = clusters.lists[cluster_idx];
    
    float3 direct_lighting = float3(0.0);
    
    // 4. Iterate over lights in cluster
    for (uint i = 0; i < list.count; i++) {
        uint light_idx = clusters.light_indices[list.offset + i];
        Light light = lights[light_idx];
        
        // Evaluate light properties
        float3 L, radiance;
        float dist;
        evaluate_light(light, world_pos, L, radiance, dist);
        
        if (any(radiance > 0.0)) {
            // Shadowing
            float shadow = 1.0;
            if (light.cast_shadows != 0) {
                // For simplicity, we assume point/spot lights use specific shadow mapping 
                // but here we use the global shadow params for demonstration
                shadow = evaluate_shadow(shadow_maps, shadow_sampler, world_pos, view_z, shadow_params);
            }
            
            // PBR Shading
            if (shadow > 0.0) {
                direct_lighting += shade_pbr(L, V, normal, albedo, metallic, roughness) * radiance * shadow;
            }
        }
    }
    
    // 5. Global Illumination & Ambient (Simplified)
    float3 ambient = albedo * 0.03 * ao; // Placeholder for real GI
    
    // 6. Accumulate and Final Output
    float3 final_color = direct_lighting + ambient + emissive;
    
    return float4(final_color, 1.0);
}
