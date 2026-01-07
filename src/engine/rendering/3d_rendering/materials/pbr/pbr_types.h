/*
 * pbr_types.h
 * PBR Material Types
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PBR_TYPES_H
#define MATERIALS_PBR_TYPES_H

#include <simd/simd.h>
#include <Metal/Metal.h>
#include "../../backend/metal/metal/mtl_device.h"

typedef struct pbr_material_params {
    simd_float4 albedo_factor;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;
    simd_float3 emissive_factor;
    float alpha_cutoff;
    // Padding to ensure 16-byte alignment if necessary, though simd types handle most alignment
    float _pad; 
} pbr_material_params_t;

typedef struct material_defaults {
    id<MTLTexture> white_1x1;
    id<MTLTexture> normal_1x1;  // (0.5, 0.5, 1.0)
    id<MTLTexture> black_1x1;
    id<MTLSamplerState> default_sampler;
} material_defaults_t;

typedef struct material {
    char name[64];
    pbr_material_params_t params;

    // Textures (can be NULL for defaults)
    id<MTLTexture> albedo_texture;
    id<MTLTexture> normal_texture;
    id<MTLTexture> metallic_roughness_texture;
    id<MTLTexture> occlusion_texture;
    id<MTLTexture> emissive_texture;

    // GPU buffer for params
    id<MTLBuffer> param_buffer;

    // Flags
    uint32_t flags;
    bool double_sided;
    bool alpha_blend;
} material_t;

#endif /* MATERIALS_PBR_TYPES_H */
