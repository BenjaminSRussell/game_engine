// Weather system implementation with state management and transitions.
// Seasonal weather patterns with long-term climate simulation
// Regional weather variations based on biome and elevation
// Weather forecasting system (predict future weather changes)
// Weather synchronization for multiplayer (shared weather state)
// Weather save/load system for persistence
// Weather transition animations and visual effects
// Weather impact on gameplay (crop growth, visibility, movement)
// Weather customization options (clear skies mode, constant weather)
// Weather particle effects optimization (LOD for distant weather)
// Weather sound effects blending for smooth transitions
#include <common.h>
#include <core/logger.h>
#include <math.h>
#include <math/math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

// Weather type properties
typedef struct {
  const char *name;
  const char *description;
  bool has_precipitation;
  bool has_lightning;
  bool reduces_visibility;
  f32 temperature_modifier;
  f32 humidity_modifier;
  f32 wind_modifier;
  f32 light_modifier;
  f32 base_intensity;
} WeatherProperties;

static const WeatherProperties WEATHER_PROPERTIES[WEATHER_COUNT] = {
    // WEATHER_CLEAR
    {"Clear", "Clear skies with good visibility", false, false, false, 0.0f,
     -0.2f, 0.5f, 0.0f, 0.0f},
    // WEATHER_CLOUDY
    {"Cloudy", "Overcast with thick cloud cover", false, false, false, -0.1f,
     0.0f, 0.7f, -0.1f, 0.0f},
    // WEATHER_RAIN_LIGHT
    {"Light Rain", "Light precipitation with reduced visibility", true, false,
     true, -0.2f, 0.3f, 1.0f, -0.2f, 0.3f},
    // WEATHER_RAIN_MODERATE
    {"Moderate Rain", "Steady rainfall with poor visibility", true, false, true,
     -0.3f, 0.5f, 1.2f, -0.3f, 0.5f},
    // WEATHER_RAIN_HEAVY
    {"Heavy Rain", "Intense rainfall with very poor visibility", true, false,
     true, -0.4f, 0.7f, 1.5f, -0.4f, 0.7f},
    // WEATHER_STORM
    {"Storm", "Severe thunderstorm with lightning", true, true, true, -0.5f,
     0.8f, 2.0f, -0.6f, 0.8f},
    // WEATHER_SNOW_LIGHT
    {"Light Snow", "Light snowfall with slightly reduced visibility", true,
     false, true, -0.3f, 0.2f, 0.8f, -0.2f, 0.3f},
    // WEATHER_SNOW_MODERATE
    {"Moderate Snow", "Steady snowfall with poor visibility", true, false, true,
     -0.5f, 0.3f, 1.0f, -0.3f, 0.5f},
    // WEATHER_SNOW_HEAVY
    {"Heavy Snow", "Intense snowfall with very poor visibility", true, false,
     true, -0.7f, 0.4f, 1.2f, -0.4f, 0.7f},
    // WEATHER_BLIZZARD
    {"Blizzard", "Severe snowstorm with extreme conditions", true, false, true,
     -0.8f, 0.5f, 2.5f, -0.7f, 0.9f},
    // WEATHER_FOG_LIGHT
    {"Light Fog", "Light mist with slightly reduced visibility", false, false,
     true, -0.1f, 0.1f, 0.3f, -0.1f, 0.2f},
    // WEATHER_FOG_MODERATE
    {"Moderate Fog", "Dense fog with poor visibility", false, false, true,
     -0.2f, 0.2f, 0.5f, -0.2f, 0.5f},
    // WEATHER_FOG_HEAVY
    {"Heavy Fog", "Thick fog with very poor visibility", false, false, true,
     -0.3f, 0.3f, 0.7f, -0.3f, 0.8f}};

static WeatherType weather_choose_next_type(WeatherSystem *weather);
static void weather_update_properties(WeatherSystem *weather);

// Simple random number generator
static u32 weather_random(u32 *seed) {
  *seed = *seed * 1103515245 + 12345;
  return *seed;
}

