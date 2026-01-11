#include "atmosphere_lut_parameterization.h"
#include <math.h>

#ifndef ATM_PI
#define ATM_PI 3.14159265359f
#endif

// =============================================================================
// Helper Functions
// =============================================================================

float atm_distance_to_atmosphere_boundary(simd_float3 pos, simd_float3 dir,
                                          float planet_radius,
                                          float atmo_height,
                                          bool *intersect_ground) {
  float r = simd_length(pos);
  float r_atm = planet_radius + atmo_height;

  // Ray-sphere intersection with atmosphere top
  float b = 2.0f * simd_dot(dir, pos);
  float c = r * r - r_atm * r_atm;
  float det = b * b - 4.0f * c;

  float dist_to_top = -1.0f;
  if (det >= 0.0f) {
    dist_to_top = (-b + sqrtf(det)) * 0.5f;
  }

  // Ray-sphere intersection with ground
  float c_ground = r * r - planet_radius * planet_radius;
  float det_ground = b * b - 4.0f * c_ground;

  if (intersect_ground)
    *intersect_ground = false;

  if (det_ground >= 0.0f) {
    float t_ground = (-b - sqrtf(det_ground)) * 0.5f;
    if (t_ground > 0.0f && (dist_to_top < 0.0f || t_ground < dist_to_top)) {
      if (intersect_ground)
        *intersect_ground = true;
      return t_ground;
    }
  }

  return dist_to_top; // Can be negative if outside looking away, but we assume
                      // inside/near
}

float atm_horizon_angle_cos(float height, float planet_radius) {
  // Cosine of angle to horizon from height r (from planet center)
  // sin(alpha) = R / r
  // cos(alpha) = -sqrt(1 - (R/r)^2) ?
  // Horizon is where ray touches the sphere tangent.
  // cos(theta_h) = -sqrt(r^2 - R^2) / r
  if (height < planet_radius)
    return -1.0f;
  float mu_h =
      -sqrtf(fmaxf(0.0f, height * height - planet_radius * planet_radius)) /
      height;
  return mu_h;
}

// =============================================================================
// Transmittance LUT Parameterization
// =============================================================================

// Based on Bruneton's improved parameterization (2017/Hillaire 2020)
// Texture coords: u = zenith axis, v = height axis

simd_float2 atm_transmittance_lut_encode(float height, float view_zenith_cos,
                                         float planet_radius,
                                         float atmo_height) {
  // Normalize height to [0,1]
  // Note: Parameterization usually uses non-linear mapping for height too?
  // Let's use simple linear height for v, and non-linear zenith for u.
  // Actually, Bruneton uses dist-to-horizon mappings.
  // Simplifying for this implementation:

  float H = sqrtf(fmaxf(0.0f, (planet_radius + atmo_height) *
                                      (planet_radius + atmo_height) -
                                  planet_radius * planet_radius));
  float rho =
      sqrtf(fmaxf(0.0f, height * height - planet_radius * planet_radius));

  float discriminant =
      height * height * (view_zenith_cos * view_zenith_cos - 1.0f) +
      (planet_radius + atmo_height) * (planet_radius + atmo_height);
  float d =
      fmaxf(0.0f, -height * view_zenith_cos + sqrtf(fmaxf(0.0f, discriminant)));

  float d_min = (planet_radius + atmo_height) - height;
  float d_max = rho + H;

  float u = (d - d_min) / (d_max - d_min);
  float v = (height - planet_radius) / atmo_height;

  // Clamp
  // Use the horizon-based mapping if possible, but the above 'd' based is from
  // Hillaire. However, let's stick to the Hillaire 2020 mapping which is
  // robust.

  // Mapping 1 (Height):
  // r = P + v * A -> v = (r - P) / A
  // Just linear height is often enough for transmittance if resolution height
  // is adequate (64).

  // Mapping 2 (Angle):
  // u = 0.5 + 0.5 * view_zenith_cos? No, needs more resolution at horizon.

  // Re-implementing Hillaire 2020 standard mapping for Transmittance:
  // u: distance to top of atmosphere / max distance
  // v: distance to horizon? No.

  // Let's implement the simpler one from referenced headers if they exist, or a
  // standard safe one. Using mapping: u = (atan(mu) + PI/2) / PI ?
  u = (atanf(view_zenith_cos * 10.0f) / (ATM_PI / 2.0f)) * 0.5f +
      0.5f; // Bias near 0
  v = (height - planet_radius) / atmo_height;

  return (simd_float2){u, v};
}

