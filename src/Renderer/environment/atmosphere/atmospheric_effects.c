// Atmospheric Effects Implementation
// Integrated volumetric fog, atmospheric scattering, and weather systems

#include "atmospheric_effects.h"
#include "engine/include/math/math_all.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Physical constants
#define EARTH_RADIUS_KM 6360.0f
#define ATMOSPHERE_HEIGHT_KM 100.0f
#define SEA_LEVEL_PRESSURE 101325.0f
#define GAS_CONSTANT 287.05f

// Time constants
#define HOURS_PER_DAY 24.0f
#define DAYS_PER_MONTH 30.0f
#define MONTHS_PER_YEAR 12.0f

// Weather transition constants
#define DEFAULT_WEATHER_TRANSITION_TIME 30.0f


// Helper functions
static Vec3 calculate_sun_position(f32 time_hours, Season season);
static Vec3 calculate_moon_position(f32 time_hours);
static void update_weather_transition(AtmosphericEffectsSystem *system,
                                      f32 delta_time);
static void apply_weather_parameters(AtmosphericEffectsSystem *system,
                                     WeatherCondition weather,
                                     f32 blend_factor);
static void update_cloud_physics(AtmosphericEffectsSystem *system,
                                 f32 delta_time);
static void update_precipitation_physics(AtmosphericEffectsSystem *system,
                                         f32 delta_time);

AtmosphericEffectsSystem *atmospheric_effects_create(void *device) {
  AtmosphericEffectsSystem *system =
      calloc(1, sizeof(AtmosphericEffectsSystem));
  if (!system) {
    LOG_ERROR("Failed to allocate atmospheric effects system");
    return NULL;
  }

  // Initialize core systems
  system->volumetric_fog = volumetric_fog_create(device, 160, 90, 128);
  if (!system->volumetric_fog) {
    LOG_ERROR("Failed to create volumetric fog system");
    free(system);
    return NULL;
  }

  system->atmosphere_scattering = calloc(1, sizeof(atmosphere_system_t));
  if (!system->atmosphere_scattering) {
    LOG_ERROR("Failed to allocate atmosphere scattering system");
    volumetric_fog_destroy(system->volumetric_fog);
    free(system);
    return NULL;
  }

  // Initialize atmospheric parameters with Earth defaults
  system->params.sun_intensity = 1.0f;
  system->params.moon_intensity = 0.1f;
  system->params.sun_color = (Vec3){1.0f, 0.95f, 0.8f};
  system->params.moon_color = (Vec3){0.7f, 0.8f, 1.0f};
  system->params.rayleigh_scattering = 5.8e-6f;
  system->params.mie_scattering = 2.0e-5f;
  system->params.atmosphere_height = ATMOSPHERE_HEIGHT_KM;

  // Initialize time and weather
  system->time_of_day_hours = 12.0f; // Noon
  system->current_weather = WEATHER_CLEAR;
  system->target_weather = WEATHER_CLEAR;
  system->current_time = TIME_NOON;
  system->current_season = SEASON_SUMMER;

  // Initialize sub-systems
  system->clouds.max_count = 128;
  system->clouds.positions = calloc(system->clouds.max_count, sizeof(Vec3));
  system->clouds.sizes = calloc(system->clouds.max_count, sizeof(Vec3));
  system->clouds.densities = calloc(system->clouds.max_count, sizeof(f32));

  system->precipitation.max_count = 2048;
  system->precipitation.particles =
      calloc(system->precipitation.max_count, sizeof(Vec3));
  system->precipitation.velocities =
      calloc(system->precipitation.max_count, sizeof(Vec3));
  system->precipitation.lifetimes =
      calloc(system->precipitation.max_count, sizeof(f32));
  system->precipitation.sizes =
      calloc(system->precipitation.max_count, sizeof(f32));

  // Set default quality
  system->high_quality_mode = true;
  system->params.volumetric_samples = 64;
  system->params.scattering_samples = 32;
  system->params.enable_aerial_perspective = true;
  system->params.enable_multiple_scattering = true;

  // Calculate initial celestial positions
  system->params.sun_direction =
      calculate_sun_position(system->time_of_day_hours, system->current_season);
  system->params.moon_direction =
      calculate_moon_position(system->time_of_day_hours);

  // Update parameters based on initial state
  atmospheric_effects_update_parameters(system);

  LOG_INFO("Atmospheric effects system created successfully");
  return system;
}