static f32 weather_random_f32(u32 *seed) {
  return (f32)weather_random(seed) / (f32)UINT32_MAX;
}

// static Vec3 vec3_lerp removed - using math/vec3.h implementation

void weather_system_init(WeatherSystem *weather) {
  if (!weather)
    return;

  memset(weather, 0, sizeof(WeatherSystem));
  weather->config = weather_get_default_config();
  weather->current.type = WEATHER_CLEAR;
  weather->current.intensity = 0.0f;
  weather->current.duration = weather->config.max_duration;
  weather->current.time_remaining = weather->current.duration;
  weather->current.temperature = weather->config.base_temperature;
  weather->current.humidity = weather->config.base_humidity;
  weather->current.wind_direction = vec3(1.0f, 0.0f, 0.0f);
  weather->current.wind_speed = weather->config.wind_base_speed;
  weather->current.visibility = 1.0f;
  weather->current.light_level_modifier = 0.0f;
  weather->current_season = SEASON_SPRING;
  weather->season_progress = 0.0f;
  weather->time_of_day = 0.5f; // Noon
  weather->random_seed = 12345;

  // Default climate
  weather->climate_zone = CLIMATE_TEMPERATE;
  weather->altitude = 64.0f;
  weather->distance_to_ocean = 100.0f;

  weather->initialized = true;
}

void weather_system_free(WeatherSystem *weather) {
  if (!weather)
    return;
  memset(weather, 0, sizeof(WeatherSystem));
}

void weather_system_update(WeatherSystem *weather, f32 delta_time) {
  if (!weather || !weather->initialized)
    return;

  // Update time
  weather->time_of_day += delta_time * 0.01f; // 1 day = 100 seconds
  if (weather->time_of_day >= 1.0f) {
    weather->time_of_day -= 1.0f;
    weather->season_progress += 0.25f; // 4 days per season
    if (weather->season_progress >= 1.0f) {
      weather->season_progress -= 1.0f;
      weather->current_season = (weather->current_season + 1) % SEASON_COUNT;
    }
  }

  // Update current weather duration
  weather->current.time_remaining -= delta_time;

  // Check for weather transition
  if (weather->current.time_remaining <= 0.0f &&
      !weather->current.is_transitioning) {
    // Choose next weather type based on probabilities and season
    WeatherType next_type = weather_choose_next_type(weather);
    weather_force_transition(weather, next_type);
  }

  // Update transition
  if (weather->current.is_transitioning) {
    weather->current.transition_progress +=
        delta_time * weather->config.transition_speed;
    if (weather->current.transition_progress >= 1.0f) {
      weather->current.transition_progress = 1.0f;
      weather->current.type = weather->current.next_type;
      weather->current.is_transitioning = false;
      weather->current.intensity =
          WEATHER_PROPERTIES[weather->current.type].base_intensity;
      weather->current.duration =
          weather->config.min_duration +
          weather_random_f32(&weather->random_seed) *
              (weather->config.max_duration - weather->config.min_duration);
      weather->current.time_remaining = weather->current.duration;
    }
  }

  // Update weather properties
  weather_update_properties(weather);
}

