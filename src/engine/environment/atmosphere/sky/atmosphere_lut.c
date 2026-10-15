#include "atmosphere_lut.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "atmosphere_common.h"
#include "atmosphere_lut_parameterization.h"
#include "atmosphere_scattering.h"

#define TRANSMITTANCE_FLOATS (TRANSMITTANCE_WIDTH * TRANSMITTANCE_HEIGHT * 4)
#define SKY_VIEW_FLOATS (SKYVIEW_WIDTH * SKYVIEW_HEIGHT * 4)

typedef struct {
  float *transmittance_lut;
  float *sky_view_lut;
  uint32_t version;
  ScatteringCoefficients coeffs;
  AtmosphereLayerParams layers;
  bool ready;
} AtmosphereLUTSystem;

static AtmosphereLUTSystem g_lut_system;

static float clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static void clear_rgba(float out_rgba[4]) {
  out_rgba[0] = 0.0f;
  out_rgba[1] = 0.0f;
  out_rgba[2] = 0.0f;
  out_rgba[3] = 0.0f;
}

static size_t table_bytes(void) {
  return (size_t)(TRANSMITTANCE_FLOATS + SKY_VIEW_FLOATS) * sizeof(float);
}

void atmosphere_lut_init(void) {
  free(g_lut_system.transmittance_lut);
  free(g_lut_system.sky_view_lut);
  g_lut_system.transmittance_lut = NULL;
  g_lut_system.sky_view_lut = NULL;
  g_lut_system.ready = false;
  g_lut_system.version = 0;

  g_lut_system.transmittance_lut =
      (float *)calloc((size_t)TRANSMITTANCE_FLOATS, sizeof(float));
  g_lut_system.sky_view_lut =
      (float *)calloc((size_t)SKY_VIEW_FLOATS, sizeof(float));
  if (!g_lut_system.transmittance_lut || !g_lut_system.sky_view_lut) {
    free(g_lut_system.transmittance_lut);
    free(g_lut_system.sky_view_lut);
    g_lut_system.transmittance_lut = NULL;
    g_lut_system.sky_view_lut = NULL;
    return;
  }

  g_lut_system.coeffs = atm_get_earth_coefficients();
  atm_get_earth_density_profiles(&g_lut_system.layers);
  g_lut_system.version = 1;
  g_lut_system.ready = true;
}

void atmosphere_lut_shutdown(void) {
  free(g_lut_system.transmittance_lut);
  free(g_lut_system.sky_view_lut);
  memset(&g_lut_system, 0, sizeof(g_lut_system));
}

uint32_t atmosphere_lut_version(void) { return g_lut_system.version; }

size_t atmosphere_lut_memory_bytes(void) {
  if (!g_lut_system.ready)
    return 0;
  return table_bytes();
}

void atmosphere_lut_transmittance_size(int *out_width, int *out_height) {
  if (out_width)
    *out_width = TRANSMITTANCE_WIDTH;
  if (out_height)
    *out_height = TRANSMITTANCE_HEIGHT;
}

void atmosphere_lut_sky_view_size(int *out_width, int *out_height) {
  if (out_width)
    *out_width = SKYVIEW_WIDTH;
  if (out_height)
    *out_height = SKYVIEW_HEIGHT;
}

const float *atmosphere_lut_transmittance_data(void) {
  return g_lut_system.transmittance_lut;
}

const float *atmosphere_lut_sky_view_data(void) {
  return g_lut_system.sky_view_lut;
}

static void sample_transmittance(float u, float v, float out_rgba[4]) {
  float height;
  float mu;
  float sin_mu;
  float dist;
  bool ground_hit;
  simd_float3 origin;
  simd_float3 dir;
  simd_float3 tau;
  simd_float3 transmittance;

  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);
  out_rgba[0] = 0.0f;
  out_rgba[1] = 0.0f;
  out_rgba[2] = 0.0f;
  out_rgba[3] = 1.0f;

  atm_transmittance_lut_decode((simd_float2){u, v}, ATM_EARTH_RADIUS,
                               ATM_ATMOSPHERE_HEIGHT, &height, &mu);
  sin_mu = sqrtf(fmaxf(0.0f, 1.0f - mu * mu));
  origin = (simd_float3){0.0f, height, 0.0f};
  dir = (simd_float3){sin_mu, mu, 0.0f};

  dist = atm_distance_to_atmosphere_boundary(
      origin, dir, ATM_EARTH_RADIUS, ATM_ATMOSPHERE_HEIGHT, &ground_hit);

  /* Blocked by the planet: no path to space. */
  if (ground_hit)
    return;

  /* Already at or outside the top, looking out: nothing left to absorb. */
  if (dist <= 0.0f) {
    out_rgba[0] = 1.0f;
    out_rgba[1] = 1.0f;
    out_rgba[2] = 1.0f;
    return;
  }

  tau = atm_optical_depth(origin, dir, dist, ATM_EARTH_RADIUS,
                          ATM_ATMOSPHERE_HEIGHT, g_lut_system.coeffs,
                          g_lut_system.layers, 40);
  transmittance = atm_transmittance_from_optical_depth(tau);
  out_rgba[0] = transmittance.x;
  out_rgba[1] = transmittance.y;
  out_rgba[2] = transmittance.z;
}

