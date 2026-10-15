/* Unit tests for atmosphere_rendering.c.
 * Compiles that file directly. Does not link Engine.
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define RAYLEIGH_SCATTERING_COEFFICIENT 5.8e-6f
#define MIE_SCATTERING_COEFFICIENT 2.0e-5f
#define OZONE_ABSORPTION_COEFFICIENT 1.1e-6f
#define EARTH_RADIUS 6371000.0f
#define SOLAR_ELEVATION_MIN (-18.0f)

void sky_set_sun_direction(float x, float y, float z);
void sky_set_time_of_day(float hours);
float sky_get_time_of_day(void);
void sky_set_scattering_intensity(float intensity);
float sky_get_scattering_intensity(void);
void sky_set_cloud_density(float density);
float sky_get_cloud_density(void);
void sky_set_clouds_enabled(bool enabled);
bool sky_get_clouds_enabled(void);

void atmosphere_init(void);
void atmosphere_update(float delta_time);
void atmosphere_render(void);
void atmosphere_set_moon_direction(float x, float y, float z);
void atmosphere_get_moon_direction(float *x, float *y, float *z);
void atmosphere_set_halo_parameters(float intensity, float radius,
                                    float falloff);
void atmosphere_enable_cloud_shadows(bool enabled);
void atmosphere_enable_space_view(bool enabled, float curvature_radius);
void atmosphere_enable_neural_sky(bool enabled);
void atmosphere_set_temporal_upsampling(float weight);
float atmosphere_get_dawn_progress(void);
float atmosphere_get_dusk_progress(void);
bool atmosphere_is_night(void);
void atmosphere_get_rayleigh_scattering(float *r, float *g, float *b);
void atmosphere_get_mie_scattering(float *r, float *g, float *b);
void atmosphere_get_ozone_absorption(float *r, float *g, float *b);
void atmosphere_apply_space_view_curvature(void);
void atmosphere_apply_temporal_upsampling(void);
void atmosphere_apply_ai_sky_generation(void);
void atmosphere_set_view_height(float height);
void atmosphere_set_curvature_intensity(float intensity);
void atmosphere_set_temporal_upsampling_enabled(bool enabled);
void atmosphere_set_temporal_weight(float weight);
void atmosphere_set_ai_sky_generation_enabled(bool enabled);
void atmosphere_set_preetham_turbidity(float turbidity);
void atmosphere_set_ai_enhancement_factor(float factor);

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void expect_true(bool cond, const char *msg) {
  if (!cond) {
    fail(msg);
  }
}

static int near(float actual, float expected, float eps) {
  float diff = actual - expected;
  if (diff < 0.0f) {
    diff = -diff;
  }
  return diff <= eps;
}

static void expect_near(float actual, float expected, float eps,
                        const char *msg) {
  if (!near(actual, expected, eps)) {
    fprintf(stderr, "FAIL: %s: got %g expected %g\n", msg, actual, expected);
    exit(1);
  }
}

/* Elevation of the sun the time-of-day update derives from hours. */
static float sun_elevation_deg(float hours) {
  float solar_angle = (hours / 24.0f) * 2.0f * (float)M_PI - (float)M_PI_2;
  return asinf(sinf(solar_angle)) * 180.0f / (float)M_PI;
}

/* Civil twilight (-18 deg) through the horizon ramps 0.1 -> 1. */
static float expected_scattering(float elevation_deg) {
  if (elevation_deg < SOLAR_ELEVATION_MIN) {
    return 0.1f;
  }
  if (elevation_deg < 0.0f) {
    float twilight_factor =
        (elevation_deg - SOLAR_ELEVATION_MIN) / (-SOLAR_ELEVATION_MIN);
    return 0.1f + 0.9f * twilight_factor;
  }
  return 1.0f;
}

static void settle(float hours) {
  sky_set_time_of_day(hours);
  atmosphere_update(0.0f);
}

