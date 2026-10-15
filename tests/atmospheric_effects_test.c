/* Issue #334: unit test for atmospheric_effects.c.
 * Compiles that file directly. Does not link Engine.
 * volumetric_fog and the logger are stubbed; weather, time, fog, clouds,
 * precipitation, and lightning run for real.
 */
#include "atmospheric_effects.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail_fog_create = 0;
static int g_failures = 0;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

volumetric_fog_t *volumetric_fog_create(metal_device_t *dev, uint32_t w,
                                        uint32_t h, uint32_t d) {
  (void)dev;
  (void)w;
  (void)h;
  (void)d;
  if (g_fail_fog_create) {
    return NULL;
  }
  return calloc(1, sizeof(volumetric_fog_t));
}

void volumetric_fog_destroy(volumetric_fog_t *fog) { free(fog); }

static void expect_true(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failures++;
  }
}

static int nearf(f32 a, f32 b) { return fabsf(a - b) < 0.0005f; }

static void expect_f(f32 got, f32 want, const char *msg) {
  if (!nearf(got, want)) {
    fprintf(stderr, "FAIL: %s (got %g want %g)\n", msg, (double)got,
            (double)want);
    g_failures++;
  }
}

static void expect_vec(Vec3 got, f32 x, f32 y, f32 z, const char *msg) {
  if (!nearf(got.x, x) || !nearf(got.y, y) || !nearf(got.z, z)) {
    fprintf(stderr, "FAIL: %s (got %g %g %g want %g %g %g)\n", msg,
            (double)got.x, (double)got.y, (double)got.z, (double)x, (double)y,
            (double)z);
    g_failures++;
  }
}

static f32 vec_len(Vec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static void test_null_and_create_failure(void) {
  Vec3 sun;
  Vec3 ambient;

  atmospheric_effects_destroy(NULL);
  atmospheric_effects_set_time_of_day(NULL, 12.0f);
  atmospheric_effects_advance_time(NULL, 1.0f);
  atmospheric_effects_set_weather(NULL, WEATHER_LIGHT_RAIN, 1.0f);
  atmospheric_effects_set_weather_immediate(NULL, WEATHER_FOG);
  atmospheric_effects_update(NULL, 0.016f);
  atmospheric_effects_update_parameters(NULL);
  atmospheric_effects_update_clouds(NULL, 0.016f);
  atmospheric_effects_update_precipitation(NULL, 0.016f);
  atmospheric_effects_update_lightning(NULL, 0.016f);
  atmospheric_effects_trigger_lightning_strike(NULL, (Vec3){1.0f, 2.0f, 3.0f});
  atmospheric_effects_create_splash_effects(NULL, (Vec3){0.0f, 0.0f, 0.0f});

  sun = atmospheric_effects_get_sun_color(NULL);
  expect_vec(sun, 1.0f, 1.0f, 1.0f, "null sun color");
  ambient = atmospheric_effects_get_ambient_color(NULL);
  expect_vec(ambient, 0.2f, 0.2f, 0.3f, "null ambient color");
  expect_f(atmospheric_effects_get_visibility_distance(NULL), 1000.0f,
           "null visibility");

  g_fail_fog_create = 1;
  expect_true(atmospheric_effects_create(NULL) == NULL, "fog create failure");
  g_fail_fog_create = 0;
}

static void test_create_defaults(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);
  Vec3 sun;
  f32 len;

  expect_true(system != NULL, "create");
  if (!system) {
    return;
  }

  expect_true(system->time_of_day_hours == 12.0f, "noon hours");
  expect_true(system->current_time == TIME_NOON, "noon category");
  expect_true(system->current_weather == WEATHER_CLEAR, "default weather");
  expect_true(system->current_season == SEASON_SUMMER, "default season");
  expect_true(system->high_quality_mode, "high quality");
  expect_true(system->params.volumetric_samples == 64, "volumetric samples");
  expect_true(system->params.scattering_samples == 32, "scattering samples");
  expect_true(system->params.enable_aerial_perspective, "aerial perspective");
  expect_true(system->params.enable_multiple_scattering, "multiple scattering");
  expect_true(system->clouds.max_count == 128, "cloud capacity");
  expect_true(system->clouds.positions != NULL, "cloud positions");
  expect_true(system->precipitation.max_count == 2048, "precip capacity");
  expect_true(system->precipitation.particles != NULL, "precip particles");
  expect_true(system->volumetric_fog != NULL, "volumetric fog");
  expect_true(system->atmosphere_scattering != NULL, "scattering system");

  len = vec_len(system->params.sun_direction);
  expect_true(fabsf(len - 1.0f) < 0.001f, "sun direction is unit length");
  expect_true(system->params.sun_direction.y > 0.0f, "noon sun is up");

  sun = atmospheric_effects_get_sun_color(system);
  expect_vec(sun, 1.0f, 0.95f, 0.8f, "default sun color");
  expect_f(atmospheric_effects_get_visibility_distance(system), 1000.0f,
           "default visibility");
  expect_f(system->volumetric_fog->density_scale, 0.0f, "initial fog scale");
  expect_f(system->atmosphere_scattering->mie_coeff, 2.0e-5f, "mie coeff");
  expect_f(system->atmosphere_scattering->rayleigh_coeff.x, 5.8e-6f,
           "rayleigh r");
  expect_f(system->atmosphere_scattering->rayleigh_coeff.y, 5.8e-6f * 0.8f,
           "rayleigh g");
  expect_f(system->atmosphere_scattering->rayleigh_coeff.z, 5.8e-6f * 1.2f,
           "rayleigh b");
  expect_f(system->atmosphere_scattering->sun_direction.y,
           system->params.sun_direction.y, "scattering sun y");

  atmospheric_effects_destroy(system);
}

