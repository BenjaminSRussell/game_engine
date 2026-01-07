#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#ifdef __OBJC__
#include <Metal/Metal.h>
#endif

#include <simd/simd.h>
#include "atmosphere_common.h"

// Forward declarations
typedef struct metal_device metal_device_t;
typedef struct camera camera_t;

#ifdef __OBJC__
#define ATMO_MTL_Texture id<MTLTexture>
#define ATMO_MTL_ComputePipelineState id<MTLComputePipelineState>
#define ATMO_MTL_RenderPipelineState id<MTLRenderPipelineState>
#else
#define ATMO_MTL_Texture void*
#define ATMO_MTL_ComputePipelineState void*
#define ATMO_MTL_RenderPipelineState void*
#endif

// Atmosphere System
typedef struct atmosphere_system {
    // LUT Textures
    ATMO_MTL_Texture transmittance_lut;    // 256x64
    ATMO_MTL_Texture scattering_lut;       // 32x128x32 (3D)
    ATMO_MTL_Texture multiscatter_lut;     // 32x32 (new)
    ATMO_MTL_Texture skyview_lut;          // 192x108 (new)
    ATMO_MTL_Texture camera_volume_lut;    // 32x32x32 (new)
    ATMO_MTL_Texture irradiance_lut;       // 64x16

    // Pipelines
    ATMO_MTL_ComputePipelineState transmittance_pipeline;
    ATMO_MTL_ComputePipelineState scattering_pipeline;
    ATMO_MTL_ComputePipelineState multiscatter_pipeline;  // new
    ATMO_MTL_ComputePipelineState skyview_pipeline;       // new
    ATMO_MTL_ComputePipelineState camera_volume_pipeline; // new
    ATMO_MTL_RenderPipelineState sky_pipeline;
    ATMO_MTL_RenderPipelineState aerial_perspective_pipeline;

    // Parameters
    float planet_radius;           // km
    float atmosphere_height;       // km
    simd_float3 rayleigh_coeff;    // Scattering coefficients
    float rayleigh_scale_height;   // km
    float mie_coeff;
    float mie_scale_height;        // km
    float mie_g;                   // Asymmetry factor
    simd_float3 sun_direction;
    simd_float3 sun_intensity;
} atmosphere_system_t;

// API
#ifdef __OBJC__
#include <Metal/Metal.h>
void atmosphere_precompute(atmosphere_system_t* atmo, id<MTLCommandBuffer> cmd);
void atmosphere_render_sky(atmosphere_system_t* atmo, id<MTLRenderCommandEncoder> encoder, camera_t* camera);
void atmosphere_render_aerial_perspective(atmosphere_system_t* atmo, id<MTLRenderCommandEncoder> encoder, camera_t* camera, id<MTLTexture> depth_tex);
#else
void atmosphere_precompute(atmosphere_system_t* atmo, void* cmd);
void atmosphere_render_sky(atmosphere_system_t* atmo, void* encoder, camera_t* camera);
void atmosphere_render_aerial_perspective(atmosphere_system_t* atmo, void* encoder, camera_t* camera, void* depth_tex);
#endif

void atmosphere_init(atmosphere_system_t* atmo, metal_device_t* dev);
void atmosphere_shutdown(atmosphere_system_t* atmo);
void atmosphere_update_params(atmosphere_system_t* atmo, const AtmosphereParams* params);

#endif // ATMOSPHERE_SYSTEM_H
