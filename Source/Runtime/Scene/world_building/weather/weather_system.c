#include "weather_system.h"
#include "unified_memory_allocator.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEATHER_TYPE_COUNT 15u
#define CLOUD_RGBA_STRIDE 4u

static uint32_t weather_rng_state = 0x6a09e667u;

static float clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static uint32_t lcg_next(uint32_t *state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static float lcg_float(uint32_t *state) {
  return (float)(lcg_next(state) & 0x00ffffffu) / (float)0x01000000u;
}

static float hash2d(uint32_t x, uint32_t y, uint32_t seed) {
  uint32_t h = x * 374761393u + y * 668265263u + seed * 2654435761u;
  h = (h ^ (h >> 13u)) * 1274126177u;
  h ^= h >> 16u;
  return (float)(h & 0x00ffffffu) / (float)0x01000000u;
}

static void copy_vec3(float dst[3], const float src[3]) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
}

static bool weather_type_has_precipitation(WeatherType type) {
  switch (type) {
  case WEATHER_LIGHT_RAIN:
  case WEATHER_RAIN:
  case WEATHER_HEAVY_RAIN:
  case WEATHER_THUNDERSTORM:
  case WEATHER_LIGHT_SNOW:
  case WEATHER_SNOW:
  case WEATHER_BLIZZARD:
  case WEATHER_HAIL:
  case WEATHER_SLEET:
    return true;
  default:
    return false;
  }
}

static float weather_type_fall_speed(WeatherType type) {
  switch (type) {
  case WEATHER_LIGHT_SNOW:
  case WEATHER_SNOW:
  case WEATHER_BLIZZARD:
    return 3.0f;
  case WEATHER_HAIL:
    return 20.0f;
  default:
    return 15.0f;
  }
}