static void test_time_of_day(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);
  Vec3 sun;
  Vec3 ambient;
  struct {
    f32 hours;
    TimeOfDay category;
  } cases[] = {
      {5.0f, TIME_DAWN},       {6.9f, TIME_DAWN},
      {7.0f, TIME_MORNING},    {9.9f, TIME_MORNING},
      {10.0f, TIME_NOON},      {13.9f, TIME_NOON},
      {14.0f, TIME_AFTERNOON}, {16.9f, TIME_AFTERNOON},
      {17.0f, TIME_DUSK},      {18.9f, TIME_DUSK},
      {19.0f, TIME_EVENING},   {21.9f, TIME_EVENING},
      {22.0f, TIME_NIGHT},     {1.9f, TIME_NIGHT},
      {2.0f, TIME_MIDNIGHT},   {4.9f, TIME_MIDNIGHT},
      {24.0f, TIME_NIGHT},     {25.0f, TIME_NIGHT},
      {-3.0f, TIME_EVENING},
  };
  size_t i;

  expect_true(system != NULL, "time system");
  if (!system) {
    return;
  }

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    atmospheric_effects_set_time_of_day(system, cases[i].hours);
    if (system->current_time != cases[i].category) {
      fprintf(stderr, "FAIL: hour %g category %d want %d\n",
              (double)cases[i].hours, (int)system->current_time,
              (int)cases[i].category);
      g_failures++;
    }
  }

  atmospheric_effects_set_time_of_day(system, 25.0f);
  expect_f(system->time_of_day_hours, 1.0f, "wrap 25 to 1");
  atmospheric_effects_set_time_of_day(system, -3.0f);
  expect_f(system->time_of_day_hours, 21.0f, "wrap -3 to 21");

  atmospheric_effects_set_time_of_day(system, 10.0f);
  atmospheric_effects_advance_time(system, 5.0f);
  expect_f(system->time_of_day_hours, 15.0f, "advance to 15");
  expect_true(system->current_time == TIME_AFTERNOON, "afternoon after advance");
  atmospheric_effects_advance_time(system, 20.0f);
  expect_f(system->time_of_day_hours, 11.0f, "advance wraps to 11");
  expect_true(system->current_time == TIME_NOON, "noon after wrap advance");

  atmospheric_effects_set_time_of_day(system, 12.0f);
  expect_true(system->params.sun_direction.y > 0.0f, "day sun elevation");
  expect_true(system->params.sun_intensity > 0.5f, "day sun intensity");
  expect_true(system->params.moon_intensity == 0.0f, "day moon off");
  expect_true(fabsf(vec_len(system->params.sun_direction) - 1.0f) < 0.001f,
              "day sun unit");
  sun = atmospheric_effects_get_sun_color(system);
  expect_f(sun.x, system->params.sun_intensity, "day sun color r");
  expect_f(sun.y, 0.95f * system->params.sun_intensity, "day sun color g");
  expect_f(sun.z, 0.8f * system->params.sun_intensity, "day sun color b");

  atmospheric_effects_set_time_of_day(system, 0.0f);
  expect_true(system->params.sun_direction.y < 0.0f, "night sun elevation");
  expect_f(system->params.sun_intensity, 0.0f, "night sun intensity");
  expect_f(system->params.moon_intensity, 0.1f, "night moon intensity");
  sun = atmospheric_effects_get_sun_color(system);
  expect_vec(sun, 0.0f, 0.0f, 0.0f, "night sun color");
  expect_vec(system->params.zenith_color, 0.05f, 0.05f, 0.1f, "night zenith");
  expect_vec(system->params.horizon_color, 0.1f, 0.1f, 0.15f, "night horizon");

  system->params.sky_brightness = 1.0f;
  ambient = atmospheric_effects_get_ambient_color(system);
  expect_vec(ambient, 0.075f * 0.3f, 0.075f * 0.3f, 0.125f * 0.3f,
             "night ambient");

  atmospheric_effects_destroy(system);
}