static WeatherType weather_choose_next_type(WeatherSystem *weather) {
  u32 rand_val = weather_random(&weather->random_seed);
  f32 probability = (f32)(rand_val % 1000) / 1000.0f;

  // Season-based weather probabilities
  switch (weather->current_season) {
  case SEASON_SPRING:
    if (probability < 0.3f)
      return WEATHER_RAIN_LIGHT;
    if (probability < 0.5f)
      return WEATHER_CLOUDY;
    if (probability < 0.7f)
      return WEATHER_CLEAR;
    if (probability < 0.85f)
      return WEATHER_RAIN_MODERATE;
    if (probability < 0.95f)
      return WEATHER_FOG_LIGHT;
    return WEATHER_STORM;

  case SEASON_SUMMER:
    if (probability < 0.4f)
      return WEATHER_CLEAR;
    if (probability < 0.6f)
      return WEATHER_CLOUDY;
    if (probability < 0.8f)
      return WEATHER_RAIN_LIGHT;
    if (probability < 0.9f)
      return WEATHER_RAIN_MODERATE;
    if (probability < 0.95f)
      return WEATHER_STORM;
    return WEATHER_CLEAR;

  case SEASON_AUTUMN:
    if (probability < 0.2f)
      return WEATHER_CLEAR;
    if (probability < 0.4f)
      return WEATHER_CLOUDY;
    if (probability < 0.6f)
      return WEATHER_RAIN_LIGHT;
    if (probability < 0.8f)
      return WEATHER_RAIN_MODERATE;
    if (probability < 0.9f)
      return WEATHER_FOG_LIGHT;
    return WEATHER_STORM;

  case SEASON_WINTER:
    if (probability < 0.3f)
      return WEATHER_SNOW_LIGHT;
    if (probability < 0.5f)
      return WEATHER_CLOUDY;
    if (probability < 0.7f)
      return WEATHER_SNOW_MODERATE;
    if (probability < 0.85f)
      return WEATHER_SNOW_HEAVY;
    if (probability < 0.95f)
      return WEATHER_BLIZZARD;
    return WEATHER_CLEAR;

  default:
    return WEATHER_CLEAR;
  }
}

static void weather_update_properties(WeatherSystem *weather) {
  const WeatherProperties *current_props =
      &WEATHER_PROPERTIES[weather->current.type];
  const WeatherProperties *next_props =
      weather->current.is_transitioning
          ? &WEATHER_PROPERTIES[weather->current.next_type]
          : current_props;

  f32 t = weather->current.transition_progress;

  // Update temperature with seasonal and daily variations
  f32 seasonal_temp = 0.0f;
  switch (weather->current_season) {
  case SEASON_SPRING:
    seasonal_temp = 0.0f;
    break;
  case SEASON_SUMMER:
    seasonal_temp = 0.3f;
    break;
  case SEASON_AUTUMN:
    seasonal_temp = 0.1f;
    break;
  case SEASON_WINTER:
    seasonal_temp = -0.4f;
    break;
  default:
    seasonal_temp = 0.0f;
    break;
  }

  f32 daily_variation = sinf(weather->time_of_day * 2.0f * M_PI) * 0.1f;
  weather->current.temperature = weather->config.base_temperature +
                                 seasonal_temp + daily_variation +
                                 lerp(current_props->temperature_modifier,
                                      next_props->temperature_modifier, t);

  // Update humidity
  weather->current.humidity =
      weather->config.base_humidity +
      lerp(current_props->humidity_modifier, next_props->humidity_modifier, t);

  // Update wind
  f32 wind_angle = weather_random_f32(&weather->random_seed) * 2.0f * M_PI;
  weather->current.wind_direction =
      vec3(cosf(wind_angle), 0.0f, sinf(wind_angle));
  weather->current.wind_speed =
      weather->config.wind_base_speed +
      lerp(current_props->wind_modifier, next_props->wind_modifier, t);

  // Update visibility
  weather->current.visibility =
      1.0f - lerp(current_props->reduces_visibility ? 0.5f : 0.0f,
                  next_props->reduces_visibility ? 0.5f : 0.0f, t);

  // Update light level modifier
  weather->current.light_level_modifier =
      lerp(current_props->light_modifier, next_props->light_modifier, t);

  // Update intensity
  if (!weather->current.is_transitioning) {
    weather->current.intensity = current_props->base_intensity;
  } else {
    weather->current.intensity =
        lerp(current_props->base_intensity, next_props->base_intensity, t);
  }
}

void weather_set_type(WeatherSystem *weather, WeatherType type) {
  if (!weather || !weather->initialized)
    return;
  if (type >= WEATHER_COUNT)
    return;

  weather->current.type = type;
  weather->current.intensity = WEATHER_PROPERTIES[type].base_intensity;
  weather->current.is_transitioning = false;
  weather->current.transition_progress = 0.0f;
}

