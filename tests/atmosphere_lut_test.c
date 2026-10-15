/*
 * atmosphere_lut_test.c
 * Transmittance and sky-view lookup tables: ground occlusion, vacuum at the
 * top of the atmosphere, and meter/kilometer scattering units.
 * Compiles the atmosphere sources directly and does not link Engine.
 */

#include "environment/atmosphere/sky/atmosphere_lut.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void failf(const char *msg, float a, float b, float c) {
  fprintf(stderr, "FAIL: %s (%.6g %.6g %.6g)\n", msg, a, b, c);
  exit(1);
}

static int finite4(const float rgba[4]) {
  return isfinite(rgba[0]) && isfinite(rgba[1]) && isfinite(rgba[2]) &&
         isfinite(rgba[3]);
}

static void expect_zero_rgb(const float rgba[4], const char *msg) {
  if (!finite4(rgba) || rgba[0] != 0.0f || rgba[1] != 0.0f || rgba[2] != 0.0f ||
      rgba[3] != 1.0f) {
    failf(msg, rgba[0], rgba[1], rgba[2]);
  }
}

static void test_lifecycle(void) {
  int tw = 0;
  int th = 0;
  int sw = 0;
  int sh = 0;
  size_t expect;
  const float *data;
  float rgba[4];
  uint32_t version;

  printf("Testing LUT lifecycle...\n");
  atmosphere_lut_shutdown();
  atmosphere_lut_shutdown();
  if (atmosphere_lut_version() != 0 || atmosphere_lut_memory_bytes() != 0)
    fail("shutdown state");

  rgba[0] = rgba[1] = rgba[2] = rgba[3] = 1.0f;
  atmosphere_lut_sample_transmittance(1.0f, 0.0f, rgba);
  atmosphere_lut_sample_transmittance(0.0f, 0.0f, NULL);
  if (rgba[0] != 0.0f || rgba[3] != 0.0f)
    fail("sample before init");

  atmosphere_lut_init();
  atmosphere_lut_transmittance_size(&tw, &th);
  atmosphere_lut_sky_view_size(&sw, &sh);
  if (tw != 256 || th != 64)
    fail("transmittance dimensions");
  if (sw != 192 || sh != 108)
    fail("sky-view dimensions");

  expect = ((size_t)tw * (size_t)th + (size_t)sw * (size_t)sh) * 4u *
           sizeof(float);
  if (atmosphere_lut_version() != 1 || atmosphere_lut_memory_bytes() != expect)
    fail("init accounting");

  data = atmosphere_lut_transmittance_data();
  if (!data || data[0] != 0.0f || data[3] != 0.0f)
    fail("tables start cleared");

  atmosphere_lut_init();
  if (atmosphere_lut_version() != 1 || atmosphere_lut_memory_bytes() != expect)
    fail("re-init leaked or kept stale tables");
  data = atmosphere_lut_transmittance_data();
  if (!data || data[0] != 0.0f || data[3] != 0.0f)
    fail("re-init did not clear");

  version = atmosphere_lut_version();
  atmosphere_lut_generate_transmittance();
  if (atmosphere_lut_version() != version + 1)
    fail("transmittance generation did not publish a new version");
  if (atmosphere_lut_memory_bytes() != expect)
    fail("generation changed the allocation");
}

static void test_transmittance_physics(void) {
  float up[4];
  float horizon[4];
  float down[4];
  float top[4];
  float stored[4];
  const float *data;
  int tw = 0;
  int th = 0;
  int idx;

  printf("Testing transmittance LUT...\n");
  atmosphere_lut_sample_transmittance(1.0f, 0.0f, up);
  atmosphere_lut_sample_transmittance(0.5f, 0.0f, horizon);
  atmosphere_lut_sample_transmittance(0.0f, 0.0f, down);
  atmosphere_lut_sample_transmittance(1.0f, 1.0f, top);

  if (!finite4(up) || up[3] != 1.0f)
    fail("zenith finite");
  /* Vertical Rayleigh optical depth is O(0.1), not ~0 (missing km->m)
   * and not opaque. Blue is removed more than red. */
  if (up[0] < 0.55f || up[0] > 0.98f || up[2] < 0.40f || up[2] > 0.90f ||
      up[2] > up[0] - 0.05f) {
    failf("surface zenith transmittance", up[0], up[1], up[2]);
  }
  if (!finite4(horizon) || horizon[0] >= up[0] || horizon[2] >= up[2] ||
      horizon[2] > 0.55f) {
    failf("horizon should be darker than zenith", horizon[0], horizon[1],
          horizon[2]);
  }

  expect_zero_rgb(down, "surface downward ray must hit the ground");

  if (!finite4(top) || top[0] < 0.99f || top[1] < 0.99f || top[2] < 0.99f ||
      top[3] != 1.0f) {
    failf("top of atmosphere looking out", top[0], top[1], top[2]);
  }

  atmosphere_lut_transmittance_size(&tw, &th);
  data = atmosphere_lut_transmittance_data();
  if (!data)
    fail("missing transmittance table");

  /* u = 1, v = 0 is the last column of the bottom row. */
  idx = ((tw - 1) * 4);
  stored[0] = data[idx + 0];
  stored[1] = data[idx + 1];
  stored[2] = data[idx + 2];
  stored[3] = data[idx + 3];
  if (stored[0] != up[0] || stored[1] != up[1] || stored[2] != up[2] ||
      stored[3] != up[3]) {
    failf("stored zenith texel", stored[0], stored[1], stored[2]);
  }

  idx = ((th - 1) * tw + (tw - 1)) * 4;
  if (data[idx] < 0.99f || data[idx + 3] != 1.0f)
    fail("stored top-of-atmosphere texel");
}

static void test_sky_view(void) {
  float day[4];
  float night[4];
  float ground[4];
  float day_sum;
  float night_sum;
  const float pi = 3.14159265359f;

  printf("Testing sky-view LUT...\n");
  atmosphere_lut_sample_sky_view(0.5f, 0.0f, 0.0f, 0.0f, day);
  atmosphere_lut_sample_sky_view(0.5f, 0.0f, 0.0f, pi, night);
  atmosphere_lut_sample_sky_view(0.5f, 1.0f, 0.0f, 0.0f, ground);

  if (!finite4(day) || !finite4(night) || day[3] != 1.0f || night[3] != 1.0f)
    fail("sky samples not finite");

  day_sum = day[0] + day[1] + day[2];
  night_sum = night[0] + night[1] + night[2];
  if (day_sum < 1.0e-3f || night_sum > day_sum * 0.05f)
    failf("day sky should dominate night", day_sum, night_sum, day[2]);
  if (day[2] <= day[0])
    failf("overhead sun should scatter more blue than red", day[0], day[1],
          day[2]);

  expect_zero_rgb(ground, "looking at the ground is not sky");
}

int main(void) {
  test_lifecycle();
  test_transmittance_physics();
  test_sky_view();
  atmosphere_lut_shutdown();
  if (atmosphere_lut_memory_bytes() != 0 || atmosphere_lut_version() != 0)
    fail("final shutdown");
  printf("atmosphere lut tests passed\n");
  return 0;
}