void atmospheric_effects_destroy(AtmosphericEffectsSystem *system) {
  if (!system)
    return;

  if (system->volumetric_fog) {
    volumetric_fog_destroy(system->volumetric_fog);
  }

  if (system->atmosphere_scattering) {
    free(system->atmosphere_scattering);
  }

  free(system->clouds.positions);
  free(system->clouds.sizes);
  free(system->clouds.densities);

  free(system->precipitation.particles);
  free(system->precipitation.velocities);
  free(system->precipitation.lifetimes);
  free(system->precipitation.sizes);

  free(system);
  LOG_INFO("Atmospheric effects system destroyed");
}

void atmospheric_effects_set_time_of_day(AtmosphericEffectsSystem *system,
                                         f32 hours) {
  if (!system)
    return;

  // Wrap hours to 0-24 range
  system->time_of_day_hours = fmodf(hours, HOURS_PER_DAY);
  if (system->time_of_day_hours < 0.0f) {
    system->time_of_day_hours += HOURS_PER_DAY;
  }

  // Update time of day category
  if (system->time_of_day_hours >= 5.0f && system->time_of_day_hours < 7.0f) {
    system->current_time = TIME_DAWN;
  } else if (system->time_of_day_hours >= 7.0f &&
             system->time_of_day_hours < 10.0f) {
    system->current_time = TIME_MORNING;
  } else if (system->time_of_day_hours >= 10.0f &&
             system->time_of_day_hours < 14.0f) {
    system->current_time = TIME_NOON;
  } else if (system->time_of_day_hours >= 14.0f &&
             system->time_of_day_hours < 17.0f) {
    system->current_time = TIME_AFTERNOON;
  } else if (system->time_of_day_hours >= 17.0f &&
             system->time_of_day_hours < 19.0f) {
    system->current_time = TIME_DUSK;
  } else if (system->time_of_day_hours >= 19.0f &&
             system->time_of_day_hours < 22.0f) {
    system->current_time = TIME_EVENING;
  } else if (system->time_of_day_hours >= 22.0f ||
             system->time_of_day_hours < 2.0f) {
    system->current_time = TIME_NIGHT;
  } else {
    system->current_time = TIME_MIDNIGHT;
  }

  // Update celestial positions
  system->params.sun_direction =
      calculate_sun_position(system->time_of_day_hours, system->current_season);
  system->params.moon_direction =
      calculate_moon_position(system->time_of_day_hours);

  // Update lighting based on sun position
  f32 sun_elevation = system->params.sun_direction.y;
  if (sun_elevation > 0.0f) {
    // Daytime
    system->params.sun_intensity = powf(sun_elevation, 0.5f);
    system->params.moon_intensity = 0.0f;

    // Sky color changes with sun angle
    f32 t = sun_elevation;
    system->params.zenith_color =
        vec3_lerp((Vec3){0.1f, 0.1f, 0.2f}, (Vec3){0.5f, 0.7f, 1.0f}, t);
    system->params.horizon_color =
        vec3_lerp((Vec3){0.2f, 0.1f, 0.1f}, (Vec3){1.0f, 0.8f, 0.6f}, t);
  } else {
    // Nighttime
    system->params.sun_intensity = 0.0f;
    system->params.moon_intensity = 0.1f;

    system->params.zenith_color = (Vec3){0.05f, 0.05f, 0.1f};
    system->params.horizon_color = (Vec3){0.1f, 0.1f, 0.15f};
  }

  atmospheric_effects_update_parameters(system);
}