void weather_force_transition(WeatherSystem *weather, WeatherType new_type) {
  if (!weather || !weather->initialized)
    return;
  if (new_type >= WEATHER_COUNT)
    return;

  weather->previous = weather->current;
  weather->current.next_type = new_type;
  weather->current.is_transitioning = true;
  weather->current.transition_progress = 0.0f;
}

WeatherType weather_get_current_type(const WeatherSystem *weather) {
  return weather ? weather->current.type : WEATHER_CLEAR;
}

f32 weather_get_intensity(const WeatherSystem *weather) {
  return weather ? weather->current.intensity : 0.0f;
}

f32 weather_get_temperature(const WeatherSystem *weather) {
  return weather ? weather->current.temperature : 20.0f;
}

f32 weather_get_visibility(const WeatherSystem *weather) {
  return weather ? weather->current.visibility : 1.0f;
}

Vec3 weather_get_wind_direction(const WeatherSystem *weather) {
  return weather ? weather->current.wind_direction : vec3(1.0f, 0.0f, 0.0f);
}

f32 weather_get_wind_speed(const WeatherSystem *weather) {
  return weather ? weather->current.wind_speed : 0.0f;
}

f32 weather_get_light_modifier(const WeatherSystem *weather) {
  return weather ? weather->current.light_level_modifier : 0.0f;
}

SeasonType weather_get_season(const WeatherSystem *weather) {
  return weather ? weather->current_season : SEASON_SPRING;
}

void weather_set_season(WeatherSystem *weather, SeasonType season) {
  if (!weather || season >= SEASON_COUNT)
    return;
  weather->current_season = season;
}

void weather_set_time_of_day(WeatherSystem *weather, f32 time) {
  if (!weather)
    return;
  weather->time_of_day = CLAMP(time, 0.0f, 1.0f);
}

bool weather_is_transitioning(const WeatherSystem *weather) {
  return weather ? weather->current.is_transitioning : false;
}

f32 weather_get_transition_progress(const WeatherSystem *weather) {
  return weather ? weather->current.transition_progress : 0.0f;
}

void weather_system_set_climate(WeatherSystem *weather, ClimateZone zone,
                                f32 altitude) {
  if (!weather || zone >= CLIMATE_COUNT)
    return;
  weather->climate_zone = zone;
  weather->altitude = altitude;

  // Adjust base temperature and humidity based on climate
  switch (zone) {
  case CLIMATE_TROPICAL:
    weather->config.base_temperature = 28.0f;
    weather->config.base_humidity = 0.8f;
    break;
  case CLIMATE_TEMPERATE:
    weather->config.base_temperature = 15.0f;
    weather->config.base_humidity = 0.6f;
    break;
  case CLIMATE_ARCTIC:
    weather->config.base_temperature = -10.0f;
    weather->config.base_humidity = 0.3f;
    break;
  case CLIMATE_DESERT:
    weather->config.base_temperature = 35.0f;
    weather->config.base_humidity = 0.1f;
    break;
  case CLIMATE_OCEANIC:
    weather->config.base_temperature = 18.0f;
    weather->config.base_humidity = 0.7f;
    break;
  case CLIMATE_CONTINENTAL:
    weather->config.base_temperature = 12.0f;
    weather->config.base_humidity = 0.5f;
    break;
  default:
    break;
  }
}

WeatherState weather_get_state_at_position(const WeatherSystem *weather,
                                           Vec3 position) {
  if (!weather) {
    WeatherState dummy = {0};
    return dummy;
  }

  WeatherState state = weather->current;

  // Apply altitude cooling (ISA scale: ~6.5C per 1000m)
  f32 altitude_diff = position.y - weather->altitude;
  f32 temp_modifier = -(altitude_diff * 0.0065f);
  state.temperature += temp_modifier;

  // Local visibility variations (e.g., fog in valleys)
  if (position.y < 40.0f && weather->current.humidity > 0.8f) {
    state.visibility *= 0.7f;
  }

  return state;
}