static void weather_params_set_defaults(WeatherParameters *params,
                                        WeatherType type) {
  if (!params) {
    return;
  }

  memset(params, 0, sizeof(*params));
  params->type = type;
  snprintf(params->name, sizeof(params->name), "Weather%u",
           (unsigned)type);

  params->sky_color[0] = 0.52f;
  params->sky_color[1] = 0.73f;
  params->sky_color[2] = 1.0f;
  params->horizon_color[0] = 0.8f;
  params->horizon_color[1] = 0.9f;
  params->horizon_color[2] = 1.0f;
  params->cloud_coverage = 0.2f;
  params->cloud_speed = 0.2f;
  params->cloud_height = 1200.0f;
  params->cloud_texture = 0u;

  params->has_precipitation = false;
  params->precipitation_intensity = 0.0f;
  params->precipitation_texture = 0u;
  params->precipitation_size = 0.02f;
  params->precipitation_speed = 12.0f;
  params->precipitation_affects_water = true;

  params->fog_density = 0.0f;
  params->fog_height_falloff = 0.1f;
  params->fog_color[0] = 0.9f;
  params->fog_color[1] = 0.9f;
  params->fog_color[2] = 0.95f;
  params->fog_start_distance = 100.0f;
  params->fog_end_distance = 1200.0f;

  params->sun_intensity_multiplier = 1.0f;
  params->ambient_intensity_multiplier = 1.0f;
  params->shadow_intensity_multiplier = 1.0f;

  params->wind_speed = 2.0f;
  params->wind_direction[0] = 1.0f;
  params->wind_direction[1] = 0.0f;
  params->wind_direction[2] = 0.0f;
  params->wind_gustiness = 0.1f;

  snprintf(params->ambient_sound, sizeof(params->ambient_sound), "clear");
  params->ambient_volume = 0.5f;

  params->visibility_range = 1000.0f;
  params->movement_speed_modifier = 1.0f;
  params->fire_extinguish_rate = 0.0f;

  switch (type) {
  case WEATHER_PARTLY_CLOUDY:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "wind_light");
    params->cloud_coverage = 0.4f;
    params->cloud_speed = 0.3f;
    break;
  case WEATHER_CLOUDY:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "wind_medium");
    params->cloud_coverage = 0.7f;
    params->sun_intensity_multiplier = 0.9f;
    break;
  case WEATHER_OVERCAST:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "wind_heavy");
    params->cloud_coverage = 0.9f;
    params->sun_intensity_multiplier = 0.75f;
    params->ambient_intensity_multiplier = 0.85f;
    break;
  case WEATHER_LIGHT_RAIN:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound), "rain");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.3f;
    params->fog_density = 0.02f;
    params->wind_speed = 4.0f;
    params->visibility_range = 650.0f;
    break;
  case WEATHER_RAIN:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "rain_medium");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.6f;
    params->fog_density = 0.04f;
    params->wind_speed = 6.0f;
    params->visibility_range = 450.0f;
    params->sun_intensity_multiplier = 0.8f;
    break;
  case WEATHER_HEAVY_RAIN:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "rain_heavy");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.9f;
    params->fog_density = 0.06f;
    params->wind_speed = 8.0f;
    params->visibility_range = 300.0f;
    params->sun_intensity_multiplier = 0.7f;
    params->fire_extinguish_rate = 1.0f;
    break;
  case WEATHER_THUNDERSTORM:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "thunder");
    params->has_precipitation = true;
    params->precipitation_intensity = 1.0f;
    params->fog_density = 0.08f;
    params->wind_speed = 10.0f;
    params->wind_gustiness = 0.7f;
    params->visibility_range = 250.0f;
    params->sun_intensity_multiplier = 0.6f;
    params->fire_extinguish_rate = 1.2f;
    break;
  case WEATHER_LIGHT_SNOW:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound), "snow");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.3f;
    params->precipitation_size = 0.04f;
    params->precipitation_speed = 4.0f;
    params->visibility_range = 700.0f;
    params->movement_speed_modifier = 0.98f;
    break;
  case WEATHER_SNOW:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "snow_medium");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.6f;
    params->precipitation_size = 0.04f;
    params->precipitation_speed = 3.5f;
    params->fog_density = 0.03f;
    params->visibility_range = 500.0f;
    params->movement_speed_modifier = 0.95f;
    break;
  case WEATHER_BLIZZARD:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "blizzard");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.9f;
    params->precipitation_size = 0.05f;
    params->precipitation_speed = 3.0f;
    params->fog_density = 0.09f;
    params->wind_speed = 15.0f;
    params->wind_gustiness = 0.8f;
    params->visibility_range = 150.0f;
    params->movement_speed_modifier = 0.85f;
    break;
  case WEATHER_FOG:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound), "fog");
    params->fog_density = 0.12f;
    params->visibility_range = 200.0f;
    params->sun_intensity_multiplier = 0.65f;
    params->ambient_intensity_multiplier = 0.8f;
    break;
  case WEATHER_SANDSTORM:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound),
             "sandstorm");
    params->fog_density = 0.15f;
    params->wind_speed = 18.0f;
    params->wind_gustiness = 0.9f;
    params->visibility_range = 100.0f;
    params->movement_speed_modifier = 0.9f;
    break;
  case WEATHER_HAIL:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound), "hail");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.7f;
    params->precipitation_size = 0.03f;
    params->precipitation_speed = 18.0f;
    params->visibility_range = 350.0f;
    params->movement_speed_modifier = 0.92f;
    break;
  case WEATHER_SLEET:
    snprintf(params->ambient_sound, sizeof(params->ambient_sound), "sleet");
    params->has_precipitation = true;
    params->precipitation_intensity = 0.5f;
    params->precipitation_size = 0.025f;
    params->precipitation_speed = 10.0f;
    params->visibility_range = 500.0f;
    params->movement_speed_modifier = 0.96f;
    break;
  default:
    break;
  }
}

