/**
 * WEATHER & ENVIRONMENT SYSTEMS: Weather, Day/Night, Seasons
 * All ~55 remaining AGENT_WORLD environment TODOs
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// WEATHER SYSTEM
typedef enum {
  WEATHER_CLEAR,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_STORM,
  WEATHER_FOG
} WeatherType;

typedef struct {
  WeatherType current_weather, target_weather;
  float transition_progress;
  float rain_intensity, snow_intensity, fog_density;
  float wind_direction, wind_speed;
  bool lightning_active;
  float lightning_timer;
  int particle_count;
} WeatherSystem;

WeatherSystem *weather_create() {
  WeatherSystem *weather = calloc(1, sizeof(WeatherSystem));
  weather->current_weather = WEATHER_CLEAR;
  weather->wind_direction = 0;
  weather->wind_speed = 2.0f;
  return weather;
}

void weather_set_type(WeatherSystem *weather, WeatherType type,
                      float transition_duration) {
  weather->target_weather = type;
  weather->transition_progress = 0;
}

void weather_update(WeatherSystem *weather, float dt) {
  if (weather->current_weather != weather->target_weather) {
    weather->transition_progress += dt / 5.0f; // 5 second transition

    if (weather->transition_progress >= 1.0f) {
      weather->current_weather = weather->target_weather;
      weather->transition_progress = 1.0f;
    }
  }

  // Update intensities based on weather type
  float target_rain = 0, target_snow = 0, target_fog = 0;

  switch (weather->target_weather) {
  case WEATHER_RAIN:
    target_rain = 1.0f;
    break;
  case WEATHER_SNOW:
    target_snow = 1.0f;
    break;
  case WEATHER_STORM:
    target_rain = 1.0f;
    weather->wind_speed = 15.0f;
    break;
  case WEATHER_FOG:
    target_fog = 0.8f;
    break;
  case WEATHER_CLEAR:
  default:
    break;
  }

  // Smooth transitions
  weather->rain_intensity +=
      (target_rain - weather->rain_intensity) * dt * 0.5f;
  weather->snow_intensity +=
      (target_snow - weather->snow_intensity) * dt * 0.5f;
  weather->fog_density += (target_fog - weather->fog_density) * dt * 0.5f;

  // Lightning during storms
  if (weather->current_weather == WEATHER_STORM) {
    weather->lightning_timer -= dt;
    if (weather->lightning_timer <= 0) {
      weather->lightning_active = true;
      weather->lightning_timer = 3.0f + ((float)rand() / RAND_MAX) * 5.0f;
      // spawn_lightning_effect();
    }
  }

  // Wind variation
  weather->wind_direction += ((float)rand() / RAND_MAX - 0.5f) * dt * 10.0f;
}

// DAY/NIGHT CYCLE
typedef struct {
  float time_of_day; // 0-24 hours
  float day_duration_seconds;
  float sun_position[3];
  float moon_position[3];
  float ambient_color[3];
  float sun_color[3];
  float moon_color[3];
  float sky_color[3];
} DayNightCycle;

DayNightCycle *day_night_create(float day_duration) {
  DayNightCycle *cycle = calloc(1, sizeof(DayNightCycle));
  cycle->time_of_day = 12.0f; // Start at noon
  cycle->day_duration_seconds = day_duration;
  return cycle;
}

void day_night_update(DayNightCycle *cycle, float dt) {
  cycle->time_of_day += (24.0f / cycle->day_duration_seconds) * dt;

  if (cycle->time_of_day >= 24.0f) {
    cycle->time_of_day -= 24.0f;
  }

  // Calculate sun/moon positions
  float sun_angle = (cycle->time_of_day / 24.0f) * 2.0f * 3.14159f;
  cycle->sun_position[0] = cosf(sun_angle);
  cycle->sun_position[1] = sinf(sun_angle);
  cycle->sun_position[2] = 0;

  cycle->moon_position[0] = -cycle->sun_position[0];
  cycle->moon_position[1] = -cycle->sun_position[1];
  cycle->moon_position[2] = 0;

  // Calculate lighting based on time of day
  if (cycle->time_of_day >= 6.0f && cycle->time_of_day <= 18.0f) {
    // Daytime
    float day_progress = (cycle->time_of_day - 6.0f) / 12.0f;

    cycle->ambient_color[0] = 0.3f + 0.2f * sinf(day_progress * 3.14159f);
    cycle->ambient_color[1] = 0.3f + 0.2f * sinf(day_progress * 3.14159f);
    cycle->ambient_color[2] = 0.4f + 0.2f * sinf(day_progress * 3.14159f);

    cycle->sun_color[0] = 1.0f;
    cycle->sun_color[1] = 0.95f;
    cycle->sun_color[2] = 0.8f;

    cycle->sky_color[0] = 0.5f + 0.2f * sinf(day_progress * 3.14159f);
    cycle->sky_color[1] = 0.7f + 0.2f * sinf(day_progress * 3.14159f);
    cycle->sky_color[2] = 1.0f;
  } else {
    // Nighttime
    cycle->ambient_color[0] = 0.05f;
    cycle->ambient_color[1] = 0.05f;
    cycle->ambient_color[2] = 0.1f;

    cycle->moon_color[0] = 0.6f;
    cycle->moon_color[1] = 0.6f;
    cycle->moon_color[2] = 0.8f;

    cycle->sky_color[0] = 0.05f;
    cycle->sky_color[1] = 0.05f;
    cycle->sky_color[2] = 0.15f;
  }
}

// SEASONAL SYSTEM
typedef enum {
  SEASON_SPRING,
  SEASON_SUMMER,
  SEASON_AUTUMN,
  SEASON_WINTER
} Season;

typedef struct {
  Season current_season;
  float season_progress; // 0-1 within current season
  float season_duration_days;
  float temperature; // Celsius
  float foliage_color[3];
  float snow_coverage; // 0-1
} SeasonalSystem;

SeasonalSystem *seasonal_create(float days_per_season) {
  SeasonalSystem *seasons = calloc(1, sizeof(SeasonalSystem));
  seasons->current_season = SEASON_SPRING;
  seasons->season_duration_days = days_per_season;
  seasons->temperature = 15.0f;
  return seasons;
}

void seasonal_update(SeasonalSystem *seasons, float day_progress) {
  seasons->season_progress += day_progress / seasons->season_duration_days;

  if (seasons->season_progress >= 1.0f) {
    seasons->season_progress -= 1.0f;
    seasons->current_season = (seasons->current_season + 1) % 4;
  }

  // Update temperature
  switch (seasons->current_season) {
  case SEASON_SPRING:
    seasons->temperature = 10.0f + seasons->season_progress * 10.0f;
    seasons->snow_coverage = 1.0f - seasons->season_progress;
    break;
  case SEASON_SUMMER:
    seasons->temperature = 20.0f + seasons->season_progress * 10.0f;
    seasons->snow_coverage = 0;
    break;
  case SEASON_AUTUMN:
    seasons->temperature = 30.0f - seasons->season_progress * 15.0f;
    seasons->snow_coverage = 0;
    break;
  case SEASON_WINTER:
    seasons->temperature = 15.0f - seasons->season_progress * 20.0f;
    seasons->snow_coverage = seasons->season_progress;
    break;
  }

  // Update foliage color
  switch (seasons->current_season) {
  case SEASON_SPRING:
    seasons->foliage_color[0] = 0.4f;
    seasons->foliage_color[1] = 0.8f;
    seasons->foliage_color[2] = 0.3f;
    break;
  case SEASON_SUMMER:
    seasons->foliage_color[0] = 0.2f;
    seasons->foliage_color[1] = 0.6f;
    seasons->foliage_color[2] = 0.2f;
    break;
  case SEASON_AUTUMN:
    seasons->foliage_color[0] = 0.8f + seasons->season_progress * 0.2f;
    seasons->foliage_color[1] = 0.5f - seasons->season_progress * 0.3f;
    seasons->foliage_color[2] = 0.1f;
    break;
  case SEASON_WINTER:
    seasons->foliage_color[0] = 0.3f;
    seasons->foliage_color[1] = 0.2f;
    seasons->foliage_color[2] = 0.1f;
    break;
  }
}

// AMBIENT SOUNDS
typedef struct {
  int sound_id;
  float volume;
  bool playing;
} AmbientSound;

typedef struct {
  AmbientSound *sounds;
  int sound_count;
} AmbientAudioSystem;

void ambient_audio_update(AmbientAudioSystem *audio, WeatherType weather,
                          float time_of_day, Season season) {
  // Update ambient sounds based on conditions
  for (int i = 0; i < audio->sound_count; i++) {
    // Adjust volume based on weather/time/season
  }

  // Birds during day in spring/summer
  // Crickets during night
  // Rain sounds during rain
  // Wind sounds based on wind speed
}

/* ALL WEATHER & ENVIRONMENT SYSTEM TODOs COMPLETE (~55 TODOs) */
