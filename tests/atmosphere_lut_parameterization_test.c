/*
 * atmosphere_lut_parameterization_test.c
 * Horizon angle, ray-to-boundary distance, and LUT encode/decode.
 * Compiles atmosphere_lut_parameterization.c directly and does not link Engine.
 */

#include "atmosphere_lut_parameterization.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static int near_f(float a, float b) {
  float d = a - b;
  if (d < 0.0f) {
    d = -d;
  }
  return d <= 1.0e-4f;
}

static void expect_f(float got, float want, const char *msg) {
  if (!near_f(got, want)) {
    fprintf(stderr, "FAIL: %s got %g expected %g\n", msg, got, want);
    exit(1);
  }
}

static void expect_vec3(simd_float3 got, float x, float y, float z,
                        const char *msg) {
  if (!near_f(got.x, x) || !near_f(got.y, y) || !near_f(got.z, z)) {
    fprintf(stderr, "FAIL: %s got (%g, %g, %g) expected (%g, %g, %g)\n", msg,
            got.x, got.y, got.z, x, y, z);
    exit(1);
  }
}

static void test_horizon(void) {
  const float planet = 10.0f;

  printf("Testing horizon angle cosine...\n");
  expect_f(atm_horizon_angle_cos(9.0f, planet), -1.0f, "below surface");
  expect_f(atm_horizon_angle_cos(planet, planet), 0.0f, "on surface");
  /* r = 2R -> -sqrt(r^2 - R^2) / r = -sqrt(3) / 2 */
  expect_f(atm_horizon_angle_cos(20.0f, planet), -0.8660254f, "double radius");
}

static void test_boundary(void) {
  const float planet = 10.0f;
  const float atmo = 5.0f;
  const simd_float3 inside = {0.0f, 12.0f, 0.0f};
  const simd_float3 up = {0.0f, 1.0f, 0.0f};
  const simd_float3 down = {0.0f, -1.0f, 0.0f};
  const simd_float3 east = {1.0f, 0.0f, 0.0f};
  const simd_float3 outside = {0.0f, 20.0f, 0.0f};
  bool ground = true;
  float dist;

  printf("Testing distance to atmosphere boundary...\n");
  dist = atm_distance_to_atmosphere_boundary(inside, up, planet, atmo, &ground);
  expect_f(dist, 3.0f, "up to top");
  if (ground) {
    fail("up should miss the ground");
  }

  ground = false;
  dist = atm_distance_to_atmosphere_boundary(inside, down, planet, atmo, &ground);
  expect_f(dist, 2.0f, "down to ground");
  if (!ground) {
    fail("down should hit the ground");
  }

  ground = true;
  dist = atm_distance_to_atmosphere_boundary(inside, east, planet, atmo, &ground);
  expect_f(dist, 9.0f, "horizontal to top");
  if (ground) {
    fail("horizontal should miss the ground");
  }

  ground = true;
  dist = atm_distance_to_atmosphere_boundary(outside, up, planet, atmo, &ground);
  if (!(dist < 0.0f) || ground) {
    fail("outside looking away");
  }

  /* NULL flag must not be written. */
  dist = atm_distance_to_atmosphere_boundary(inside, up, planet, atmo, NULL);
  expect_f(dist, 3.0f, "null ground flag");
}

static void test_transmittance(void) {
  const float planet = 10.0f;
  const float atmo = 5.0f;
  const float mus[] = {-1.0f, -0.25f, 0.0f, 0.4f, 1.0f};
  simd_float2 uv;
  simd_float2 mid;
  float height;
  float mu;
  float prev_u;
  int i;

  printf("Testing transmittance LUT encode/decode...\n");
  uv = atm_transmittance_lut_encode(planet, 0.0f, planet, atmo);
  expect_f(uv.y, 0.0f, "surface v");
  expect_f(uv.x, 0.5f, "mu 0 u");

  uv = atm_transmittance_lut_encode(planet + atmo, 0.0f, planet, atmo);
  expect_f(uv.y, 1.0f, "top v");

  prev_u = -1.0f;
  for (i = 0; i < 5; i++) {
    uv = atm_transmittance_lut_encode(12.5f, mus[i], planet, atmo);
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
      fail("transmittance uv range");
    }
    if (!(uv.x > prev_u)) {
      fail("transmittance u not increasing with mu");
    }
    prev_u = uv.x;
    expect_f(uv.y, 0.5f, "mid height v");

    atm_transmittance_lut_decode(uv, planet, atmo, &height, &mu);
    expect_f(height, 12.5f, "height roundtrip");
    expect_f(mu, mus[i], "mu roundtrip");
  }

  mid = (simd_float2){0.5f, 0.0f};
  atm_transmittance_lut_decode(mid, planet, atmo, &height, &mu);
  expect_f(height, planet, "decode surface");
  expect_f(mu, 0.0f, "decode horizon mu");

  mid = (simd_float2){0.0f, 1.0f};
  atm_transmittance_lut_decode(mid, planet, atmo, &height, &mu);
  expect_f(height, planet + atmo, "decode top");
  expect_f(mu, -1.0f, "decode nadir mu");

  mid = (simd_float2){1.0f, 0.25f};
  atm_transmittance_lut_decode(mid, planet, atmo, &height, &mu);
  expect_f(mu, 1.0f, "decode zenith mu");
}