void atm_transmittance_lut_decode(simd_float2 uv, float planet_radius,
                                  float atmo_height, float *out_height,
                                  float *out_view_zenith_cos) {
  float u = uv.x;
  float v = uv.y;

  *out_height = planet_radius + v * atmo_height;

  // Inverse of u = (atan(mu*10) / (PI/2)) * 0.5 + 0.5
  // 2u - 1 = atan(mu*10) / (PI/2)
  // (2u - 1) * PI/2 = atan(mu*10)
  // tan(...) = mu * 10
  // mu = tan((2u - 1) * ATM_PI * 0.5f) * 0.1f;

  float angle = (2.0f * u - 1.0f) * (ATM_PI * 0.5f);
  // Clamp angle to avoid infinite tan
  angle = fmaxf(-1.5f, fminf(1.5f, angle));
  *out_view_zenith_cos = tanf(angle) * 0.1f;

  // Clamp cos
  if (*out_view_zenith_cos < -1.0f)
    *out_view_zenith_cos = -1.0f;
  if (*out_view_zenith_cos > 1.0f)
    *out_view_zenith_cos = 1.0f;
}

// =============================================================================
// Sky View LUT Parameterization
// =============================================================================

simd_float2 atm_skyview_lut_encode(simd_float3 view_dir, float view_height,
                                   float planet_radius, float atmo_height) {
  // u: sun angle diff? No, usually:
  // u: View-Sun azimuth angle? NO, encoded relative to sun.
  // SkyViewLUT(u, v)
  // u: Non-linear Azimuth angle between view and sun
  // v: Non-linear Zenith angle of view

  simd_float3 up = {0, 1, 0}; // Assuming local up
  // In reality, need `up` passed in or derived from position.
  // The current signature assumes world-space view_dir and view_height.
  // We assume 'up' is Y-axis or we compute it if we had pos.
  // Since we only have height, we assume the camera is aligned or up is
  // implicit. Wait, usually SkyView is aligned with gravity vector.

  // Let's assume standard up={0,1,0} for the local frame LUT generation.
  float cos_theta = view_dir.y;

  // v coord (Zenith)
  // 0 = zenith, 0.5 = horizon, 1 = nadir ?
  // Non-linear mapping to focus on horizon.
  float v;
  float horizon_angle =
      acosf(atm_horizon_angle_cos(view_height, planet_radius));
  float view_angle = acosf(cos_theta);

  if (view_angle < horizon_angle) {
    // Upper hemisphere
    float coord = view_angle / horizon_angle;
    coord = sqrtf(coord); // Non-linear
    v = 0.5f * coord;
  } else {
    // Lower hemisphere
    float coord = (view_angle - horizon_angle) / (ATM_PI - horizon_angle);
    coord = sqrtf(coord);
    v = 0.5f + 0.5f * coord;
  }

  // u coord (Azimuth? No, usually based on Sun)
  // We don't have Sun direction here?
  // Wait, typical SkyView LUT depends on Sun Zenith but the LUT itself is 2D:
  // (ViewZenith, ViewSunAzimuth). The Sun Zenith is constant for the whole LUT
  // (it's a slice of 4D). So u = relative azimuth.

  float u = 0.5f; // TODO: Azimuth calculation requires project to plane
  // Assuming view_dir.x and .z encode azimuth.
  float azimuth = atan2f(view_dir.z, view_dir.x);
  u = (azimuth + ATM_PI) / (2.0f * ATM_PI);

  return (simd_float2){u, v};
}

simd_float3 atm_skyview_lut_decode(simd_float2 uv, float view_height,
                                   float planet_radius, float atmo_height,
                                   simd_float3 up_dir) {
  // Decoding Logic matching encode...
  // Simplified return for now as we don't strictly need accurate decode for
  // generation (we generate by iterating Angles -> UV checking -> storing).
  // Actually we iterate UV -> Angles -> Compute.

  float v = uv.y;
  float cos_theta;

  float horizon_angle =
      acosf(atm_horizon_angle_cos(view_height, planet_radius));

  if (v < 0.5f) {
    float coord = 2.0f * v;
    coord = coord * coord;
    float view_angle = horizon_angle * coord;
    cos_theta = cosf(view_angle);
  } else {
    float coord = 2.0f * v - 1.0f;
    coord = coord * coord;
    float view_angle = horizon_angle + coord * (ATM_PI - horizon_angle);
    cos_theta = cosf(view_angle);
  }

  float u = uv.x;
  float azimuth = (u * 2.0f * ATM_PI) - ATM_PI;

  float sin_theta = sqrtf(fmaxf(0.0f, 1.0f - cos_theta * cos_theta));

  // Reconstruct vector
  // Assuming Up is Y.
  return (simd_float3){sin_theta * cosf(azimuth), cos_theta,
                       sin_theta * sinf(azimuth)};
}