WeatherConfig weather_get_default_config(void) {
  WeatherConfig config = {.base_temperature = 20.0f,
                          .temperature_variation = 10.0f,
                          .base_humidity = 0.5f,
                          .humidity_variation = 0.3f,
                          .wind_base_speed = 5.0f,
                          .wind_variation = 3.0f,
                          .storm_probability = 0.1f,
                          .fog_probability = 0.15f,
                          .snow_probability = 0.2f,
                          .rain_probability = 0.3f,
                          .transition_speed = 0.5f,
                          .min_duration = 30.0f,
                          .max_duration = 300.0f};
  return config;
}

void weather_set_config(WeatherSystem *weather, const WeatherConfig *config) {
  if (!weather || !config)
    return;
  weather->config = *config;
}

// Weather utilities
const char *weather_type_get_name(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return "Unknown";
  return WEATHER_PROPERTIES[type].name;
}

const char *weather_type_get_description(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return "Unknown weather type";
  return WEATHER_PROPERTIES[type].description;
}

bool weather_type_has_precipitation(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return false;
  return WEATHER_PROPERTIES[type].has_precipitation;
}

bool weather_type_has_lightning(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return false;
  return WEATHER_PROPERTIES[type].has_lightning;
}

bool weather_type_reduces_visibility(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return false;
  return WEATHER_PROPERTIES[type].reduces_visibility;
}

f32 weather_type_get_temperature_modifier(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return 0.0f;
  return WEATHER_PROPERTIES[type].temperature_modifier;
}

f32 weather_type_get_humidity_modifier(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return 0.0f;
  return WEATHER_PROPERTIES[type].humidity_modifier;
}

f32 weather_type_get_wind_modifier(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return 0.0f;
  return WEATHER_PROPERTIES[type].wind_modifier;
}

f32 weather_type_get_light_modifier(WeatherType type) {
  if (type >= WEATHER_COUNT)
    return 0.0f;
  return WEATHER_PROPERTIES[type].light_modifier;
}

// Weather lighting system
f32 weather_get_ambient_light_level(const WeatherSystem *weather) {
  if (!weather)
    return 1.0f;

  // Base light level depends on time of day (0 = midnight, 0.5 = noon, 1.0 =
  // midnight)
  f32 time_of_day = weather->time_of_day;
  f32 base_light = 0.2f; // Minimum ambient light at night

  // Calculate sun light contribution (simple day/night cycle)
  if (time_of_day >= 0.25f && time_of_day <= 0.75f) {
    // Daytime: 6 AM to 6 PM
    f32 day_progress = (time_of_day - 0.25f) / 0.5f; // 0 = sunrise, 1 = sunset
    base_light = 0.3f + 0.7f * sinf(day_progress * 3.14159f); // Peak at noon
  } else {
    // Nighttime: 6 PM to 6 AM
    base_light = 0.2f; // Constant low light at night
  }

  // Apply weather light modifier
  f32 weather_modifier = weather->current.light_level_modifier;
  f32 final_light = base_light + weather_modifier;

  // Clamp to valid range
  if (final_light < 0.0f)
    final_light = 0.0f;
  if (final_light > 1.0f)
    final_light = 1.0f;

  return final_light;
}