static void test_weather_and_visibility(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);
  int frame;

  expect_true(system != NULL, "weather system");
  if (!system) {
    return;
  }

  atmospheric_effects_set_weather_immediate(system, WEATHER_FOG);
  expect_true(system->current_weather == WEATHER_FOG, "fog weather");
  expect_true(system->target_weather == WEATHER_FOG, "fog target");
  expect_f(system->params.fog_density, 0.2f, "fog density");
  expect_f(system->params.cloud_coverage, 0.3f, "fog clouds");
  expect_f(system->params.precipitation_intensity, 0.0f, "fog precip");
  expect_f(system->params.wind_strength, 0.2f, "fog wind");
  expect_vec(system->params.fog_color, 0.7f, 0.7f, 0.8f, "fog color");
  expect_f(atmospheric_effects_get_visibility_distance(system),
           1.0f / 0.201f, "fog visibility");

  atmospheric_effects_update_parameters(system);
  expect_f(system->volumetric_fog->density_scale, 0.2f, "fog density scale");
  expect_f(system->volumetric_fog->fog_color.x, 0.7f, "volumetric fog r");
  expect_f(system->volumetric_fog->fog_color.y, 0.7f, "volumetric fog g");
  expect_f(system->volumetric_fog->fog_color.z, 0.8f, "volumetric fog b");

  atmospheric_effects_set_weather_immediate(system, WEATHER_CLEAR);
  expect_f(system->params.fog_density, 0.001f, "clear density");
  expect_f(system->params.cloud_coverage, 0.1f, "clear clouds");
  expect_vec(system->params.wind_direction, 1.0f, 0.0f, 0.0f, "clear wind");

  atmospheric_effects_set_weather_immediate(system, WEATHER_MIST);
  expect_f(system->params.fog_density, 0.001f, "mist uses default profile");
  expect_vec(system->params.fog_color, 0.8f, 0.9f, 1.0f, "mist fog color");

  atmospheric_effects_set_weather_immediate(system, WEATHER_CLEAR);
  atmospheric_effects_set_weather(system, WEATHER_HEAVY_RAIN, 10.0f);
  expect_true(system->current_weather == WEATHER_CLEAR, "transition not snapped");
  expect_true(system->target_weather == WEATHER_HEAVY_RAIN, "rain target");
  expect_f(system->weather_transition_duration, 10.0f, "transition duration");
  atmospheric_effects_update(system, 9.9f);
  expect_true(system->current_weather == WEATHER_CLEAR, "still clear at 9.9");
  expect_f(system->params.fog_density, 0.001f + (0.05f - 0.001f) * 0.99f,
           "partial fog blend");
  atmospheric_effects_update(system, 0.1f);
  expect_true(system->current_weather == WEATHER_HEAVY_RAIN, "rain completed");
  expect_f(system->params.fog_density, 0.05f, "heavy rain density");
  expect_f(system->params.cloud_coverage, 1.0f, "heavy rain clouds");
  expect_f(system->params.precipitation_intensity, 0.8f, "heavy rain precip");
  expect_f(system->params.wind_strength, 5.0f, "heavy rain wind");
  expect_vec(system->params.wind_direction, 0.2f, 0.0f, 0.8f, "rain wind dir");
  expect_vec(system->params.fog_color, 0.4f, 0.4f, 0.5f, "rain fog color");

  atmospheric_effects_set_weather(system, WEATHER_BLIZZARD, 0.0f);
  expect_f(system->weather_transition_duration, 30.0f, "default transition");
  atmospheric_effects_update(system, 30.0f);
  expect_true(system->current_weather == WEATHER_BLIZZARD, "blizzard done");
  expect_f(system->params.fog_density, 0.15f, "blizzard density");
  expect_f(system->params.wind_strength, 12.0f, "blizzard wind");
  expect_f(system->params.precipitation_intensity, 1.0f, "blizzard precip");

  system->frame_counter = 0;
  for (frame = 0; frame < 60; frame++) {
    atmospheric_effects_update(system, 0.02f);
  }
  expect_true(system->frame_counter == 60, "frame counter");
  expect_f(system->average_frame_time, 0.02f, "average frame time");

  atmospheric_effects_destroy(system);
}