static void sample_sky_view(float u, float v, float sun_phi, float sun_theta,
                            float out_rgba[4]) {
  float view_height;
  float dist;
  bool ground_hit;
  simd_float3 sun_dir;
  simd_float3 view_pos;
  simd_float3 up;
  simd_float3 view_dir;
  simd_float3 color;

  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);
  out_rgba[0] = 0.0f;
  out_rgba[1] = 0.0f;
  out_rgba[2] = 0.0f;
  out_rgba[3] = 1.0f;

  sun_dir = (simd_float3){sinf(sun_theta) * cosf(sun_phi), cosf(sun_theta),
                          sinf(sun_theta) * sinf(sun_phi)};
  view_height = ATM_EARTH_RADIUS + 0.002f;
  view_pos = (simd_float3){0.0f, view_height, 0.0f};
  up = (simd_float3){0.0f, 1.0f, 0.0f};
  view_dir = atm_skyview_lut_decode((simd_float2){u, v}, view_height,
                                    ATM_EARTH_RADIUS, ATM_ATMOSPHERE_HEIGHT, up);

  dist = atm_distance_to_atmosphere_boundary(
      view_pos, view_dir, ATM_EARTH_RADIUS, ATM_ATMOSPHERE_HEIGHT, &ground_hit);
  if (ground_hit || dist <= 0.0f)
    return;

  color = atm_combined_scattering(
      view_pos, view_dir, sun_dir, dist, ATM_EARTH_RADIUS, ATM_ATMOSPHERE_HEIGHT,
      g_lut_system.coeffs, g_lut_system.layers, 32);
  out_rgba[0] = color.x;
  out_rgba[1] = color.y;
  out_rgba[2] = color.z;
}

void atmosphere_lut_sample_transmittance(float u, float v, float out_rgba[4]) {
  if (!out_rgba)
    return;
  if (!g_lut_system.ready) {
    clear_rgba(out_rgba);
    return;
  }
  sample_transmittance(u, v, out_rgba);
}

void atmosphere_lut_sample_sky_view(float u, float v, float sun_phi,
                                   float sun_theta, float out_rgba[4]) {
  if (!out_rgba)
    return;
  if (!g_lut_system.ready) {
    clear_rgba(out_rgba);
    return;
  }
  sample_sky_view(u, v, sun_phi, sun_theta, out_rgba);
}

void atmosphere_lut_generate_transmittance(void) {
  int x;
  int y;

  if (!g_lut_system.ready || !g_lut_system.transmittance_lut)
    return;

  for (y = 0; y < TRANSMITTANCE_HEIGHT; ++y) {
    for (x = 0; x < TRANSMITTANCE_WIDTH; ++x) {
      float rgba[4];
      float u = (float)x / (float)(TRANSMITTANCE_WIDTH - 1);
      float v = (float)y / (float)(TRANSMITTANCE_HEIGHT - 1);
      int idx = (y * TRANSMITTANCE_WIDTH + x) * 4;

      sample_transmittance(u, v, rgba);
      g_lut_system.transmittance_lut[idx + 0] = rgba[0];
      g_lut_system.transmittance_lut[idx + 1] = rgba[1];
      g_lut_system.transmittance_lut[idx + 2] = rgba[2];
      g_lut_system.transmittance_lut[idx + 3] = rgba[3];
    }
  }
  g_lut_system.version++;
}

void atmosphere_lut_generate_sky_view(float sun_phi, float sun_theta) {
  int x;
  int y;

  if (!g_lut_system.ready || !g_lut_system.sky_view_lut)
    return;

  for (y = 0; y < SKYVIEW_HEIGHT; ++y) {
    for (x = 0; x < SKYVIEW_WIDTH; ++x) {
      float rgba[4];
      float u = (float)x / (float)(SKYVIEW_WIDTH - 1);
      float v = (float)y / (float)(SKYVIEW_HEIGHT - 1);
      int idx = (y * SKYVIEW_WIDTH + x) * 4;

      sample_sky_view(u, v, sun_phi, sun_theta, rgba);
      g_lut_system.sky_view_lut[idx + 0] = rgba[0];
      g_lut_system.sky_view_lut[idx + 1] = rgba[1];
      g_lut_system.sky_view_lut[idx + 2] = rgba[2];
      g_lut_system.sky_view_lut[idx + 3] = rgba[3];
    }
  }
  g_lut_system.version++;
}