f32 weather_get_sky_brightness(const WeatherSystem *weather) {
  if (!weather)
    return 1.0f;

  // Sky brightness is affected more dramatically by weather
  f32 base_brightness = 0.8f; // Clear sky brightness

  // Apply weather effects with stronger impact
  switch (weather->current.type) {
  case WEATHER_CLEAR:
    return base_brightness;
  case WEATHER_CLOUDY:
    return base_brightness * 0.7f;
  case WEATHER_RAIN_LIGHT:
    return base_brightness * 0.5f;
  case WEATHER_RAIN_MODERATE:
    return base_brightness * 0.4f;
  case WEATHER_RAIN_HEAVY:
    return base_brightness * 0.3f;
  case WEATHER_STORM:
    return base_brightness * 0.2f; // Very dark during storms
  case WEATHER_SNOW_LIGHT:
    return base_brightness * 0.6f;
  case WEATHER_SNOW_MODERATE:
    return base_brightness * 0.5f;
  case WEATHER_SNOW_HEAVY:
    return base_brightness * 0.4f;
  case WEATHER_BLIZZARD:
    return base_brightness * 0.25f;
  case WEATHER_FOG_LIGHT:
    return base_brightness * 0.7f;
  case WEATHER_FOG_MODERATE:
    return base_brightness * 0.5f;
  case WEATHER_FOG_HEAVY:
    return base_brightness * 0.3f;
  default:
    return base_brightness;
  }
}

Vec3 weather_get_sky_color(const WeatherSystem *weather) {
  if (!weather)
    return vec3(0.5f, 0.7f, 1.0f); // Default clear sky blue

  // Base sky color depends on time of day
  f32 time_of_day = weather->time_of_day;
  Vec3 base_color;

  if (time_of_day >= 0.25f && time_of_day <= 0.75f) {
    // Daytime - blue sky
    base_color = vec3(0.5f, 0.7f, 1.0f);
  } else if (time_of_day >= 0.2f && time_of_day <= 0.25f) {
    // Sunrise - orange/pink
    base_color = vec3(1.0f, 0.6f, 0.3f);
  } else if (time_of_day >= 0.75f && time_of_day <= 0.8f) {
    // Sunset - orange/red
    base_color = vec3(1.0f, 0.4f, 0.2f);
  } else {
    // Night - dark blue
    base_color = vec3(0.1f, 0.1f, 0.3f);
  }

  // Apply weather tinting
  Vec3 weather_tint = vec3(1.0f, 1.0f, 1.0f);
  f32 brightness_factor = 1.0f;

  switch (weather->current.type) {
  default:
    break;
  case WEATHER_CLEAR:
    break; // No modification
  case WEATHER_CLOUDY:
    weather_tint = vec3(0.8f, 0.8f, 0.85f);
    brightness_factor = 0.8f;
    break;
  case WEATHER_RAIN_LIGHT:
  case WEATHER_RAIN_MODERATE:
  case WEATHER_RAIN_HEAVY:
    weather_tint = vec3(0.6f, 0.6f, 0.7f);
    brightness_factor = 0.6f;
    break;
  case WEATHER_STORM:
    weather_tint = vec3(0.4f, 0.4f, 0.5f);
    brightness_factor = 0.3f;
    break;
  case WEATHER_SNOW_LIGHT:
  case WEATHER_SNOW_MODERATE:
  case WEATHER_SNOW_HEAVY:
    weather_tint = vec3(0.9f, 0.9f, 0.95f);
    brightness_factor = 0.7f;
    break;
  case WEATHER_BLIZZARD:
    weather_tint = vec3(0.7f, 0.7f, 0.8f);
    brightness_factor = 0.4f;
    break;
  case WEATHER_FOG_LIGHT:
  case WEATHER_FOG_MODERATE:
  case WEATHER_FOG_HEAVY:
    weather_tint = vec3(0.8f, 0.8f, 0.8f);
    brightness_factor = 0.5f;
    break;
  }

  // Apply weather modifications
  Vec3 final_color = vec3_scale(base_color, weather_tint);
  final_color = vec3_mul(final_color, brightness_factor);

  return final_color;
}

bool weather_has_lightning(const WeatherSystem *weather) {
  if (!weather)
    return false;
  return weather_type_has_lightning(weather->current.type);
}