void atmospheric_effects_advance_time(AtmosphericEffectsSystem *system,
                                      f32 delta_hours) {
  if (!system)
    return;
  atmospheric_effects_set_time_of_day(system,
                                      system->time_of_day_hours + delta_hours);
}

void atmospheric_effects_set_weather(AtmosphericEffectsSystem *system,
                                     WeatherCondition weather,
                                     f32 transition_time) {
  if (!system)
    return;

  system->target_weather = weather;
  system->weather_transition_duration = transition_time > 0.0f
                                            ? transition_time
                                            : DEFAULT_WEATHER_TRANSITION_TIME;
  system->weather_transition_time = 0.0f;
}

void atmospheric_effects_set_weather_immediate(AtmosphericEffectsSystem *system,
                                               WeatherCondition weather) {
  if (!system)
    return;

  system->current_weather = weather;
  system->target_weather = weather;
  system->weather_transition_time = 0.0f;
  apply_weather_parameters(system, weather, 1.0f);
}

void atmospheric_effects_update(AtmosphericEffectsSystem *system,
                                f32 delta_time) {
  if (!system)
    return;

  // Update weather transitions
  update_weather_transition(system, delta_time);

  // Update sub-systems
  atmospheric_effects_update_clouds(system, delta_time);
  atmospheric_effects_update_precipitation(system, delta_time);
  atmospheric_effects_update_lightning(system, delta_time);

  // Update performance metrics
  system->frame_counter++;
  if (system->frame_counter % 60 == 0) { // Update every 60 frames
    system->average_frame_time = delta_time;
  }
}

static void update_weather_transition(AtmosphericEffectsSystem *system,
                                      f32 delta_time) {
  if (system->current_weather == system->target_weather)
    return;

  system->weather_transition_time += delta_time;
  f32 blend_factor =
      system->weather_transition_time / system->weather_transition_duration;

  if (blend_factor >= 1.0f) {
    blend_factor = 1.0f;
    system->current_weather = system->target_weather;
  }

  apply_weather_parameters(system, system->target_weather, blend_factor);
}

