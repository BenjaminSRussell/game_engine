#ifndef ATMOSPHERE_COMMON_H
#define ATMOSPHERE_COMMON_H

#ifdef __METAL_VERSION__
#include <metal_stdlib>
using namespace metal;
#else
#include <simd/simd.h>
#endif

// LUT Dimensions
#define TRANSMITTANCE_WIDTH 256
#define TRANSMITTANCE_HEIGHT 64

#define SCATTERING_WIDTH 32
#define SCATTERING_HEIGHT 128
#define SCATTERING_DEPTH 32

#define MULTISCATTER_WIDTH 32
#define MULTISCATTER_HEIGHT 32

#define SKYVIEW_WIDTH 192
#define SKYVIEW_HEIGHT 108

#define CAMERA_VOLUME_WIDTH 32
#define CAMERA_VOLUME_HEIGHT 32
#define CAMERA_VOLUME_DEPTH 32

#define IRRADIANCE_WIDTH 64
#define IRRADIANCE_HEIGHT 16

typedef struct {
    float planet_radius;           // 6360 km
    float atmosphere_height;       // 100 km
    simd_float3 rayleigh_coeff;    // Scattering coefficients
    float rayleigh_scale_height;   // 8 km
    float mie_coeff;
    float mie_scale_height;        // 1.2 km
    float mie_g;                   // Asymmetry factor
    simd_float3 sun_direction;
    float padding0;
    simd_float3 sun_intensity;
    float padding1;
} AtmosphereParams;

typedef struct {
    simd_float4x4 inv_view_proj;
    simd_float3 camera_pos;
    float planet_radius;
    simd_float3 sun_direction;
    float padding0;
    simd_float3 sun_intensity;
    float padding1;
    simd_float2 screen_size;
} SkyUniforms;

#endif // ATMOSPHERE_COMMON_H