// Weather save/load system for persistence
void weather_save_state(const WeatherSystem *weather,
                        WeatherSaveData *save_data) {
  if (!weather || !save_data)
    return;

  save_data->current_weather = weather->current.type;
  save_data->current_intensity = weather->current.intensity;
  save_data->target_weather = weather->current.next_type;
  save_data->target_intensity = 0.0f; // Not stored in current struct
  save_data->transition_progress = weather->current.transition_progress;
  save_data->transition_duration = 0.0f; // Not stored in current struct
  save_data->time_until_change = weather->current.time_remaining;
  save_data->forecast_hours = 0; // Not stored in current struct
  save_data->season = weather->current_season;
  save_data->season_progress = weather->season_progress;

  // Forecast not implemented in current struct
  memset(save_data->forecast, 0, sizeof(save_data->forecast));
}

bool weather_load_state(WeatherSystem *weather,
                        const WeatherSaveData *save_data) {
  if (!weather || !save_data)
    return false;

  // Validate saved data
  if (save_data->current_weather >= WEATHER_COUNT ||
      save_data->target_weather >= WEATHER_COUNT) {
    LOG_WARN("Invalid weather data in save file, using defaults");
    return false;
  }

  // Restore weather state
  weather->current.type = save_data->current_weather;
  weather->current.intensity = CLAMP(save_data->current_intensity, 0.0f, 1.0f);
  weather->current.next_type = save_data->target_weather;
  weather->current.transition_progress =
      CLAMP(save_data->transition_progress, 0.0f, 1.0f);
  weather->current.time_remaining = MAX(save_data->time_until_change, 0.0f);
  weather->current_season =
      CLAMP(save_data->season, SEASON_SPRING, SEASON_WINTER);
  weather->season_progress = CLAMP(save_data->season_progress, 0.0f, 1.0f);

  LOG_INFO("Weather state loaded: %s (intensity: %.2f)",
           weather_type_get_name(weather->current.type),
           weather->current.intensity);
  return true;
}

// Weather impact on gameplay (crop growth, visibility, movement)
f32 weather_get_movement_speed_modifier(const WeatherSystem *weather) {
  if (!weather)
    return 1.0f;

  switch (weather->current.type) {
  case WEATHER_CLEAR:
    return 1.0f;
  case WEATHER_RAIN_LIGHT:
    return 0.95f;
  case WEATHER_RAIN_MODERATE:
    return 0.9f;
  case WEATHER_RAIN_HEAVY:
  case WEATHER_STORM:
    return 0.8f;
  case WEATHER_SNOW_LIGHT:
    return 0.9f;
  case WEATHER_SNOW_MODERATE:
    return 0.8f;
  case WEATHER_SNOW_HEAVY:
    return 0.7f;
  case WEATHER_BLIZZARD:
    return 0.5f;
  case WEATHER_FOG_LIGHT:
    return 0.95f;
  case WEATHER_FOG_MODERATE:
    return 0.85f;
  case WEATHER_FOG_HEAVY:
    return 0.75f;
  default:
    return 1.0f;
  }
}

f32 weather_get_visibility_modifier(const WeatherSystem *weather) {
  if (!weather)
    return 1.0f;

  switch (weather->current.type) {
  case WEATHER_CLEAR:
    return 1.0f;
  case WEATHER_RAIN_LIGHT:
    return 0.9f;
  case WEATHER_RAIN_MODERATE:
    return 0.8f;
  case WEATHER_RAIN_HEAVY:
    return 0.6f;
  case WEATHER_STORM:
    return 0.5f;
  case WEATHER_SNOW_LIGHT:
    return 0.9f;
  case WEATHER_SNOW_MODERATE:
    return 0.7f;
  case WEATHER_SNOW_HEAVY:
    return 0.5f;
  case WEATHER_BLIZZARD:
    return 0.3f;
  case WEATHER_FOG_LIGHT:
    return 0.7f;
  case WEATHER_FOG_MODERATE:
    return 0.4f;
  case WEATHER_FOG_HEAVY:
    return 0.2f;
  default:
    return 1.0f;
  }
}