static void apply_weather_parameters(AtmosphericEffectsSystem *system,
                                     WeatherCondition weather,
                                     f32 blend_factor) {
  AtmosphericParameters *params = &system->params;

  // Base parameters for different weather conditions
  Vec3 target_fog_color, target_horizon_color, target_zenith_color;
  f32 target_fog_density, target_cloud_coverage, target_precipitation;
  f32 target_wind_strength, target_visibility;
  Vec3 target_wind_direction;

  switch (weather) {
  case WEATHER_CLEAR:
    target_fog_density = 0.001f;
    target_cloud_coverage = 0.1f;
    target_precipitation = 0.0f;
    target_wind_strength = 0.5f;
    target_fog_color = (Vec3){0.8f, 0.9f, 1.0f};
    target_wind_direction = (Vec3){1.0f, 0.0f, 0.0f};
    break;

  case WEATHER_PARTLY_CLOUDY:
    target_fog_density = 0.002f;
    target_cloud_coverage = 0.4f;
    target_precipitation = 0.0f;
    target_wind_strength = 1.0f;
    target_fog_color = (Vec3){0.7f, 0.8f, 0.9f};
    target_wind_direction = (Vec3){0.7f, 0.0f, 0.7f};
    break;

  case WEATHER_OVERCAST:
    target_fog_density = 0.01f;
    target_cloud_coverage = 0.9f;
    target_precipitation = 0.0f;
    target_wind_strength = 2.0f;
    target_fog_color = (Vec3){0.6f, 0.6f, 0.7f};
    target_wind_direction = (Vec3){0.5f, 0.0f, 0.5f};
    break;

  case WEATHER_LIGHT_RAIN:
    target_fog_density = 0.02f;
    target_cloud_coverage = 0.8f;
    target_precipitation = 0.3f;
    target_wind_strength = 3.0f;
    target_fog_color = (Vec3){0.5f, 0.5f, 0.6f};
    target_wind_direction = (Vec3){0.3f, 0.0f, 0.7f};
    break;

  case WEATHER_HEAVY_RAIN:
    target_fog_density = 0.05f;
    target_cloud_coverage = 1.0f;
    target_precipitation = 0.8f;
    target_wind_strength = 5.0f;
    target_fog_color = (Vec3){0.4f, 0.4f, 0.5f};
    target_wind_direction = (Vec3){0.2f, 0.0f, 0.8f};
    break;

  case WEATHER_THUNDERSTORM:
    target_fog_density = 0.08f;
    target_cloud_coverage = 1.0f;
    target_precipitation = 1.0f;
    target_wind_strength = 8.0f;
    target_fog_color = (Vec3){0.3f, 0.3f, 0.4f};
    target_wind_direction = (Vec3){0.1f, 0.0f, 0.9f};
    break;

  case WEATHER_LIGHT_SNOW:
    target_fog_density = 0.015f;
    target_cloud_coverage = 0.7f;
    target_precipitation = 0.2f;
    target_wind_strength = 1.5f;
    target_fog_color = (Vec3){0.9f, 0.9f, 1.0f};
    target_wind_direction = (Vec3){0.6f, 0.0f, 0.4f};
    break;

  case WEATHER_HEAVY_SNOW:
    target_fog_density = 0.04f;
    target_cloud_coverage = 0.95f;
    target_precipitation = 0.7f;
    target_wind_strength = 3.0f;
    target_fog_color = (Vec3){0.85f, 0.85f, 0.95f};
    target_wind_direction = (Vec3){0.4f, 0.0f, 0.6f};
    break;

  case WEATHER_BLIZZARD:
    target_fog_density = 0.15f;
    target_cloud_coverage = 1.0f;
    target_precipitation = 1.0f;
    target_wind_strength = 12.0f;
    target_fog_color = (Vec3){0.8f, 0.8f, 0.9f};
    target_wind_direction = (Vec3){0.1f, 0.0f, 0.95f};
    break;

  case WEATHER_FOG:
    target_fog_density = 0.2f;
    target_cloud_coverage = 0.3f;
    target_precipitation = 0.0f;
    target_wind_strength = 0.2f;
    target_fog_color = (Vec3){0.7f, 0.7f, 0.8f};
    target_wind_direction = (Vec3){0.0f, 0.0f, 0.1f};
    break;

  default:
    target_fog_density = 0.001f;
    target_cloud_coverage = 0.1f;
    target_precipitation = 0.0f;
    target_wind_strength = 0.5f;
    target_fog_color = (Vec3){0.8f, 0.9f, 1.0f};
    target_wind_direction = (Vec3){1.0f, 0.0f, 0.0f};
    break;
  }

  // Blend parameters
  params->fog_density =
      params->fog_density +
      (target_fog_density - params->fog_density) * blend_factor;
  params->fog_color =
      vec3_lerp(params->fog_color, target_fog_color, blend_factor);
  params->cloud_coverage =
      lerp(params->cloud_coverage, target_cloud_coverage, blend_factor);
  params->precipitation_intensity =
      lerp(params->precipitation_intensity, target_precipitation, blend_factor);
  params->wind_strength =
      lerp(params->wind_strength, target_wind_strength, blend_factor);
  params->wind_direction =
      vec3_lerp(params->wind_direction, target_wind_direction, blend_factor);

  // Update visibility based on fog density
  target_visibility = 1.0f / (params->fog_density + 0.001f);
  target_visibility =
      fminf(target_visibility, 1000.0f); // Max visibility of 1km
}