static void weather_params_lerp_local(WeatherParameters *out,
                                      const WeatherParameters *from_params,
                                      const WeatherParameters *to_params,
                                      float t) {
  if (!out || !from_params || !to_params) {
    return;
  }
  t = clamp01(t);
  *out = *from_params;
  out->type = (t < 0.5f) ? from_params->type : to_params->type;
  snprintf(out->name, sizeof(out->name), "%s",
           (t < 0.5f) ? from_params->name : to_params->name);

  for (size_t i = 0; i < 3; ++i) {
    out->sky_color[i] = lerp(from_params->sky_color[i], to_params->sky_color[i], t);
    out->horizon_color[i] =
        lerp(from_params->horizon_color[i], to_params->horizon_color[i], t);
    out->fog_color[i] = lerp(from_params->fog_color[i], to_params->fog_color[i], t);
    out->wind_direction[i] =
        lerp(from_params->wind_direction[i], to_params->wind_direction[i], t);
  }

  out->cloud_coverage =
      lerp(from_params->cloud_coverage, to_params->cloud_coverage, t);
  out->cloud_speed = lerp(from_params->cloud_speed, to_params->cloud_speed, t);
  out->cloud_height =
      lerp(from_params->cloud_height, to_params->cloud_height, t);
  out->cloud_texture =
      (t < 0.5f) ? from_params->cloud_texture : to_params->cloud_texture;

  out->has_precipitation =
      (t < 0.5f) ? from_params->has_precipitation : to_params->has_precipitation;
  out->precipitation_intensity = lerp(from_params->precipitation_intensity,
                                      to_params->precipitation_intensity, t);
  out->precipitation_texture = (t < 0.5f)
                                   ? from_params->precipitation_texture
                                   : to_params->precipitation_texture;
  out->precipitation_size =
      lerp(from_params->precipitation_size, to_params->precipitation_size, t);
  out->precipitation_speed = lerp(from_params->precipitation_speed,
                                  to_params->precipitation_speed, t);
  out->precipitation_affects_water =
      (t < 0.5f) ? from_params->precipitation_affects_water
                 : to_params->precipitation_affects_water;

  out->fog_density = lerp(from_params->fog_density, to_params->fog_density, t);
  out->fog_height_falloff = lerp(from_params->fog_height_falloff,
                                 to_params->fog_height_falloff, t);
  out->fog_start_distance = lerp(from_params->fog_start_distance,
                                 to_params->fog_start_distance, t);
  out->fog_end_distance =
      lerp(from_params->fog_end_distance, to_params->fog_end_distance, t);

  out->sun_intensity_multiplier = lerp(from_params->sun_intensity_multiplier,
                                       to_params->sun_intensity_multiplier, t);
  out->ambient_intensity_multiplier =
      lerp(from_params->ambient_intensity_multiplier,
           to_params->ambient_intensity_multiplier, t);
  out->shadow_intensity_multiplier =
      lerp(from_params->shadow_intensity_multiplier,
           to_params->shadow_intensity_multiplier, t);

  out->wind_speed = lerp(from_params->wind_speed, to_params->wind_speed, t);
  out->wind_gustiness =
      lerp(from_params->wind_gustiness, to_params->wind_gustiness, t);

  snprintf(out->ambient_sound, sizeof(out->ambient_sound), "%s",
           (t < 0.5f) ? from_params->ambient_sound : to_params->ambient_sound);
  out->ambient_volume =
      lerp(from_params->ambient_volume, to_params->ambient_volume, t);

  out->visibility_range =
      lerp(from_params->visibility_range, to_params->visibility_range, t);
  out->movement_speed_modifier = lerp(from_params->movement_speed_modifier,
                                      to_params->movement_speed_modifier, t);
  out->fire_extinguish_rate = lerp(from_params->fire_extinguish_rate,
                                   to_params->fire_extinguish_rate, t);
}

