/**
 * =================================================================================================
 *                                 SHADOW SAMPLING SHADER
 * =================================================================================================
 *
 * Purpose: Implements shadow map sampling, PCF filtering, and cascade blending.
 */

#include <metal_stdlib>
using namespace metal;

/* =================================================================================================
 *                                    STRUCTURES
 * =================================================================================================
 */

struct ShadowParams {
    float4x4 shadow_matrix[4];      // Shadow matrices for up to 4 cascades
    float4 cascade_splits;          // Distance splits for cascades
    float shadow_bias;
    float shadow_texel_size;
    uint shadow_type;               // 0=hard, 1=pcf, 2=pcss
    uint cascade_count;
};

/* =================================================================================================
 *                                    PCF FILTERING
 * =================================================================================================
 */

/**
 * Basic 4-tap PCF (Percentage Closer Filtering).
 */
float sample_shadow_pcf_4(
    depth2d_array<float> shadow_map, 
    sampler shadow_sampler,
    float3 shadow_coords, 
    uint cascade_index,
    float texel_size) 
{
    float shadow = 0.0;
    
    // 2x2 PCF kernel
    for (int y = -1; y <= 0; y++) {
        for (int x = -1; x <= 0; x++) {
            float2 offset = float2(x, y) * texel_size;
            shadow += shadow_map.sample_compare(shadow_sampler, shadow_coords.xy + offset, cascade_index, shadow_coords.z);
        }
    }
    
    return shadow / 4.0;
}

/**
 * Optimized 9-tap PCF.
 */
float sample_shadow_pcf_9(
    depth2d_array<float> shadow_map, 
    sampler shadow_sampler,
    float3 shadow_coords, 
    uint cascade_index,
    float texel_size) 
{
    float shadow = 0.0;
    
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float2 offset = float2(x, y) * texel_size;
            shadow += shadow_map.sample_compare(shadow_sampler, shadow_coords.xy + offset, cascade_index, shadow_coords.z);
        }
    }
    
    return shadow / 9.0;
}

/* =================================================================================================
 *                                    CASCADE SELECTION
 * =================================================================================================
 */

/**
 * Selects the appropriate shadow cascade based on view-space depth.
 */
uint select_shadow_cascade(float view_depth, float4 splits, uint count) {
    uint cascade_index = count - 1;
    
    for (uint i = 0; i < count; i++) {
        if (view_depth < splits[i]) {
            cascade_index = i;
            break;
        }
    }
    
    return cascade_index;
}

/* =================================================================================================
 *                                    SHADOW EVALUATION
 * =================================================================================================
 */

/**
 * Evaluates the shadow factor (0=shadow, 1=lit) for a world-space position.
 */
float evaluate_shadow(
    depth2d_array<float> shadow_map,
    sampler shadow_sampler,
    float3 world_pos,
    float view_depth,
    constant ShadowParams &params)
{
    // Select cascade
    uint cascade_index = select_shadow_cascade(view_depth, params.cascade_splits, params.cascade_count);
    
    // Project into shadow space
    float4 shadow_pos = params.shadow_matrix[cascade_index] * float4(world_pos, 1.0);
    float3 shadow_coords = shadow_pos.xyz / shadow_pos.w;
    
    // Bias
    shadow_coords.z -= params.shadow_bias;
    
    // Bounds check
    if (shadow_coords.x < 0.0 || shadow_coords.x > 1.0 || 
        shadow_coords.y < 0.0 || shadow_coords.y > 1.0) {
        return 1.0;
    }
    
    // Sample shadow map with PCF
    if (params.shadow_type == 0) {
        return shadow_map.sample_compare(shadow_sampler, shadow_coords.xy, cascade_index, shadow_coords.z);
    } else if (params.shadow_type == 1) {
        return sample_shadow_pcf_4(shadow_map, shadow_sampler, shadow_coords, cascade_index, params.shadow_texel_size);
    } else {
        return sample_shadow_pcf_9(shadow_map, shadow_sampler, shadow_coords, cascade_index, params.shadow_texel_size);
    }
}