void atmospheric_effects_update_parameters(AtmosphericEffectsSystem *system) {
  if (!system)
    return;

  // Update volumetric fog parameters
  if (system->volumetric_fog) {
    system->volumetric_fog->density_scale = system->params.fog_density;
    system->volumetric_fog->fog_color =
        (simd_float3){system->params.fog_color.x, system->params.fog_color.y,
                      system->params.fog_color.z};
  }

  // Update atmosphere scattering parameters
  if (system->atmosphere_scattering) {
    system->atmosphere_scattering->rayleigh_coeff = (simd_float3){
        system->params.rayleigh_scattering,
        system->params.rayleigh_scattering * 0.8f, // Green slightly less
        system->params.rayleigh_scattering * 1.2f  // Blue slightly more
    };
    system->atmosphere_scattering->mie_coeff = system->params.mie_scattering;
    system->atmosphere_scattering->sun_direction = (simd_float3){
        system->params.sun_direction.x, system->params.sun_direction.y,
        system->params.sun_direction.z};
    system->atmosphere_scattering->sun_intensity = (simd_float3){
        system->params.sun_intensity * system->params.sun_color.x,
        system->params.sun_intensity * system->params.sun_color.y,
        system->params.sun_intensity * system->params.sun_color.z};
  }
}

void atmospheric_effects_update_clouds(AtmosphericEffectsSystem *system,
                                       f32 delta_time) {
  if (!system || system->params.cloud_coverage <= 0.0f)
    return;

  update_cloud_physics(system, delta_time);

  // Generate new clouds if needed
  u32 target_cloud_count =
      (u32)(system->clouds.max_count * system->params.cloud_coverage);
  while (system->clouds.count < target_cloud_count) {
    u32 idx = system->clouds.count++;

    // Random position in upper atmosphere
    system->clouds.positions[idx] =
        (Vec3){(rand() / (f32)RAND_MAX - 0.5f) * 2000.0f,
               500.0f + (rand() / (f32)RAND_MAX) * 300.0f,
               (rand() / (f32)RAND_MAX - 0.5f) * 2000.0f};

    // Random size
    system->clouds.sizes[idx] =
        (Vec3){100.0f + (rand() / (f32)RAND_MAX) * 200.0f,
               50.0f + (rand() / (f32)RAND_MAX) * 100.0f,
               100.0f + (rand() / (f32)RAND_MAX) * 200.0f};

    // Random density
    system->clouds.densities[idx] = 0.3f + (rand() / (f32)RAND_MAX) * 0.7f;
  }
}

static void update_cloud_physics(AtmosphericEffectsSystem *system,
                                 f32 delta_time) {
  // Move clouds with wind
  for (u32 i = 0; i < system->clouds.count; i++) {
    Vec3 wind_velocity =
        vec3_mul(system->params.wind_direction, system->params.wind_strength);
    system->clouds.positions[i] = vec3_add(system->clouds.positions[i],
                                           vec3_mul(wind_velocity, delta_time));

    // Wrap clouds around world boundaries
    if (system->clouds.positions[i].x > 1500.0f)
      system->clouds.positions[i].x = -1500.0f;
    if (system->clouds.positions[i].x < -1500.0f)
      system->clouds.positions[i].x = 1500.0f;
    if (system->clouds.positions[i].z > 1500.0f)
      system->clouds.positions[i].z = -1500.0f;
    if (system->clouds.positions[i].z < -1500.0f)
      system->clouds.positions[i].z = 1500.0f;

    // Slowly change cloud density
    f32 density_change = ((rand() / (f32)RAND_MAX) - 0.5f) * 0.1f * delta_time;
    system->clouds.densities[i] += density_change;
    system->clouds.densities[i] =
        clamp(system->clouds.densities[i], 0.1f, 1.0f);
  }
}