void cloud_weather_map(const VolumetricCloudSettings *settings, float time,
                       float *out_map, uint32_t width, uint32_t height) {
  if (!settings || !out_map || width == 0 || height == 0) {
    return;
  }

  float coverage = clamp01(settings->coverage);
  float wind_x = settings->wind_direction[0] * settings->wind_speed * time;
  float wind_y = settings->wind_direction[1] * settings->wind_speed * time;

  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      float nx = (float)x / (float)width;
      float ny = (float)y / (float)height;
      uint32_t sx = (uint32_t)(nx * 1024.0f + wind_x * 4.0f);
      uint32_t sy = (uint32_t)(ny * 1024.0f + wind_y * 4.0f);
      float base = hash2d(sx, sy, 0x9e3779b9u);
      float detail = hash2d(sx * 2u, sy * 2u, 0x85ebca6bu);
      float shape = lerp(base, detail, clamp01(settings->detail_scale * 0.5f));
      float density =
          clamp01(shape * settings->density + coverage - 0.25f);
      density = lerp(density, density * density, settings->cloud_type);
      out_map[y * width + x] = density;
    }
  }
}

void cloud_temporal_reprojection(const float *previous_map, float *current_map,
                                 uint32_t width, uint32_t height,
                                 float blend_factor) {
  if (!previous_map || !current_map || width == 0 || height == 0) {
    return;
  }

  float t = clamp01(blend_factor);
  uint32_t count = width * height;
  for (uint32_t i = 0; i < count; ++i) {
    current_map[i] = lerp(current_map[i], previous_map[i], t);
  }
}

void cloud_render(const VolumetricCloudSettings *settings,
                  const float *weather_map, uint32_t width, uint32_t height,
                  float *out_rgba) {
  if (!settings || !out_rgba || width == 0 || height == 0) {
    return;
  }

  uint32_t count = width * height;
  for (uint32_t i = 0; i < count; ++i) {
    float density =
        weather_map ? weather_map[i] : clamp01(settings->coverage);
    float alpha = clamp01(density * settings->density);
    float shade = 0.75f + 0.2f * density - 0.1f * settings->ambient_occlusion;
    uint32_t base = i * CLOUD_RGBA_STRIDE;
    out_rgba[base + 0u] = shade;
    out_rgba[base + 1u] = shade;
    out_rgba[base + 2u] = shade;
    out_rgba[base + 3u] = alpha;
  }
}

void precipitation_init(PrecipitationSystem *system, WeatherType type) {
  if (!system) {
    return;
  }

  memset(system, 0, sizeof(*system));
  system->type = type;
  system->max_particles = 10000u;
  system->particle_size = 0.02f;
  system->streak_length = 0.1f;
  system->opacity = 0.0f;
  system->fall_speed = weather_type_fall_speed(type);
  system->wind_influence = 0.5f;
  system->collision_offset = 0.1f;
  system->spawn_splashes = (type == WEATHER_LIGHT_RAIN ||
                            type == WEATHER_RAIN || type == WEATHER_HEAVY_RAIN ||
                            type == WEATHER_THUNDERSTORM || type == WEATHER_HAIL ||
                            type == WEATHER_SLEET);

  system->active = weather_type_has_precipitation(type);
  system->intensity = 0.0f;
}

void precipitation_update(PrecipitationSystem *system, float delta_time) {
  if (!system) {
    return;
  }

  if (!system->active) {
    system->active_particles = 0u;
    system->opacity = 0.0f;
    return;
  }

  float intensity = clamp01(system->intensity);
  uint32_t target = (uint32_t)(system->max_particles * intensity);
  if (system->active_particles < target) {
    precipitation_emit(system, target - system->active_particles);
  } else if (system->active_particles > target) {
    uint32_t reduce = (system->active_particles - target) / 2u;
    system->active_particles -= reduce;
  }

  precipitation_simulate(system, delta_time);
  precipitation_render(system);
  if (system->spawn_splashes) {
    precipitation_splash(system, system->active_particles / 200u);
  }
}

void precipitation_emit(PrecipitationSystem *system, uint32_t emit_count) {
  if (!system || emit_count == 0u) {
    return;
  }

  uint32_t available =
      system->max_particles - system->active_particles;
  uint32_t to_emit = emit_count < available ? emit_count : available;
  system->active_particles += to_emit;
}

void precipitation_simulate(PrecipitationSystem *system, float delta_time) {
  if (!system || system->active_particles == 0u) {
    return;
  }

  float decay = clamp01(delta_time * 0.25f);
  uint32_t remove =
      (uint32_t)(system->active_particles * decay);
  if (remove > system->active_particles) {
    remove = system->active_particles;
  }
  system->active_particles -= remove;
}