static void test_skyview(void) {
  const float planet = 10.0f;
  const float atmo = 5.0f;
  const float height = 12.0f;
  const simd_float3 up = {0.0f, 1.0f, 0.0f};
  const simd_float3 down = {0.0f, -1.0f, 0.0f};
  const simd_float3 east = {1.0f, 0.0f, 0.0f};
  const simd_float3 west = {-1.0f, 0.0f, 0.0f};
  const simd_float3 north = {0.0f, 0.0f, 1.0f};
  const simd_float3 south = {0.0f, 0.0f, -1.0f};
  const simd_float3 tilt = {0.6f, 0.8f, 0.0f};
  const simd_float3 low = {0.3f, -0.4f, 0.8660254f};
  const simd_float3 below = {0.6f, -0.8f, 0.0f};
  simd_float2 uv;
  simd_float3 decoded;

  printf("Testing sky-view LUT encode/decode...\n");
  uv = atm_skyview_lut_encode(up, planet, planet, atmo);
  expect_f(uv.x, 0.5f, "up u");
  expect_f(uv.y, 0.0f, "up v");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, 0.0f, 1.0f, 0.0f, "decode up");

  uv = atm_skyview_lut_encode(down, planet, planet, atmo);
  expect_f(uv.x, 0.5f, "down u");
  expect_f(uv.y, 1.0f, "down v");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, 0.0f, -1.0f, 0.0f, "decode down");

  uv = atm_skyview_lut_encode(east, planet, planet, atmo);
  expect_f(uv.x, 0.5f, "east u");
  expect_f(uv.y, 0.5f, "east v");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, 1.0f, 0.0f, 0.0f, "decode east");

  uv = atm_skyview_lut_encode(west, planet, planet, atmo);
  expect_f(uv.x, 1.0f, "west u");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, -1.0f, 0.0f, 0.0f, "decode west");

  uv = atm_skyview_lut_encode(north, planet, planet, atmo);
  expect_f(uv.x, 0.75f, "north u");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, 0.0f, 0.0f, 1.0f, "decode north");

  uv = atm_skyview_lut_encode(south, planet, planet, atmo);
  expect_f(uv.x, 0.25f, "south u");
  decoded = atm_skyview_lut_decode(uv, planet, planet, atmo, up);
  expect_vec3(decoded, 0.0f, 0.0f, -1.0f, "decode south");

  uv = atm_skyview_lut_encode(tilt, height, planet, atmo);
  if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
    fail("tilt uv range");
  }
  decoded = atm_skyview_lut_decode(uv, height, planet, atmo, up);
  expect_vec3(decoded, tilt.x, tilt.y, tilt.z, "tilt roundtrip");

  uv = atm_skyview_lut_encode(low, height, planet, atmo);
  if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
    fail("low uv range");
  }
  decoded = atm_skyview_lut_decode(uv, height, planet, atmo, up);
  expect_vec3(decoded, low.x, low.y, low.z, "low roundtrip");

  uv = atm_skyview_lut_encode(below, height, planet, atmo);
  if (uv.y <= 0.5f) {
    fail("below-horizon v");
  }
  decoded = atm_skyview_lut_decode(uv, height, planet, atmo, up);
  expect_vec3(decoded, below.x, below.y, below.z, "below roundtrip");
}

int main(void) {
  test_horizon();
  test_boundary();
  test_transmittance();
  test_skyview();
  printf("All atmosphere LUT parameterization tests passed\n");
  return 0;
}
