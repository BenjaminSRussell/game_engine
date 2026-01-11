#ifndef ATMOSPHERE_LUT_PARAMETERIZATION_H
#define ATMOSPHERE_LUT_PARAMETERIZATION_H

#include "../../../include/math/math.h"
#include <simd/simd.h>
#include <stdbool.h>

#define ATM_PI 3.14159265359f

/**
 * @file atmosphere_lut_parameterization.h
 * @brief LUT UV parameterization functions
 *
 * Provides non-linear parameterization for atmospheric LUTs to achieve
 * better precision where it matters (near horizon, etc.)
 */

// =============================================================================
// Transmittance LUT Parameterization
// =============================================================================

/**
 * Map (height, view_zenith_cos) to transmittance LUT UV
 *
 * Uses non-linear mapping to increase precision near horizon
 *
 * @param height Distance from planet center (km)
 * @param view_zenith_cos Cosine of view zenith angle
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @return UV coordinates in transmittance LUT [0,1]²
 */
simd_float2 atm_transmittance_lut_encode(float height, float view_zenith_cos,
                                         float planet_radius,
                                         float atmo_height);

/**
 * Map transmittance LUT UV to (height, view_zenith_cos)
 *
 * Inverse of atm_transmittance_lut_encode
 *
 * @param uv UV coordinates in transmittance LUT [0,1]²
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @param out_height Output: distance from planet center
 * @param out_view_zenith_cos Output: cosine of view zenith angle
 */
void atm_transmittance_lut_decode(simd_float2 uv, float planet_radius,
                                  float atmo_height, float *out_height,
                                  float *out_view_zenith_cos);

// =============================================================================
// Multi-Scattering LUT Parameterization
// =============================================================================

/**
 * Map (height, sun_zenith_cos) to multi-scattering LUT UV
 *
 * @param height Distance from planet center (km)
 * @param sun_zenith_cos Cosine of sun zenith angle
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @return UV coordinates in multi-scattering LUT [0,1]²
 */
simd_float2 atm_multiscatter_lut_encode(float height, float sun_zenith_cos,
                                        float planet_radius, float atmo_height);

/**
 * Map multi-scattering LUT UV to (height, sun_zenith_cos)
 *
 * @param uv UV coordinates [0,1]²
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @param out_height Output: distance from planet center
 * @param out_sun_zenith_cos Output: cosine of sun zenith angle
 */
void atm_multiscatter_lut_decode(simd_float2 uv, float planet_radius,
                                 float atmo_height, float *out_height,
                                 float *out_sun_zenith_cos);

// =============================================================================
// Sky View LUT Parameterization
// =============================================================================

/**
 * Map view direction to sky view LUT UV
 *
 * The sky view LUT stores pre-integrated sky color for every view direction
 * from the current camera position.
 *
 * @param view_dir View direction (world space, normalized)
 * @param view_height Camera height from planet center (km)
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @return UV coordinates in sky view LUT [0,1]²
 */
simd_float2 atm_skyview_lut_encode(simd_float3 view_dir, float view_height,
                                   float planet_radius, float atmo_height);

/**
 * Map sky view LUT UV to view direction
 *
 * @param uv UV coordinates [0,1]²
 * @param view_height Camera height from planet center (km)
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @param up_dir World up direction (typically (0,1,0))
 * @return View direction (normalized)
 */
simd_float3 atm_skyview_lut_decode(simd_float2 uv, float view_height,
                                   float planet_radius, float atmo_height,
                                   simd_float3 up_dir);

// =============================================================================
// Camera Volume LUT Parameterization (Froxels)
// =============================================================================

/**
 * Map world position to camera volume froxel UVW
 *
 * Froxels are frustum-aligned voxels for aerial perspective
 *
 * @param world_pos World position
 * @param camera_pos Camera position
 * @param inv_view_proj Inverse view-projection matrix
 * @param near_plane Near plane distance
 * @param far_plane Far plane distance
 * @param froxel_depth_distribution Depth distribution (0=linear, 1=exponential)
 * @return UVW coordinates in camera volume [0,1]³
 */
simd_float3 atm_camera_volume_encode(simd_float3 world_pos,
                                     simd_float3 camera_pos,
                                     simd_float4x4 inv_view_proj,
                                     float near_plane, float far_plane,
                                     float froxel_depth_distribution);

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Calculate distance to atmosphere boundary from a point
 *
 * @param pos Position (km from planet center)
 * @param dir Direction (normalized)
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @param intersect_ground Output: true if ray hits ground
 * @return Distance to atmosphere boundary (km)
 */
float atm_distance_to_atmosphere_boundary(simd_float3 pos, simd_float3 dir,
                                          float planet_radius,
                                          float atmo_height,
                                          bool *intersect_ground);

/**
 * Calculate horizon angle from a given height
 *
 * @param height Distance from planet center (km)
 * @param planet_radius Planet radius (km)
 * @return Cosine of horizon angle
 */
float atm_horizon_angle_cos(float height, float planet_radius);

/**
 * Apply non-linear mapping for better horizon precision
 *
 * @param x Input value [0,1]
 * @param power Power for non-linearity (typical: 2.0-5.0)
 * @return Remapped value [0,1]
 */
float atm_nonlinear_map(float x, float power);

/**
 * Inverse of atm_nonlinear_map
 *
 * @param y Mapped value [0,1]
 * @param power Power used in forward mapping
 * @return Original value [0,1]
 */
float atm_nonlinear_unmap(float y, float power);

#endif // ATMOSPHERE_LUT_PARAMETERIZATION_H