void precipitation_render(PrecipitationSystem *system) {
  if (!system) {
    return;
  }

  float intensity = clamp01(system->intensity);
  system->opacity = intensity;
  system->streak_length = system->particle_size * (1.0f + system->fall_speed * 0.05f);
}

void precipitation_collision(PrecipitationSystem *system,
                             float ground_height) {
  if (!system || system->active_particles == 0u) {
    return;
  }

  float collision_factor =
      ground_height > 0.0f ? 0.05f : 0.02f;
  uint32_t remove =
      (uint32_t)(system->active_particles * collision_factor);
  if (remove > system->active_particles) {
    remove = system->active_particles;
  }
  system->active_particles -= remove;
}

void precipitation_splash(PrecipitationSystem *system, uint32_t splash_count) {
  if (!system || !system->spawn_splashes) {
    return;
  }

  system->splash_rate =
      clamp01(system->splash_rate + 0.001f * (float)splash_count);
  system->splash_size = system->particle_size * (1.0f + system->intensity);
}

float precipitation_accumulation(float current_accumulation,
                                 const PrecipitationSystem *system,
                                 float delta_time) {
  if (!system || !system->active) {
    return current_accumulation;
  }

  float rate = clamp01(system->intensity) * delta_time;
  switch (system->type) {
  case WEATHER_LIGHT_SNOW:
  case WEATHER_SNOW:
  case WEATHER_BLIZZARD:
    rate *= 1.5f;
    break;
  case WEATHER_HAIL:
    rate *= 1.2f;
    break;
  default:
    break;
  }
  return current_accumulation + rate;
}

bool lightning_generate_bolt(LightningSystem *system, const float start[3],
                             const float end[3]) {
  if (!system || !system->bolts || system->bolt_count >= system->max_bolts) {
    return false;
  }

  LightningBolt *bolt = &system->bolts[system->bolt_count++];
  memset(bolt, 0, sizeof(*bolt));
  copy_vec3(bolt->start, start);
  copy_vec3(bolt->end, end);
  bolt->intensity = 1.0f;
  bolt->lifetime = system->bolt_lifetime > 0.0f ? system->bolt_lifetime : 0.5f;
  bolt->age = 0.0f;
  bolt->has_flash = true;
  bolt->flash_intensity = 1.0f;
  bolt->segment_count = 2u;
  copy_vec3(bolt->segments[0], start);
  copy_vec3(bolt->segments[1], end);
  lightning_subdivide(bolt, 2.5f);
  return true;
}

void lightning_subdivide(LightningBolt *bolt, float displacement) {
  if (!bolt || bolt->segment_count < 2u) {
    return;
  }

  uint32_t rng = 0x1234abcdu;
  float temp[32][3];
  uint32_t count = bolt->segment_count;

  for (uint32_t iter = 0; iter < 4u && count < 32u; ++iter) {
    uint32_t new_count = 0u;
    float scale = displacement / (float)(iter + 1u);
    for (uint32_t i = 0; i + 1u < count && new_count + 2u < 32u; ++i) {
      float *a = bolt->segments[i];
      float *b = bolt->segments[i + 1u];
      float mid[3];
      mid[0] = 0.5f * (a[0] + b[0]);
      mid[1] = 0.5f * (a[1] + b[1]);
      mid[2] = 0.5f * (a[2] + b[2]);
      mid[0] += (lcg_float(&rng) - 0.5f) * scale;
      mid[1] += (lcg_float(&rng) - 0.5f) * scale;
      mid[2] += (lcg_float(&rng) - 0.5f) * scale;

      copy_vec3(temp[new_count++], a);
      if (new_count < 32u) {
        copy_vec3(temp[new_count++], mid);
      }
    }
    if (new_count < 32u) {
      copy_vec3(temp[new_count++], bolt->segments[count - 1u]);
    }
    count = new_count;
    for (uint32_t i = 0; i < count; ++i) {
      copy_vec3(bolt->segments[i], temp[i]);
    }
  }

  bolt->segment_count = count;
}

