/*
 * volumetric_fog.h
 * Volumetric fog rendering
 */

#ifndef LIGHTING_VOLUMETRIC_FOG_H
#define LIGHTING_VOLUMETRIC_FOG_H

#include <stdint.h>
#include <stdbool.h>
#include <simd/simd.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#include "../../backend/metal/metal/mtl_device.h"
#include "../../rendering/deferred/gbuffer_layout.h"
#include "froxel_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct volumetric_fog {
#ifdef __OBJC__
    id<MTLTexture> froxel_scattering;     // RGBA16F: inscatter.rgb, transmittance.a
    id<MTLTexture> froxel_density;        // R16F: density
    id<MTLTexture> integrated_scattering; // Result of integration

    id<MTLComputePipelineState> inject_pipeline;
    id<MTLComputePipelineState> raymarch_pipeline;
    id<MTLComputePipelineState> integrate_pipeline;
#else
    void* froxel_scattering;
    void* froxel_density;
    void* integrated_scattering;

    void* inject_pipeline;
    void* raymarch_pipeline;
    void* integrate_pipeline;
#endif

    simd_uint3 resolution;  // e.g., 160x90x128
    float near_plane;
    float far_plane;
    float density_scale;
    simd_float3 fog_color;
} volumetric_fog_t;

// Opaque light system and camera handles for C interop
typedef struct light_system light_system_t;
typedef struct Camera camera_t;
typedef struct shadow_map_system shadow_map_system_t;

/**
 * Creates the volumetric fog system.
 */
volumetric_fog_t* volumetric_fog_create(metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d);

/**
 * Destroys the volumetric fog system.
 */
void volumetric_fog_destroy(volumetric_fog_t* fog);

/**
 * Renders the volumetric fog.
 */
#ifdef __OBJC__
void volumetric_fog_render(volumetric_fog_t* fog, id<MTLCommandBuffer> cmd,
                           light_system_t* lights, camera_t* camera,
                           shadow_map_system_t* shadows);
#else
void volumetric_fog_render(volumetric_fog_t* fog, void* cmd,
                           light_system_t* lights, camera_t* camera,
                           shadow_map_system_t* shadows);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOLUMETRIC_FOG_H */
