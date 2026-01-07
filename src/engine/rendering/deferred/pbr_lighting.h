/**
 * =================================================================================================
 *                                 PBR LIGHTING HEADER
 * =================================================================================================
 *
 * Purpose: Defines constants and structures for physically-based rendering.
 * Aligned with the Cook-Torrance BRDF and metallic/roughness workflow.
 */

#pragma once

#ifndef PI
#define PI 3.14159265359f
#endif

#include <stdbool.h>
#include "/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/include/core/types.h"
#include <simd/simd.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#ifndef PI
#define PI 3.14159265359f
#endif

#define MIN_ROUGHNESS 0.045f
#define DIELECTRIC_F0 0.04f

/* =================================================================================================
 *                                    TYPE DEFINITIONS
 * =================================================================================================
 */

/**
 * Surface properties derived from G-Buffer and material parameters.
 */
typedef struct {
    simd_float3 albedo;
    f32 metallic;
    f32 roughness;
    f32 alpha;              // Alpha^2 (used in GGX)
    
    simd_float3 normal;     // World-space normal
    simd_float3 view_dir;   // World-space view direction
    simd_float3 f0;         // Fresnel at normal incidence
    
    f32 n_dot_v;            // cos(theta) for view direction
    simd_float3 position;   // World-space position
} SurfaceProperties;

/**
 * Lighting contribution for a single light source.
 */
typedef struct {
    simd_float3 diffuse;
    simd_float3 specular;
} LightContribution;

/* =================================================================================================
 *                                    UTILITIES
 * =================================================================================================
 */

/**
 * Calculates F0 based on albedo and metallic value.
 */
static inline simd_float3 calculate_f0(simd_float3 albedo, f32 metallic) {
    simd_float3 dielectric = {DIELECTRIC_F0, DIELECTRIC_F0, DIELECTRIC_F0};
    return simd_mix(dielectric, albedo, metallic);
}

/**
 * Remaps perceptual roughness to alpha (roughness squared).
 */
static inline f32 perceptual_roughness_to_alpha(f32 roughness) {
    f32 r = fmaxf(roughness, MIN_ROUGHNESS);
    return r * r;
}
