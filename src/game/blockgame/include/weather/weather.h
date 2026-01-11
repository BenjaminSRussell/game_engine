// Weather system core data structures and API.
// Manages weather types, transitions, and environmental effects.
#ifndef WEATHER_H
#define WEATHER_H

#include <game_common.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Forward declaration to avoid pulling Vulkan into gameplay headers
typedef struct VulkanRenderer VulkanRenderer;

// Weather types
typedef enum {
  WEATHER_CLEAR,
  WEATHER_CLOUDY,
  WEATHER_RAIN_LIGHT,
  WEATHER_RAIN_MODERATE,
  WEATHER_RAIN_HEAVY,
  WEATHER_STORM,
  WEATHER_SNOW_LIGHT,
  WEATHER_SNOW_MODERATE,
  WEATHER_SNOW_HEAVY,
  WEATHER_BLIZZARD,
  WEATHER_FOG_LIGHT,
  WEATHER_FOG_MODERATE,
  WEATHER_FOG_HEAVY,
  WEATHER_COUNT
} WeatherType;

typedef enum {
  SEASON_SPRING,
  SEASON_SUMMER,
  SEASON_AUTUMN,
  SEASON_WINTER,
  SEASON_COUNT
} SeasonType;

// Climate zones
typedef enum {
  CLIMATE_TROPICAL,
  CLIMATE_TEMPERATE,
  CLIMATE_ARCTIC,
  CLIMATE_DESERT,
  CLIMATE_OCEANIC,
  CLIMATE_CONTINENTAL,
  CLIMATE_COUNT
} ClimateZone;

// Weather intensity levels
typedef enum {
  WEATHER_INTENSITY_NONE,
  WEATHER_INTENSITY_LIGHT,
  WEATHER_INTENSITY_MODERATE,
  WEATHER_INTENSITY_HEAVY,
  WEATHER_INTENSITY_EXTREME
} WeatherIntensity;

// Weather state structure
typedef struct {
  WeatherType type;
  WeatherType next_type;
  f32 transition_progress;
  f32 intensity;
  f32 duration;
  f32 time_remaining;
  Vec3 wind_direction;
  f32 wind_speed;
  f32 temperature;
  f32 humidity;
  f32 visibility;
  f32 light_level_modifier;
  bool is_transitioning;
} WeatherState;

// Weather configuration
typedef struct {
  f32 base_temperature;
  f32 temperature_variation;
  f32 base_humidity;
  f32 humidity_variation;
  f32 wind_base_speed;
  f32 wind_variation;
  f32 storm_probability;
  f32 fog_probability;
  f32 snow_probability;
  f32 rain_probability;
  f32 transition_speed;
  f32 min_duration;
  f32 max_duration;
} WeatherConfig;

// Weather save/load data structure
#define WEATHER_FORECAST_HOURS 48
typedef struct {
  WeatherType current_weather;
  f32 current_intensity;
  WeatherType target_weather;
  f32 target_intensity;
  f32 transition_progress;
  f32 transition_duration;
  f32 time_until_change;
  u32 forecast_hours;
  SeasonType season;
  f32 season_progress;
  WeatherType forecast[WEATHER_FORECAST_HOURS];
} WeatherSaveData;

// Weather system
typedef struct WeatherSystem {
  WeatherState current;
  WeatherState previous;
  WeatherConfig config;
  SeasonType current_season;
  f32 season_progress;
  f32 time_of_day;
  bool initialized;
  u32 random_seed;

  // Climate data
  ClimateZone climate_zone;
  f32 altitude;
  f32 distance_to_ocean;

  // Forecasting
  WeatherType forecast[WEATHER_FORECAST_HOURS];
  u32 forecast_hours;

  // Target state helper (for save/load consistency)
  WeatherState target;
  f32 time_until_change;
  f32 transition_duration;
  f32 transition_progress;
} WeatherSystem;

// Weather particle data
typedef struct WeatherParticle {
  Vec3 position;
  Vec3 velocity;
  f32 life;
  f32 max_life;
  f32 size;
  u32 type;
} WeatherParticle;