static void test_weather_names(void) {
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_CLEAR), "Clear") ==
                  0,
              "clear name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_PARTLY_CLOUDY),
                     "Partly Cloudy") == 0,
              "partly name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_OVERCAST),
                     "Overcast") == 0,
              "overcast name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_LIGHT_RAIN),
                     "Light Rain") == 0,
              "light rain name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_HEAVY_RAIN),
                     "Heavy Rain") == 0,
              "heavy rain name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_THUNDERSTORM),
                     "Thunderstorm") == 0,
              "storm name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_LIGHT_SNOW),
                     "Light Snow") == 0,
              "light snow name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_HEAVY_SNOW),
                     "Heavy Snow") == 0,
              "heavy snow name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_BLIZZARD),
                     "Blizzard") == 0,
              "blizzard name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_FOG), "Fog") == 0,
              "fog name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_MIST), "Mist") == 0,
              "mist name");
  expect_true(strcmp(atmospheric_weather_to_string(WEATHER_HAZE), "Haze") == 0,
              "haze name");
  expect_true(strcmp(atmospheric_weather_to_string((WeatherCondition)99),
                     "Unknown") == 0,
              "unknown weather name");
}

static void test_clouds(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);
  u32 i;

  expect_true(system != NULL, "cloud system");
  if (!system) {
    return;
  }

  atmospheric_effects_set_weather_immediate(system, WEATHER_CLEAR);
  atmospheric_effects_update_clouds(system, 0.016f);
  expect_true(system->clouds.count == 12, "clear cloud count");
  for (i = 0; i < system->clouds.count; i++) {
    if (system->clouds.positions[i].y < 500.0f ||
        system->clouds.positions[i].y > 800.0f) {
      fprintf(stderr, "FAIL: cloud y out of range %g\n",
              (double)system->clouds.positions[i].y);
      g_failures++;
      break;
    }
    if (system->clouds.densities[i] < 0.3f ||
        system->clouds.densities[i] > 1.0f) {
      fprintf(stderr, "FAIL: cloud density out of range %g\n",
              (double)system->clouds.densities[i]);
      g_failures++;
      break;
    }
  }

  system->clouds.positions[0].x = 1499.6f;
  system->clouds.densities[0] = 50.0f;
  system->clouds.densities[1] = -5.0f;
  atmospheric_effects_update_clouds(system, 1.0f);
  expect_f(system->clouds.positions[0].x, -1500.0f, "cloud wrap +x");
  expect_f(system->clouds.densities[0], 1.0f, "density clamp high");
  expect_f(system->clouds.densities[1], 0.1f, "density clamp low");

  system->clouds.positions[0].x = -1499.6f;
  system->params.wind_direction = (Vec3){-1.0f, 0.0f, 0.0f};
  system->params.wind_strength = 0.5f;
  atmospheric_effects_update_clouds(system, 1.0f);
  expect_f(system->clouds.positions[0].x, 1500.0f, "cloud wrap -x");

  system->clouds.positions[0].z = 1600.0f;
  system->params.wind_direction = (Vec3){0.0f, 0.0f, 1.0f};
  system->params.wind_strength = 1.0f;
  atmospheric_effects_update_clouds(system, 1.0f);
  expect_f(system->clouds.positions[0].z, -1500.0f, "cloud wrap +z");

  system->params.cloud_coverage = 0.0f;
  system->clouds.count = 3;
  atmospheric_effects_update_clouds(system, 1.0f);
  expect_true(system->clouds.count == 3, "no cloud update when coverage is 0");

  atmospheric_effects_destroy(system);
}

