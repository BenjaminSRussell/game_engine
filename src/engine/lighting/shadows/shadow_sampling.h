/*
 * shadow_sampling.h
 * Shader-side shadow sampling API definitions
 * Used for shared structs between C and Metal
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SHADOW_SAMPLING_H
#define LIGHTING_SHADOW_SAMPLING_H

#include <simd/simd.h>

typedef struct {
    matrix_float4x4 view_proj;
    float split_distance;
    float pad[3];
} ShadowCascadeData;

typedef struct {
    ShadowCascadeData cascades[4];
    uint32_t cascade_count;
    float shadow_bias;
    float blend_region;
    float shadow_fade;
} ShadowUniforms;

#endif /* LIGHTING_SHADOW_SAMPLING_H */
