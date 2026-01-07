/*
 * shadow_sampling.metal
 * Metal shader implementation for CSM
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include <metal_stdlib>
#include "shadow_sampling.h"

using namespace metal;

// ----------------------------------------------------------------------------
// UTILS
// ----------------------------------------------------------------------------

// PCF Sampling 3x3
float sample_shadow_pcf(depth2d<float> shadow_map, sampler s, float2 uv, float compare_depth) {
    float shadow = 0.0f;
    float2 texel_size = float2(1.0f) / float2(shadow_map.get_width(), shadow_map.get_height());
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texel_size;
            shadow += shadow_map.sample_compare(s, uv + offset, compare_depth);
        }
    }
    return shadow / 9.0f;
}

// ----------------------------------------------------------------------------
// CSM LOGIC
// ----------------------------------------------------------------------------

float calculate_cascaded_shadow(
    depth2d<float> shadow_map_atlas,
    sampler shadow_sampler,
    constant ShadowUniforms& uniforms,
    float3 world_pos,
    float view_depth_linear
) {
    // 1. Select Cascade
    uint cascade_idx = uniforms.cascade_count - 1;
    for (uint i = 0; i < uniforms.cascade_count; i++) {
        if (view_depth_linear < uniforms.cascades[i].split_distance) {
            cascade_idx = i;
            break;
        }
    }
    
    // 2. Project to shadow space
    float4 shadow_pos = uniforms.cascades[cascade_idx].view_proj * float4(world_pos, 1.0f);
    shadow_pos.xy /= shadow_pos.w;
    
    // NDC -> UV [0,1] with flip Y
    float2 uv = shadow_pos.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    
    float current_depth = shadow_pos.z / shadow_pos.w;
    
    // 3. Sample
    // Bias
    float bias = uniforms.shadow_bias;
    
    // Basic PCF
    float shadow = sample_shadow_pcf(shadow_map_atlas, shadow_sampler, uv, current_depth - bias);
    
    // 4. Cascade Blending (Optional)
    // If near the edge of split distance, we sample next cascade and blend
    
    return shadow;
}
