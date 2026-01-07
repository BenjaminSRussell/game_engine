/**
 * @file atmosphere_lut_parameterization.c
 * @brief LUT UV parameterization implementations
 */

#include "environment/atmosphere/sky/atmosphere_lut_parameterization.h"
#include "environment/atmosphere/sky/atmosphere_scattering.h"
#include <include/math/math.h>
#include <stdbool.h>

#define ATM_PI 3.14159265358979323846

// =============================================================================
// Transmittance LUT Parameterization
// =============================================================================

simd_float2 atm_transmittance_lut_encode(
    float height,
    float view_zenith_cos,
    float planet_radius,
    float atmo_height
) {
    // Normalize height to [0, 1]
    float h = (height - planet_radius) / atmo_height;
    h = fmaxf(0.0f, fminf(1.0f, h));
    
    // Apply non-linear mapping for better precision near ground
    float u_height = sqrtf(h); // Square root gives more precision at lower altitudes
    
    // For zenith angle, we need more precision near horizon
    // Map cos(theta) from [-1, 1] to [0, 1]
    float horizon_cos = atm_horizon_angle_cos(height, planet_radius);
    
    float v_zenith;
    if (view_zenith_cos > horizon_cos) {
        // Above horizon: map [horizon_cos, 1] to [0.5, 1]
        v_zenith = 0.5f + 0.5f * (view_zenith_cos - horizon_cos) / (1.0f - horizon_cos);
    } else {
        // Below horizon: map [-1, horizon_cos] to [0, 0.5]
        v_zenith = 0.5f * (view_zenith_cos + 1.0f) / (horizon_cos + 1.0f);
    }
    
    return simd_make_float2(u_height, v_zenith);
}

void atm_transmittance_lut_decode(
    simd_float2 uv,
    float planet_radius,
    float atmo_height,
    float* out_height,
    float* out_view_zenith_cos
) {
    // Decode height (inverse of square root)
    float h = uv.x * uv.x;
    *out_height = planet_radius + h * atmo_height;
    
    // Decode zenith angle
    float horizon_cos = atm_horizon_angle_cos(*out_height, planet_radius);
    
    if (uv.y > 0.5f) {
        // Above horizon
        float t = (uv.y - 0.5f) * 2.0f;
        *out_view_zenith_cos = horizon_cos + t * (1.0f - horizon_cos);
    } else {
        // Below horizon
        float t = uv.y * 2.0f;
        *out_view_zenith_cos = -1.0f + t * (horizon_cos + 1.0f);
    }
}

// =============================================================================
// Multi-Scattering LUT Parameterization
// =============================================================================

simd_float2 atm_multiscatter_lut_encode(
    float height,
    float sun_zenith_cos,
    float planet_radius,
    float atmo_height
) {
    // Height mapping (same as transmittance)
    float h = (height - planet_radius) / atmo_height;
    h = fmaxf(0.0f, fminf(1.0f, h));
    float u_height = sqrtf(h);
    
    // Sun zenith: simple linear mapping from [-1, 1] to [0, 1]
    float v_sun = (sun_zenith_cos + 1.0f) * 0.5f;
    
    return simd_make_float2(u_height, v_sun);
}

void atm_multiscatter_lut_decode(
    simd_float2 uv,
    float planet_radius,
    float atmo_height,
    float* out_height,
    float* out_sun_zenith_cos
) {
    // Decode height
    float h = uv.x * uv.x;
    *out_height = planet_radius + h * atmo_height;
    
    // Decode sun zenith
    *out_sun_zenith_cos = uv.y * 2.0f - 1.0f;
}

// =============================================================================
// Sky View LUT Parameterization
// =============================================================================

simd_float2 atm_skyview_lut_encode(
    simd_float3 view_dir,
    float view_height,
    float planet_radius,
    float atmo_height
) {
    // Calculate view zenith angle
    float view_zenith_cos = view_dir.y; // Assuming Y is up
    
    // Calculate view azimuth (0 to 2π)
    float view_azimuth = atan2f(view_dir.x, view_dir.z);
    if (view_azimuth < 0.0f) view_azimuth += 2.0f * ATM_PI;
    
    // Normalize height
    float h = (view_height - planet_radius) / atmo_height;
    h = fmaxf(0.0f, fminf(1.0f, h));
    
    // U coordinate: view zenith angle with horizon correction
    float horizon_cos = atm_horizon_angle_cos(view_height, planet_radius);
    
    float u;
    if (view_zenith_cos > horizon_cos) {
        // Above horizon
        u = 0.5f + 0.5f * sqrtf((view_zenith_cos - horizon_cos) / (1.0f - horizon_cos));
    } else {
        // Below horizon
        u = 0.5f * sqrtf((horizon_cos - view_zenith_cos) / (horizon_cos + 1.0f));
    }
    
    // V coordinate: azimuth angle
    float v = view_azimuth / (2.0f * ATM_PI);
    
    return simd_make_float2(u, v);
}