void lightning_render(LightningSystem *system) {
  (void)system;
}

void lightning_flash(LightningBolt *bolt, float intensity, float duration) {
  if (!bolt) {
    return;
  }
  bolt->has_flash = true;
  bolt->flash_intensity = intensity;
  if (duration > bolt->lifetime) {
    bolt->lifetime = duration;
  }
  bolt->age = 0.0f;
}

float lightning_thunder_delay(float distance) {
  if (distance <= 0.0f) {
    return 0.0f;
  }
  return distance / 343.0f;
}

float lightning_damage(float distance, float base_damage) {
  if (base_damage <= 0.0f) {
    return 0.0f;
  }
  if (distance <= 0.1f) {
    return base_damage;
  }
  float falloff = 1.0f + distance * 0.1f;
  return base_damage / falloff;
}

void weather_manager_init(WeatherManager *manager) {
  if (!manager) {
    return;
  }

  memset(manager, 0, sizeof(*manager));
  for (uint32_t i = 0; i < WEATHER_TYPE_COUNT; ++i) {
    weather_params_set_defaults(&manager->weather_presets[i],
                                (WeatherType)i);
  }

  manager->current_weather = WEATHER_CLEAR;
  manager->transition.from_weather = WEATHER_CLEAR;
  manager->transition.to_weather = WEATHER_CLEAR;
  manager->transition.duration = 0.0f;
  manager->transition.progress = 0.0f;
  manager->transition.is_active = false;
  manager->transition.current_params =
      manager->weather_presets[WEATHER_CLEAR];

  manager->clouds.enabled = true;
  manager->clouds.coverage = manager->transition.current_params.cloud_coverage;
  manager->clouds.cloud_type = 0.4f;
  manager->clouds.density = 0.6f;
  manager->clouds.altitude_min =
      manager->transition.current_params.cloud_height - 200.0f;
  manager->clouds.altitude_max =
      manager->transition.current_params.cloud_height + 200.0f;
  manager->clouds.detail_scale = 1.0f;
  manager->clouds.weather_scale = 1.0f;
  manager->clouds.wind_speed = manager->transition.current_params.wind_speed;
  manager->clouds.wind_direction[0] =
      manager->transition.current_params.wind_direction[0];
  manager->clouds.wind_direction[1] =
      manager->transition.current_params.wind_direction[2];
  manager->clouds.light_absorption = 0.2f;
  manager->clouds.ambient_occlusion = 0.2f;
  manager->clouds.shadow_strength = 0.5f;
  manager->clouds.ray_march_steps = 64;
  manager->clouds.light_march_steps = 8;

  precipitation_init(&manager->precipitation, manager->current_weather);
  manager->precipitation.intensity =
      manager->transition.current_params.precipitation_intensity;
  manager->precipitation.active =
      manager->transition.current_params.has_precipitation;

  manager->lightning.max_bolts = 8u;
  manager->lightning.bolt_count = 0u;
  manager->lightning.bolts =
      (LightningBolt *)calloc(manager->lightning.max_bolts,
                              sizeof(LightningBolt));
  manager->lightning.strike_interval_min = 4.0f;
  manager->lightning.strike_interval_max = 12.0f;
  manager->lightning.bolt_lifetime = 0.5f;
  manager->lightning.flash_duration = 0.1f;
  manager->lightning.trigger_thunder = true;
  manager->lightning.next_strike_time = manager->lightning.strike_interval_min;

  manager->auto_weather_enabled = true;
  manager->weather_change_interval = 600.0f;
  manager->next_weather_change = manager->weather_change_interval;
  for (uint32_t i = 0; i < WEATHER_TYPE_COUNT; ++i) {
    manager->weather_probabilities[i] = 1.0f;
  }

  manager->seasonal_weather = false;
  manager->current_season = 0u;
}