void atmospheric_effects_update_precipitation(AtmosphericEffectsSystem *system,
                                              f32 delta_time) {
  if (!system || system->params.precipitation_intensity <= 0.0f)
    return;

  update_precipitation_physics(system, delta_time);

  // Generate new precipitation particles
  u32 particles_to_spawn =
      (u32)(system->params.precipitation_intensity * 100.0f * delta_time);
  particles_to_spawn = MIN(particles_to_spawn, system->precipitation.max_count -
                                                   system->precipitation.count);

  for (u32 i = 0; i < particles_to_spawn; i++) {
    if (system->precipitation.count >= system->precipitation.max_count)
      break;

    u32 idx = system->precipitation.count++;

    // Random position above camera
    system->precipitation.particles[idx] =
        (Vec3){(rand() / (f32)RAND_MAX - 0.5f) * 1000.0f,
               200.0f + (rand() / (f32)RAND_MAX) * 100.0f,
               (rand() / (f32)RAND_MAX - 0.5f) * 1000.0f};

    // Fall velocity with wind effect
    Vec3 wind_effect = vec3_mul(system->params.wind_direction,
                                system->params.wind_strength * 0.3f);
    Vec3 fall_velocity = (Vec3){0.0f, -system->precipitation.fall_speed, 0.0f};
    system->precipitation.velocities[idx] =
        vec3_add(fall_velocity, wind_effect);

    system->precipitation.lifetimes[idx] = 0.0f;
    system->precipitation.sizes[idx] = system->precipitation.particle_size;
  }
}

static void update_precipitation_physics(AtmosphericEffectsSystem *system,
                                         f32 delta_time) {
  for (u32 i = 0; i < system->precipitation.count; i++) {
    // Update particle position
    system->precipitation.particles[i] =
        vec3_add(system->precipitation.particles[i],
                 vec3_mul(system->precipitation.velocities[i], delta_time));

    // Update lifetime
    system->precipitation.lifetimes[i] += delta_time;

    // Remove particles that hit the ground or expired
    if (system->precipitation.particles[i].y < 0.0f ||
        system->precipitation.lifetimes[i] > 10.0f) {

      // Create splash effect if hitting ground
      if (system->precipitation.particles[i].y < 0.0f) {
        atmospheric_effects_create_splash_effects(
            system, system->precipitation.particles[i]);
      }

      // Replace with last particle
      if (i < system->precipitation.count - 1) {
        system->precipitation.particles[i] =
            system->precipitation.particles[system->precipitation.count - 1];
        system->precipitation.velocities[i] =
            system->precipitation.velocities[system->precipitation.count - 1];
        system->precipitation.lifetimes[i] =
            system->precipitation.lifetimes[system->precipitation.count - 1];
        system->precipitation.sizes[i] =
            system->precipitation.sizes[system->precipitation.count - 1];
        i--; // Re-check this index
      }
      system->precipitation.count--;
    }
  }
}

void atmospheric_effects_create_splash_effects(AtmosphericEffectsSystem *system,
                                               Vec3 position) {
  // This would create splash particles and audio effects
  // For now, just log the splash position
  LOG_DEBUG("Splash effect at position: %.2f, %.2f, %.2f", position.x,
            position.y, position.z);
}

void atmospheric_effects_update_lightning(AtmosphericEffectsSystem *system,
                                          f32 delta_time) {
  if (!system || system->current_weather != WEATHER_THUNDERSTORM)
    return;

  system->lightning.next_strike_time -= delta_time;

  if (system->lightning.next_strike_time <= 0.0f) {
    // Trigger lightning strike
    Vec3 strike_pos = (Vec3){(rand() / (f32)RAND_MAX - 0.5f) * 2000.0f,
                             100.0f + (rand() / (f32)RAND_MAX) * 400.0f,
                             (rand() / (f32)RAND_MAX - 0.5f) * 2000.0f};

    atmospheric_effects_trigger_lightning_strike(system, strike_pos);

    // Schedule next strike
    system->lightning.next_strike_time = 2.0f + (rand() / (f32)RAND_MAX) * 8.0f;
  }

  // Update active strikes
  for (u32 i = 0; i < system->lightning.active_strikes; i++) {
    system->lightning.strike_times[i] -= delta_time;
    if (system->lightning.strike_times[i] <= 0.0f) {
      // Remove expired strike
      if (i < system->lightning.active_strikes - 1) {
        system->lightning.strike_positions[i] =
            system->lightning
                .strike_positions[system->lightning.active_strikes - 1];
        system->lightning.strike_times[i] =
            system->lightning
                .strike_times[system->lightning.active_strikes - 1];
        i--;
      }
      system->lightning.active_strikes--;
    }
  }
}