int main(void) {
  float r;
  float g;
  float b;
  float x;
  float y;
  float z;
  float twilight_hours = 5.4f;
  float enhancement;

  sky_set_time_of_day(12.0f);
  expect_near(sky_get_time_of_day(), 12.0f, 1e-4f, "noon set");
  sky_set_scattering_intensity(0.25f);
  expect_near(sky_get_scattering_intensity(), 0.25f, 1e-5f, "scatter set");
  sky_set_cloud_density(0.7f);
  expect_near(sky_get_cloud_density(), 0.7f, 1e-5f, "density set");
  sky_set_clouds_enabled(false);
  expect_true(!sky_get_clouds_enabled(), "clouds off");
  sky_set_clouds_enabled(true);
  expect_true(sky_get_clouds_enabled(), "clouds on");
  sky_set_sun_direction(0.0f, 1.0f, 0.0f);

  atmosphere_set_moon_direction(0.2f, -0.4f, 0.8f);
  atmosphere_get_moon_direction(&x, &y, &z);
  expect_near(x, 0.2f, 1e-5f, "moon x");
  expect_near(y, -0.4f, 1e-5f, "moon y");
  expect_near(z, 0.8f, 1e-5f, "moon z");
  atmosphere_get_moon_direction(NULL, NULL, NULL);

  atmosphere_init();
  atmosphere_get_rayleigh_scattering(NULL, NULL, NULL);
  atmosphere_get_mie_scattering(NULL, NULL, NULL);
  atmosphere_get_ozone_absorption(NULL, NULL, NULL);

  atmosphere_get_rayleigh_scattering(&r, &g, &b);
  expect_near(r, RAYLEIGH_SCATTERING_COEFFICIENT, 1e-12f, "rayleigh r");
  expect_near(g, RAYLEIGH_SCATTERING_COEFFICIENT, 1e-12f, "rayleigh g");
  expect_near(b, RAYLEIGH_SCATTERING_COEFFICIENT, 1e-12f, "rayleigh b");

  atmosphere_get_mie_scattering(&r, &g, &b);
  expect_near(r, MIE_SCATTERING_COEFFICIENT, 1e-12f, "mie r");
  expect_near(g, MIE_SCATTERING_COEFFICIENT, 1e-12f, "mie g");
  expect_near(b, MIE_SCATTERING_COEFFICIENT, 1e-12f, "mie b");

  atmosphere_get_ozone_absorption(&r, &g, &b);
  expect_near(r, OZONE_ABSORPTION_COEFFICIENT * 0.6f, 1e-12f, "ozone r");
  expect_near(g, OZONE_ABSORPTION_COEFFICIENT, 1e-12f, "ozone g");
  expect_near(b, OZONE_ABSORPTION_COEFFICIENT * 0.8f, 1e-12f, "ozone b");
  expect_true(g > r && g > b, "ozone Chappuis peak is green");

  atmosphere_enable_space_view(true, 6370000.0f);
  atmosphere_get_rayleigh_scattering(&r, &g, &b);
  expect_near(r, RAYLEIGH_SCATTERING_COEFFICIENT, 1e-12f,
              "below 50km keeps sea-level rayleigh");
  atmosphere_enable_space_view(false, EARTH_RADIUS);

  settle(12.0f);
  expect_true(!atmosphere_is_night(), "noon is day");
  expect_near(atmosphere_get_dawn_progress(), 0.0f, 1e-4f, "noon dawn");
  expect_near(atmosphere_get_dusk_progress(), 0.0f, 1e-4f, "noon dusk");
  expect_near(sky_get_scattering_intensity(),
              expected_scattering(sun_elevation_deg(12.0f)), 1e-4f,
              "noon scattering");
  atmosphere_get_moon_direction(&x, &y, &z);
  expect_near(x, 0.0f, 1e-4f, "noon moon x");
  expect_near(y, -1.0f, 1e-4f, "noon moon opposite sun");
  expect_near(z, 0.0f, 1e-4f, "noon moon z");

  settle(0.0f);
  expect_true(atmosphere_is_night(), "midnight is night");
  expect_near(atmosphere_get_dawn_progress(), 0.0f, 1e-4f, "midnight dawn");
  expect_near(atmosphere_get_dusk_progress(), 0.0f, 1e-4f, "midnight dusk");
  expect_near(sky_get_scattering_intensity(), 0.1f, 1e-4f,
              "deep night scattering");

  settle(6.0f);
  expect_true(!atmosphere_is_night(), "06:00 is not night");
  expect_near(atmosphere_get_dawn_progress(), 0.5f, 1e-4f, "dawn midpoint");
  expect_near(atmosphere_get_dusk_progress(), 0.0f, 1e-4f, "dawn has no dusk");
  expect_near(sky_get_scattering_intensity(), 1.0f, 1e-4f,
              "horizon scattering");

  settle(18.0f);
  expect_true(!atmosphere_is_night(), "18:00 is not night");
  expect_near(atmosphere_get_dusk_progress(), 0.5f, 1e-4f, "dusk midpoint");
  expect_near(atmosphere_get_dawn_progress(), 0.0f, 1e-4f, "dusk has no dawn");

  settle(5.0f);
  expect_true(atmosphere_is_night(), "05:00 is night");
  expect_near(atmosphere_get_dawn_progress(), 0.0f, 1e-4f, "dawn start");

  settle(19.0f);
  expect_true(atmosphere_is_night(), "19:00 is night");
  expect_near(atmosphere_get_dusk_progress(), 1.0f, 1e-3f, "dusk end");

  settle(twilight_hours);
  expect_true(atmosphere_is_night(), "05:24 is still night");
  expect_near(atmosphere_get_dawn_progress(), 0.2f, 1e-3f, "dawn progress");
  expect_near(atmosphere_get_dusk_progress(), 0.0f, 1e-4f, "twilight dusk");
  expect_near(sky_get_scattering_intensity(),
              expected_scattering(sun_elevation_deg(twilight_hours)), 1e-3f,
              "civil twilight scattering");

  sky_set_time_of_day(23.5f);
  atmosphere_update(10.0f);
  expect_near(sky_get_time_of_day(), 0.5f, 1e-3f, "24h wrap");
  expect_true(atmosphere_is_night(), "00:30 is night");

  settle(6.0f);
  settle(12.0f);
  expect_near(atmosphere_get_dawn_progress(), 0.0f, 1e-4f, "dawn cleared");
  expect_near(atmosphere_get_dusk_progress(), 0.0f, 1e-4f, "dusk still clear");

  sky_set_scattering_intensity(1.0f);
  sky_set_cloud_density(0.5f);
  atmosphere_set_ai_enhancement_factor(-5.0f);
  atmosphere_apply_ai_sky_generation();
  expect_near(sky_get_scattering_intensity(), 1.0f, 1e-4f,
              "negative enhancement clamps to zero");
  expect_near(sky_get_cloud_density(), 0.4f, 1e-4f, "weather cloud blend");

  sky_set_scattering_intensity(1.0f);
  sky_set_cloud_density(0.5f);
  atmosphere_set_ai_enhancement_factor(1.0f);
  atmosphere_apply_ai_sky_generation();
  expect_near(sky_get_scattering_intensity(), 2.0f, 1e-4f,
              "enhancement doubles scattering");
  expect_near(sky_get_cloud_density(), 0.4f, 1e-4f, "weather blend repeats");

  sky_set_scattering_intensity(2.0f);
  atmosphere_set_curvature_intensity(1.0f);
  atmosphere_set_view_height(1000.0f);
  atmosphere_apply_space_view_curvature();
  expect_near(sky_get_scattering_intensity(), 2.0f, 1e-4f,
              "low view skips curvature");

  sky_set_scattering_intensity(2.0f);
  atmosphere_set_view_height(6471000.0f);
  atmosphere_apply_space_view_curvature();
  expect_near(sky_get_scattering_intensity(), 3.0f, 1e-3f,
              "high view boosts scattering");

  atmosphere_enable_space_view(true, 6000000.0f);
  atmosphere_get_rayleigh_scattering(&r, &g, &b);
  expect_near(r, RAYLEIGH_SCATTERING_COEFFICIENT * 0.8f, 1e-12f, "space r");
  expect_near(g, RAYLEIGH_SCATTERING_COEFFICIENT * 0.9f, 1e-12f, "space g");
  expect_near(b, RAYLEIGH_SCATTERING_COEFFICIENT * 1.2f, 1e-12f, "space b");
  expect_true(b > g && g > r, "high altitude shifts toward blue");

  atmosphere_enable_neural_sky(true);
  sky_set_time_of_day(12.0f);
  atmosphere_update(0.0f);
  enhancement = sinf(12.0f * 0.1f);
  atmosphere_get_rayleigh_scattering(&r, &g, &b);
  expect_near(r,
              RAYLEIGH_SCATTERING_COEFFICIENT * 0.8f *
                  (1.0f + enhancement * 0.1f),
              1e-9f, "neural rayleigh r");
  expect_near(g,
              RAYLEIGH_SCATTERING_COEFFICIENT * 0.9f *
                  (1.0f + enhancement * 0.05f),
              1e-9f, "neural rayleigh g");
  expect_near(b,
              RAYLEIGH_SCATTERING_COEFFICIENT * 1.2f *
                  (1.0f + enhancement * 0.15f),
              1e-9f, "neural rayleigh b");
  atmosphere_get_mie_scattering(&r, &g, &b);
  expect_near(r, MIE_SCATTERING_COEFFICIENT * (1.0f + enhancement * 0.08f),
              1e-9f, "neural mie");
  expect_near(sky_get_scattering_intensity(), 1.0f, 1e-4f,
              "noon still full scattering after neural update");

  atmosphere_set_halo_parameters(0.5f, 22.0f, 2.0f);
  atmosphere_enable_cloud_shadows(true);
  atmosphere_set_temporal_upsampling_enabled(true);
  atmosphere_set_temporal_weight(-1.0f);
  atmosphere_set_temporal_weight(2.0f);
  atmosphere_set_temporal_weight(0.25f);
  atmosphere_set_temporal_upsampling(0.2f);
  atmosphere_apply_temporal_upsampling();
  atmosphere_set_ai_sky_generation_enabled(true);
  atmosphere_set_preetham_turbidity(0.1f);
  atmosphere_set_preetham_turbidity(3.0f);
  atmosphere_render();

  printf("atmosphere rendering ok\n");
  return 0;
}