void weather_manager_shutdown(WeatherManager *manager) {
  if (!manager) {
    return;
  }

  UNIFIED_FREE(manager->lightning.bolts);
  manager->lightning.bolts = NULL;
  manager->lightning.bolt_count = 0u;
  manager->lightning.max_bolts = 0u;

  if (manager->zones) {
    UNIFIED_FREE(manager->zones);
    manager->zones = NULL;
    manager->zone_count = 0u;
  }
}

void weather_manager_update(WeatherManager *manager, float delta_time) {
  if (!manager) {
    return;
  }

  if (manager->auto_weather_enabled && !manager->transition.is_active) {
    manager->next_weather_change -= delta_time;
    if (manager->next_weather_change <= 0.0f) {
      WeatherType next_type = weather_manager_random_weather(manager);
      weather_manager_transition_to(manager, next_type, 20.0f);
      manager->next_weather_change = manager->weather_change_interval;
    }
  }

  WeatherParameters current_params = manager->transition.current_params;
  if (manager->transition.is_active) {
    float duration = manager->transition.duration > 0.0f
                         ? manager->transition.duration
                         : 0.01f;
    manager->transition.progress += delta_time / duration;
    if (manager->transition.progress >= 1.0f) {
      manager->transition.progress = 1.0f;
      manager->transition.is_active = false;
      manager->current_weather = manager->transition.to_weather;
    }
    weather_params_lerp_local(
        &current_params,
        &manager->weather_presets[manager->transition.from_weather],
        &manager->weather_presets[manager->transition.to_weather],
        manager->transition.progress);
  } else {
    current_params = manager->weather_presets[manager->current_weather];
  }

  manager->transition.current_params = current_params;
  manager->clouds.coverage = current_params.cloud_coverage;
  manager->clouds.wind_speed = current_params.wind_speed;
  manager->clouds.wind_direction[0] = current_params.wind_direction[0];
  manager->clouds.wind_direction[1] = current_params.wind_direction[2];
  manager->clouds.altitude_min = current_params.cloud_height - 200.0f;
  manager->clouds.altitude_max = current_params.cloud_height + 200.0f;

  manager->precipitation.type = current_params.type;
  manager->precipitation.intensity = current_params.precipitation_intensity;
  manager->precipitation.active = current_params.has_precipitation;
  precipitation_update(&manager->precipitation, delta_time);

  if (manager->current_weather == WEATHER_THUNDERSTORM) {
    manager->lightning.next_strike_time -= delta_time;
    if (manager->lightning.next_strike_time <= 0.0f) {
      uint32_t rng = lcg_next(&weather_rng_state);
      float range = 60.0f;
      float start[3] = {0.0f, manager->clouds.altitude_max, 0.0f};
      start[0] += (lcg_float(&rng) - 0.5f) * range;
      start[2] += (lcg_float(&rng) - 0.5f) * range;
      float end[3] = {start[0] + (lcg_float(&rng) - 0.5f) * 8.0f, 0.0f,
                      start[2] + (lcg_float(&rng) - 0.5f) * 8.0f};
      lightning_generate_bolt(&manager->lightning, start, end);

      float interval =
          manager->lightning.strike_interval_min +
          lcg_float(&rng) *
              (manager->lightning.strike_interval_max -
               manager->lightning.strike_interval_min);
      manager->lightning.next_strike_time = interval;
    }
  }

  for (uint32_t i = 0; i < manager->lightning.bolt_count;) {
    LightningBolt *bolt = &manager->lightning.bolts[i];
    bolt->age += delta_time;
    if (bolt->age >= bolt->lifetime) {
      manager->lightning.bolts[i] =
          manager->lightning.bolts[manager->lightning.bolt_count - 1u];
      manager->lightning.bolt_count--;
      continue;
    }
    ++i;
  }
}