static void test_precipitation(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);

  expect_true(system != NULL, "precip system");
  if (!system) {
    return;
  }

  atmospheric_effects_set_weather_immediate(system, WEATHER_HEAVY_RAIN);
  atmospheric_effects_update_precipitation(system, 1.0f);
  expect_true(system->precipitation.count == 80, "spawn 80 rain drops");

  system->precipitation.count = 1;
  system->precipitation.particles[0] = (Vec3){4.0f, -1.0f, 5.0f};
  system->precipitation.velocities[0] = (Vec3){0.0f, 0.0f, 0.0f};
  system->precipitation.lifetimes[0] = 0.0f;
  system->precipitation.sizes[0] = 1.0f;
  atmospheric_effects_update_precipitation(system, 0.001f);
  expect_true(system->precipitation.count == 0, "ground hit removes drop");

  system->precipitation.count = 1;
  system->precipitation.particles[0] = (Vec3){1.0f, 50.0f, 1.0f};
  system->precipitation.velocities[0] = (Vec3){0.0f, 0.0f, 0.0f};
  system->precipitation.lifetimes[0] = 10.5f;
  atmospheric_effects_update_precipitation(system, 0.001f);
  expect_true(system->precipitation.count == 0, "expired drop removed");

  system->params.precipitation_intensity = 0.0f;
  system->precipitation.count = 4;
  atmospheric_effects_update_precipitation(system, 1.0f);
  expect_true(system->precipitation.count == 4, "no precip when intensity is 0");

  atmospheric_effects_destroy(system);
}

static void test_lightning(void) {
  AtmosphericEffectsSystem *system = atmospheric_effects_create(NULL);
  Vec3 pos = {12.0f, 80.0f, -4.0f};
  int i;

  expect_true(system != NULL, "lightning system");
  if (!system) {
    return;
  }

  atmospheric_effects_set_weather_immediate(system, WEATHER_CLEAR);
  atmospheric_effects_update_lightning(system, 5.0f);
  expect_true(system->lightning.active_strikes == 0, "no lightning in clear");

  system->lightning.strike_duration = 1.5f;
  atmospheric_effects_trigger_lightning_strike(system, pos);
  expect_true(system->lightning.active_strikes == 1, "one strike");
  expect_vec(system->lightning.strike_positions[0], 12.0f, 80.0f, -4.0f,
             "strike position");
  expect_f(system->lightning.strike_times[0], 1.5f, "strike duration");

  atmospheric_effects_set_weather_immediate(system, WEATHER_THUNDERSTORM);
  system->lightning.next_strike_time = 100.0f;
  atmospheric_effects_update_lightning(system, 0.5f);
  expect_true(system->lightning.active_strikes == 1, "strike still active");
  expect_f(system->lightning.strike_times[0], 1.0f, "strike time decay");
  expect_f(system->lightning.next_strike_time, 99.5f, "next strike countdown");

  atmospheric_effects_update_lightning(system, 1.0f);
  expect_true(system->lightning.active_strikes == 0, "strike expired");
  expect_f(system->lightning.next_strike_time, 98.5f, "no extra strike");

  system->lightning.strike_duration = 2.0f;
  for (i = 0; i < 17; i++) {
    atmospheric_effects_trigger_lightning_strike(
        system, (Vec3){(f32)i, 10.0f, 3.0f});
  }
  expect_true(system->lightning.active_strikes == 16, "strike cap");
  expect_f(system->lightning.strike_positions[0].x, 0.0f, "first strike kept");
  expect_f(system->lightning.strike_positions[15].x, 15.0f,
           "sixteenth strike kept");

  atmospheric_effects_destroy(system);
}

int main(void) {
  test_null_and_create_failure();
  test_create_defaults();
  test_time_of_day();
  test_weather_and_visibility();
  test_weather_names();
  test_clouds();
  test_precipitation();
  test_lightning();

  if (g_failures != 0) {
    fprintf(stderr, "atmospheric effects: %d failure(s)\n", g_failures);
    return 1;
  }
  printf("atmospheric effects ok\n");
  return 0;
}