simd_float3 atm_skyview_lut_decode(
    simd_float2 uv,
    float view_height,
    float planet_radius,
    float atmo_height,
    simd_float3 up_dir
) {
    float horizon_cos = atm_horizon_angle_cos(view_height, planet_radius);
    
    // Decode zenith angle
    float view_zenith_cos;
    if (uv.x > 0.5f) {
        // Above horizon
        float t = (uv.x - 0.5f) * 2.0f;
        t = t * t; // Inverse of sqrt
        view_zenith_cos = horizon_cos + t * (1.0f - horizon_cos);
    } else {
        // Below horizon
        float t = uv.x * 2.0f;
        t = t * t; // Inverse of sqrt
        view_zenith_cos = horizon_cos - t * (horizon_cos + 1.0f);
    }
    
    // Decode azimuth
    float view_azimuth = uv.y * 2.0f * ATM_PI;
    
    // Construct view direction
    float view_zenith_sin = sqrtf(fmaxf(0.0f, 1.0f - view_zenith_cos * view_zenith_cos));
    
    simd_float3 view_dir;
    view_dir.x = view_zenith_sin * sinf(view_azimuth);
    view_dir.y = view_zenith_cos;
    view_dir.z = view_zenith_sin * cosf(view_azimuth);
    
    return simd_normalize(view_dir);
}

// =============================================================================
// Camera Volume LUT Parameterization
// =============================================================================

simd_float3 atm_camera_volume_encode(
    simd_float3 world_pos,
    simd_float3 camera_pos,
    simd_float4x4 inv_view_proj,
    float near_plane,
    float far_plane,
    float froxel_depth_distribution
) {
    // Calculate view-space depth
    simd_float3 view_vec = world_pos - camera_pos;
    float distance = simd_length(view_vec);
    
    // Normalize distance to [0, 1]
    float t = (distance - near_plane) / (far_plane - near_plane);
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    // Apply distribution (0 = linear, 1 = exponential)
    float depth_uv;
    if (froxel_depth_distribution > 0.5f) {
        // Exponential distribution (more slices near camera)
        depth_uv = 1.0f - expf(-t * 4.0f) / (1.0f - expf(-4.0f));
    } else {
        // Linear distribution
        depth_uv = t;
    }
    
    // For XY, we would need to project to screen space
    // This is a simplified version - in practice, use proper projection
    simd_float3 dir = simd_normalize(view_vec);
    
    // Simple spherical mapping for demonstration
    float u = atan2f(dir.x, dir.z) / (2.0f * ATM_PI) + 0.5f;
    float v = asinf(dir.y) / ATM_PI + 0.5f;
    
    return simd_make_float3(u, v, depth_uv);
}

// =============================================================================
// Helper Functions
// =============================================================================

float atm_distance_to_atmosphere_boundary(
    simd_float3 pos,
    simd_float3 dir,
    float planet_radius,
    float atmo_height,
    bool* intersect_ground
) {
    simd_float3 planet_center = simd_make_float3(0.0f, 0.0f, 0.0f);
    
    float t_ground1, t_ground2;
    bool hits_ground = atm_ray_sphere_intersect(pos, dir, planet_center, planet_radius, &t_ground1, &t_ground2);
    
    if (intersect_ground) {
        *intersect_ground = hits_ground && (t_ground1 > 0.0f || t_ground2 > 0.0f);
    }
    
    // Check intersection with atmosphere boundary
    float t_atmo1, t_atmo2;
    bool hits_atmo = atm_ray_sphere_intersect(pos, dir, planet_center, planet_radius + atmo_height, &t_atmo1, &t_atmo2);
    
    if (!hits_atmo) {
        return 0.0f; // Outside atmosphere, no intersection
    }
    
    // Return the farthest positive intersection
    float t = (t_atmo2 > 0.0f) ? t_atmo2 : t_atmo1;
    
    // If we hit ground first, return that distance
    if (hits_ground && t_ground1 > 0.0f && t_ground1 < t) {
        t = t_ground1;
    }
    
    return fmaxf(0.0f, t);
}

float atm_horizon_angle_cos(float height, float planet_radius) {
    if (height <= planet_radius) {
        return -1.0f; // Below surface
    }
    
    // Cosine of horizon angle from Pythagoras
    // cos(θ) = -sqrt(h² - r²) / h
    // where h is height from center, r is planet radius
    float h = height;
    float r = planet_radius;
    
    float discriminant = h * h - r * r;
    if (discriminant <= 0.0f) {
        return -1.0f;
    }
    
    return -sqrtf(discriminant) / h;
}

float atm_nonlinear_map(float x, float power) {
    x = fmaxf(0.0f, fminf(1.0f, x));
    return powf(x, power);
}

float atm_nonlinear_unmap(float y, float power) {
    y = fmaxf(0.0f, fminf(1.0f, y));
    if (power == 0.0f) return y;
    return powf(y, 1.0f / power);
}
