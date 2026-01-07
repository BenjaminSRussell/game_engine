// Seasonal system implementation
#include "../include/weather/seasonal_system.h"
#include "../../engine/include/core/logger.h"
#include <string.h>

// Default configuration
static const SeasonConfig DEFAULT_CONFIG = {.days_per_season = 7,
                                            .temperature_modifier = 0.0f,
                                            .rainfall_modifier = 1.0f};

void seasonal_system_init(SeasonalSystem *sys) {
  if (!sys)
    return;

  memset(sys, 0, sizeof(SeasonalSystem));
  sys->config = DEFAULT_CONFIG;
  sys->current_season = SEASON_SPRING;
  sys->day_of_season = 1;
  sys->initialized = true;

  LOG_INFO("Seasonal system initialized (7-day cycle)");
}

void seasonal_system_update(SeasonalSystem *sys, f32 day_increment) {
  if (!sys || !sys->initialized)
    return;

  // In a real game loop, this would accumulate time
  // For now, assuming external system increments 'total_days' or we handle tick
  // accumulation here Let's assume day_increment is "fraction of a day passed"

  static f32 accumulated_time = 0.0f;
  accumulated_time += day_increment;

  if (accumulated_time >= 1.0f) {
    // New day
    sys->total_days++;
    sys->day_of_season++;
    accumulated_time -= 1.0f;

    // Check for season change
    if (sys->day_of_season > sys->config.days_per_season) {
      sys->day_of_season = 1;
      sys->current_season = (sys->current_season + 1) % SEASON_COUNT;
      LOG_INFO("Season changed to %s",
               seasonal_system_get_name(sys->current_season));
    }
  }

  sys->season_progress = (f32)(sys->day_of_season - 1 + accumulated_time) /
                         (f32)sys->config.days_per_season;
}

SeasonValue seasonal_system_get_season(SeasonalSystem *sys) {
  if (!sys)
    return SEASON_SPRING;
  return sys->current_season;
}

f32 seasonal_system_get_temperature_offset(SeasonalSystem *sys) {
  if (!sys)
    return 0.0f;

  switch (sys->current_season) {
  case SEASON_SPRING:
    return 0.0f;
  case SEASON_SUMMER:
    return 5.0f;
  case SEASON_AUTUMN:
    return -2.0f;
  case SEASON_WINTER:
    return -10.0f;
  default:
    return 0.0f;
  }
}

f32 seasonal_system_get_rainfall_multiplier(SeasonalSystem *sys) {
  if (!sys)
    return 1.0f;

  switch (sys->current_season) {
  case SEASON_SPRING:
    return 1.5f; // Rainy
  case SEASON_SUMMER:
    return 0.5f; // Dry
  case SEASON_AUTUMN:
    return 1.2f; // Wet
  case SEASON_WINTER:
    return 1.0f; // Snow?
  default:
    return 1.0f;
  }
}

const char *seasonal_system_get_name(SeasonValue season) {
  switch (season) {
  case SEASON_SPRING:
    return "Spring";
  case SEASON_SUMMER:
    return "Summer";
  case SEASON_AUTUMN:
    return "Autumn";
  case SEASON_WINTER:
    return "Winter";
  default:
    return "Unknown";
  }
}

f32 seasonal_system_get_growth_speed(SeasonalSystem *sys, u32 plant_type) {
  if (!sys)
    return 1.0f;

  switch (sys->current_season) {
  case SEASON_SPRING:
    return 1.5f; // Boost growth
  case SEASON_SUMMER:
    return 1.0f;
  case SEASON_AUTUMN:
    return 0.8f;
  case SEASON_WINTER:
    return 0.2f; // Very slow
  default:
    return 1.0f;
  }
}