#define MAX_WEATHER_PARTICLES 8192
#define WEATHER_PARTICLE_TYPES 4

// Weather particle system
typedef struct {
  WeatherParticle particles[MAX_WEATHER_PARTICLES];
  u32 active_count;
  u32 spawn_rate;
  f32 spawn_timer;
  bool enabled;
} WeatherParticleSystem;

// Public API
void weather_system_init(WeatherSystem *weather);
void weather_system_free(WeatherSystem *weather);
void weather_system_update(WeatherSystem *weather, f32 delta_time);
void weather_set_type(WeatherSystem *weather, WeatherType type);
void weather_force_transition(WeatherSystem *weather, WeatherType new_type);
WeatherType weather_get_current_type(const WeatherSystem *weather);
f32 weather_get_intensity(const WeatherSystem *weather);
f32 weather_get_temperature(const WeatherSystem *weather);
f32 weather_get_visibility(const WeatherSystem *weather);
Vec3 weather_get_wind_direction(const WeatherSystem *weather);
f32 weather_get_wind_speed(const WeatherSystem *weather);
f32 weather_get_light_modifier(const WeatherSystem *weather);
SeasonType weather_get_season(const WeatherSystem *weather);
void weather_set_season(WeatherSystem *weather, SeasonType season);
void weather_set_time_of_day(WeatherSystem *weather, f32 time);
bool weather_is_transitioning(const WeatherSystem *weather);
f32 weather_get_transition_progress(const WeatherSystem *weather);

// Climate variation support
void weather_system_set_climate(WeatherSystem *weather, ClimateZone zone,
                                f32 altitude);
WeatherState weather_get_state_at_position(const WeatherSystem *weather,
                                           Vec3 position);

// Weather configuration
void weather_set_config(WeatherSystem *weather, const WeatherConfig *config);
WeatherConfig weather_get_default_config(void);
void weather_load_config_from_file(WeatherSystem *weather,
                                   const char *filename);
void weather_save_config_to_file(const WeatherSystem *weather,
                                 const char *filename);

// Weather particles
void weather_particles_init(WeatherParticleSystem *particles);
void weather_particles_free(WeatherParticleSystem *particles);
void weather_particles_update(WeatherParticleSystem *particles,
                              const WeatherSystem *weather, f32 delta_time);
void weather_particles_spawn(WeatherParticleSystem *particles, WeatherType type,
                             Vec3 center, f32 radius);
u32 weather_particles_get_count(const WeatherParticleSystem *particles);
const WeatherParticle *
weather_particles_get_data(const WeatherParticleSystem *particles);

// Weather particle rendering hook (may be a stub until integrated with
// renderer)
void weather_particles_render(const WeatherParticleSystem *particles,
                              VulkanRenderer *renderer);

// Weather utilities
const char *weather_type_get_name(WeatherType type);
const char *weather_type_get_description(WeatherType type);
bool weather_type_has_precipitation(WeatherType type);
bool weather_type_has_lightning(WeatherType type);
bool weather_type_reduces_visibility(WeatherType type);
f32 weather_type_get_temperature_modifier(WeatherType type);
f32 weather_type_get_humidity_modifier(WeatherType type);
f32 weather_type_get_wind_modifier(WeatherType type);
f32 weather_type_get_light_modifier(WeatherType type);

// Weather lighting system
f32 weather_get_ambient_light_level(const WeatherSystem *weather);
f32 weather_get_sky_brightness(const WeatherSystem *weather);
Vec3 weather_get_sky_color(const WeatherSystem *weather);
bool weather_has_lightning(const WeatherSystem *weather);

// Weather save/load system for persistence
void weather_save_state(const WeatherSystem *weather,
                        WeatherSaveData *save_data);
bool weather_load_state(WeatherSystem *weather,
                        const WeatherSaveData *save_data);

// Weather impact on gameplay (crop growth, visibility, movement)
f32 weather_get_movement_speed_modifier(const WeatherSystem *weather);
f32 weather_get_visibility_modifier(const WeatherSystem *weather);

#endif