void weather_manager_set_weather(WeatherManager *manager, WeatherType type) {
  if (!manager) {
    return;
  }

  manager->current_weather = type;
  manager->transition.is_active = false;
  manager->transition.progress = 0.0f;
  manager->transition.from_weather = type;
  manager->transition.to_weather = type;
  manager->transition.current_params = manager->weather_presets[type];

  manager->clouds.coverage = manager->transition.current_params.cloud_coverage;
  manager->clouds.wind_speed = manager->transition.current_params.wind_speed;
  manager->clouds.wind_direction[0] =
      manager->transition.current_params.wind_direction[0];
  manager->clouds.wind_direction[1] =
      manager->transition.current_params.wind_direction[2];
  manager->clouds.altitude_min =
      manager->transition.current_params.cloud_height - 200.0f;
  manager->clouds.altitude_max =
      manager->transition.current_params.cloud_height + 200.0f;

  manager->precipitation.type = type;
  manager->precipitation.intensity =
      manager->transition.current_params.precipitation_intensity;
  manager->precipitation.active =
      manager->transition.current_params.has_precipitation;
}

void weather_manager_transition_to(WeatherManager *manager, WeatherType type,
                                   float duration) {
  if (!manager) {
    return;
  }

  manager->transition.from_weather = manager->current_weather;
  manager->transition.to_weather = type;
  manager->transition.duration = duration;
  manager->transition.progress = 0.0f;
  manager->transition.is_active = true;
  manager->transition.current_params =
      manager->weather_presets[manager->current_weather];
}

WeatherType weather_manager_random_weather(WeatherManager *manager) {
  if (!manager) {
    return WEATHER_CLEAR;
  }

  float total = 0.0f;
  for (uint32_t i = 0; i < WEATHER_TYPE_COUNT; ++i) {
    float weight = manager->weather_probabilities[i];
    if (weight < 0.0f) {
      weight = 0.0f;
    }
    total += weight;
  }

  if (total <= 0.0f) {
    return (WeatherType)(lcg_next(&weather_rng_state) % WEATHER_TYPE_COUNT);
  }

  float roll = lcg_float(&weather_rng_state) * total;
  float cumulative = 0.0f;
  for (uint32_t i = 0; i < WEATHER_TYPE_COUNT; ++i) {
    float weight = manager->weather_probabilities[i];
    if (weight < 0.0f) {
      weight = 0.0f;
    }
    cumulative += weight;
    if (roll <= cumulative) {
      return (WeatherType)i;
    }
  }

  return WEATHER_CLEAR;
}

static bool weather_position_in_zone(const WeatherZone *zone,
                                     const float position[3],
                                     float *out_edge_distance) {
  if (!zone || !position) {
    return false;
  }

  if (position[0] < zone->bounds_min[0] ||
      position[1] < zone->bounds_min[1] ||
      position[2] < zone->bounds_min[2] ||
      position[0] > zone->bounds_max[0] ||
      position[1] > zone->bounds_max[1] ||
      position[2] > zone->bounds_max[2]) {
    return false;
  }

  float dx = fminf(position[0] - zone->bounds_min[0],
                   zone->bounds_max[0] - position[0]);
  float dy = fminf(position[1] - zone->bounds_min[1],
                   zone->bounds_max[1] - position[1]);
  float dz = fminf(position[2] - zone->bounds_min[2],
                   zone->bounds_max[2] - position[2]);
  if (out_edge_distance) {
    *out_edge_distance = fminf(dx, fminf(dy, dz));
  }
  return true;
}

bool weather_manager_get_at_position(const WeatherManager *manager,
                                     const float position[3],
                                     WeatherParameters *out_params) {
  if (!manager || !position || !out_params) {
    return false;
  }

  WeatherParameters base_params = manager->transition.current_params;
  for (uint32_t i = 0; i < manager->zone_count; ++i) {
    float edge_distance = 0.0f;
    const WeatherZone *zone = &manager->zones[i];
    if (!weather_position_in_zone(zone, position, &edge_distance)) {
      continue;
    }

    WeatherParameters zone_params =
        manager->weather_presets[zone->forced_weather];
    if (zone->override_global || zone->blend_distance <= 0.0f) {
      *out_params = zone_params;
      return true;
    }

    float blend =
        1.0f - clamp01(edge_distance / zone->blend_distance);
    weather_params_lerp_local(out_params, &base_params, &zone_params, blend);
    return true;
  }

  *out_params = base_params;
  return true;
}