void atmospheric_effects_trigger_lightning_strike(
    AtmosphericEffectsSystem *system, Vec3 position) {
  if (!system || system->lightning.active_strikes >= 16)
    return;

  u32 idx = system->lightning.active_strikes++;
  system->lightning.strike_positions[idx] = position;
  system->lightning.strike_times[idx] = system->lightning.strike_duration;

  LOG_INFO("Lightning strike at: %.2f, %.2f, %.2f", position.x, position.y,
           position.z);
}

static Vec3 calculate_sun_position(f32 time_hours, Season season) {
  // Simplified sun position calculation
  f32 hour_angle =
      (time_hours - 12.0f) * 15.0f * (M_PI / 180.0f); // Convert to radians

  // Seasonal declination angle
  f32 declination = 0.0f;
  switch (season) {
  case SEASON_SPRING:
    declination = 0.0f;
    break;
  case SEASON_SUMMER:
    declination = 23.5f * (M_PI / 180.0f);
    break;
  case SEASON_AUTUMN:
    declination = 0.0f;
    break;
  case SEASON_WINTER:
    declination = -23.5f * (M_PI / 180.0f);
    break;
  }

  f32 latitude = 40.0f * (M_PI / 180.0f); // Assume 40° latitude

  // Calculate sun position
  f32 altitude = asinf(sinf(latitude) * sinf(declination) +
                       cosf(latitude) * cosf(declination) * cosf(hour_angle));

  f32 azimuth =
      atan2f(sinf(hour_angle), cosf(hour_angle) * sinf(latitude) -
                                   tanf(declination) * cosf(latitude));

  return (Vec3){cosf(altitude) * sinf(azimuth), sinf(altitude),
                cosf(altitude) * cosf(azimuth)};
}

static Vec3 calculate_moon_position(f32 time_hours) {
  // Simplified moon position (roughly opposite to sun)
  f32 moon_hour_angle = (time_hours + 12.0f) * 15.0f * (M_PI / 180.0f);

  return (Vec3){cosf(moon_hour_angle),
                sinf(moon_hour_angle) * 0.3f, // Moon follows different path
                sinf(moon_hour_angle)};
}

Vec3 atmospheric_effects_get_sun_color(const AtmosphericEffectsSystem *system) {
  if (!system)
    return (Vec3){1.0f, 1.0f, 1.0f};
  return vec3_mul(system->params.sun_color, system->params.sun_intensity);
}

Vec3 atmospheric_effects_get_ambient_color(
    const AtmosphericEffectsSystem *system) {
  if (!system)
    return (Vec3){0.2f, 0.2f, 0.3f};

  // Blend horizon and zenith colors
  Vec3 sky_color = vec3_lerp(system->params.horizon_color,
                             system->params.zenith_color, 0.5f);
  return vec3_mul(sky_color, system->params.sky_brightness * 0.3f);
}

f32 atmospheric_effects_get_visibility_distance(
    const AtmosphericEffectsSystem *system) {
  if (!system)
    return 1000.0f;
  return 1.0f / (system->params.fog_density + 0.001f);
}

const char *atmospheric_weather_to_string(WeatherCondition weather) {
  switch (weather) {
  case WEATHER_CLEAR:
    return "Clear";
  case WEATHER_PARTLY_CLOUDY:
    return "Partly Cloudy";
  case WEATHER_OVERCAST:
    return "Overcast";
  case WEATHER_LIGHT_RAIN:
    return "Light Rain";
  case WEATHER_HEAVY_RAIN:
    return "Heavy Rain";
  case WEATHER_THUNDERSTORM:
    return "Thunderstorm";
  case WEATHER_LIGHT_SNOW:
    return "Light Snow";
  case WEATHER_HEAVY_SNOW:
    return "Heavy Snow";
  case WEATHER_BLIZZARD:
    return "Blizzard";
  case WEATHER_FOG:
    return "Fog";
  case WEATHER_MIST:
    return "Mist";
  case WEATHER_HAZE:
    return "Haze";
  default:
    return "Unknown";
  }
}
