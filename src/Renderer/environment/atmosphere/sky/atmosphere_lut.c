#include "../../../include/common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "atmosphere_common.h"
#include "atmosphere_lut_parameterization.h"
#include "atmosphere_scattering.h"

// Dimensions for atmospheric LUTs
#define TRANSMITTANCE_W 256
#define TRANSMITTANCE_H 64
#define SKY_VIEW_W 192
#define SKY_VIEW_H 108

typedef struct {
  float *transmittance_lut;
  float *sky_view_lut;
  uint32_t version;

  // Cache current params
  ScatteringCoefficients coeffs;
  AtmosphereLayerParams layers;
} AtmosphereLUTSystem;

static AtmosphereLUTSystem g_lut_system = {0};

void atmosphere_lut_init(void) {
  g_lut_system.transmittance_lut =
      (float *)malloc(TRANSMITTANCE_W * TRANSMITTANCE_H * 4 * sizeof(float));
  g_lut_system.sky_view_lut =
      (float *)malloc(SKY_VIEW_W * SKY_VIEW_H * 4 * sizeof(float));
  g_lut_system.version = 1;

  // Init default earth params
  g_lut_system.coeffs = atm_get_earth_coefficients();
  atm_get_earth_density_profiles(&g_lut_system.layers);
}

void atmosphere_lut_generate_transmittance(void) {
  if (!g_lut_system.transmittance_lut)
    return;

  float planet_radius = ATM_EARTH_RADIUS;
  float atmo_height = ATM_ATMOSPHERE_HEIGHT;

  // Parallelizable loop
  for (int y = 0; y < TRANSMITTANCE_H; ++y) {
    for (int x = 0; x < TRANSMITTANCE_W; ++x) {
      simd_float2 uv = {(float)x / (TRANSMITTANCE_W - 1),
                        (float)y / (TRANSMITTANCE_H - 1)};

      float height;
      float view_zenith_cos;
      atm_transmittance_lut_decode(uv, planet_radius, atmo_height, &height,
                                   &view_zenith_cos);

      simd_float3 origin = {0, height, 0};
      float view_zenith_sin =
          sqrtf(fmaxf(0.0f, 1.0f - view_zenith_cos * view_zenith_cos));
      simd_float3 dir = {view_zenith_sin, view_zenith_cos, 0.0f};

      // Raymarch to top
      bool ground_hit;
      float dist_to_top = atm_distance_to_atmosphere_boundary(
          origin, dir, planet_radius, atmo_height, &ground_hit);

      simd_float3 transmittance = {0, 0, 0};

      if (!ground_hit && dist_to_top > 0.0f) {
        simd_float3 tau = atm_optical_depth(
            origin, dir, dist_to_top, planet_radius, atmo_height,
            g_lut_system.coeffs, g_lut_system.layers, 40);
        transmittance = atm_transmittance_from_optical_depth(tau);
      }

      // Store R,G,B,1
      int idx = (y * TRANSMITTANCE_W + x) * 4;
      g_lut_system.transmittance_lut[idx + 0] = transmittance.x;
      g_lut_system.transmittance_lut[idx + 1] = transmittance.y;
      g_lut_system.transmittance_lut[idx + 2] = transmittance.z;
      g_lut_system.transmittance_lut[idx + 3] = 1.0f;
    }
  }
}

void atmosphere_lut_generate_sky_view(float sun_phi, float sun_theta) {
  if (!g_lut_system.sky_view_lut)
    return;

  // Sun direction from angles
  // theta = zenith from up? Usually theta=zenith.
  simd_float3 sun_dir = {sinf(sun_theta) * cosf(sun_phi), cosf(sun_theta),
                         sinf(sun_theta) * sinf(sun_phi)};

  float planet_radius = ATM_EARTH_RADIUS;
  float atmo_height = ATM_ATMOSPHERE_HEIGHT;
  float view_height = planet_radius + 0.002f; // Offset slightly (2m)

  simd_float3 view_pos = {0, view_height, 0};
  simd_float3 up = {0, 1, 0};

  for (int y = 0; y < SKY_VIEW_H; ++y) {
    for (int x = 0; x < SKY_VIEW_W; ++x) {
      simd_float2 uv = {(float)x / (SKY_VIEW_W - 1),
                        (float)y / (SKY_VIEW_H - 1)};

      simd_float3 view_dir = atm_skyview_lut_decode(
          uv, view_height, planet_radius, atmo_height, up);

      bool ground_hit;
      float dist_to_top = atm_distance_to_atmosphere_boundary(
          view_pos, view_dir, planet_radius, atmo_height, &ground_hit);

      simd_float3 color = {0, 0, 0};

      if (!ground_hit && dist_to_top > 0.0f) {
        // Single scattering integration
        color = atm_combined_scattering(
            view_pos, view_dir, sun_dir, dist_to_top, planet_radius,
            atmo_height, g_lut_system.coeffs, g_lut_system.layers, 32);
      }

      int idx = (y * SKY_VIEW_W + x) * 4;
      g_lut_system.sky_view_lut[idx + 0] = color.x;
      g_lut_system.sky_view_lut[idx + 1] = color.y;
      g_lut_system.sky_view_lut[idx + 2] = color.z;
      g_lut_system.sky_view_lut[idx + 3] = 1.0f;
    }
  }

  g_lut_system.version++;
}

void atmosphere_lut_shutdown(void) {
  if (g_lut_system.transmittance_lut)
    free(g_lut_system.transmittance_lut);
  if (g_lut_system.sky_view_lut)
    free(g_lut_system.sky_view_lut);
  g_lut_system.transmittance_lut = NULL;
  g_lut_system.sky_view_lut = NULL;
}
