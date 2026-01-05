// Seasonal system configuration and API
#ifndef SEASONAL_SYSTEM_H
#define SEASONAL_SYSTEM_H

#include "../game_common.h"

// Season types
typedef enum {
  SEASON_SPRING,
  SEASON_SUMMER,
  SEASON_AUTUMN,
  SEASON_WINTER,
  SEASON_COUNT
} SeasonValue;

// Seasonal configuration
typedef struct {
  u32 days_per_season;
  f32 temperature_modifier; // Global offset
  f32 rainfall_modifier;    // Multiplier
} SeasonConfig;

// Core system state
typedef struct {
  SeasonValue current_season;
  u32 day_of_season;
  u32 total_days;
  f32 season_progress; // 0.0 - 1.0 within current season
  SeasonConfig config;
  bool initialized;
} SeasonalSystem;

// Initialization
void seasonal_system_init(SeasonalSystem *sys);

// Update logic
void seasonal_system_update(
    SeasonalSystem *sys,
    f32 day_increment); // day_increment e.g. 1.0/24000.0 per tick

// Query functions
SeasonValue seasonal_system_get_season(SeasonalSystem *sys);
f32 seasonal_system_get_temperature_offset(SeasonalSystem *sys);
f32 seasonal_system_get_rainfall_multiplier(SeasonalSystem *sys);
const char *seasonal_system_get_name(SeasonValue season);

// Plant growth modifiers
f32 seasonal_system_get_growth_speed(SeasonalSystem *sys, u32 plant_type);

#endif // SEASONAL_SYSTEM_H
