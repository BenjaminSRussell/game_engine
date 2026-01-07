/**
 * =================================================================================================
 *                              WEATHER & TIME OF DAY SYSTEM
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Dynamic weather with smooth transitions, time of day, and atmospheric
 * effects.
 *
 * =================================================================================================
 */

#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    WEATHER TYPES
 * =================================================================================================
 */

typedef enum WeatherType {
  WEATHER_CLEAR,
  WEATHER_PARTLY_CLOUDY,
  WEATHER_CLOUDY,
  WEATHER_OVERCAST,
  WEATHER_LIGHT_RAIN,
  WEATHER_RAIN,
  WEATHER_HEAVY_RAIN,
  WEATHER_THUNDERSTORM,
  WEATHER_LIGHT_SNOW,
  WEATHER_SNOW,
  WEATHER_BLIZZARD,
  WEATHER_FOG,
  WEATHER_SANDSTORM,
  WEATHER_HAIL,
  WEATHER_SLEET,
} WeatherType;

typedef struct WeatherParameters {
  WeatherType type;
  char name[32];

  // Sky
  float sky_color[3];
  float horizon_color[3];
  float cloud_coverage;
  float cloud_speed;
  float cloud_height;
  uint32_t cloud_texture;

  // Precipitation
  bool has_precipitation;
  float precipitation_intensity;
  uint32_t precipitation_texture;
  float precipitation_size;
  float precipitation_speed;
  bool precipitation_affects_water;

  // Fog
  float fog_density;
  float fog_height_falloff;
  float fog_color[3];
  float fog_start_distance;
  float fog_end_distance;

  // Lighting
  float sun_intensity_multiplier;
  float ambient_intensity_multiplier;
  float shadow_intensity_multiplier;

  // Wind
  float wind_speed;
  float wind_direction[3];
  float wind_gustiness;

  // Audio
  char ambient_sound[128];
  float ambient_volume;

  // Gameplay effects
  float visibility_range;
  float movement_speed_modifier;
  float fire_extinguish_rate;
} WeatherParameters;

// TODO(AGENT_WORLD_1): Implement weather_params_create [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement weather_params_lerp [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_params_load [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_params_save [Difficulty: 4]

/* =================================================================================================
 *                                    WEATHER ZONES
 * =================================================================================================
 */

typedef struct WeatherZone {
  uint32_t id;
  char name[32];
  float bounds_min[3];
  float bounds_max[3];
  float blend_distance;
  WeatherType forced_weather;
  bool override_global;
  float probability_modifiers[15]; // Per weather type
} WeatherZone;

// TODO(AGENT_WORLD_1): Implement weather_zone_create [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement weather_zone_check_position [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement weather_zone_blend [Difficulty: 5]

/* =================================================================================================
 *                                    WEATHER TRANSITION
 * =================================================================================================
 */

typedef struct WeatherTransition {
  WeatherType from_weather;
  WeatherType to_weather;
  float duration;
  float progress;
  bool is_active;
  WeatherParameters current_params;
} WeatherTransition;

// TODO(AGENT_WORLD_1): Implement transition_start [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement transition_update [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement transition_complete [Difficulty: 3]
// TODO(AGENT_WORLD_1): Implement transition_interrupt [Difficulty: 4]

/* =================================================================================================
 *                                    TIME OF DAY
 * =================================================================================================
 */

typedef struct TimeOfDaySettings {
  // Sun
  float sun_azimuth;
  float sun_elevation;
  float sun_color[3];
  float sun_intensity;
  float sun_disk_size;

  // Moon
  float moon_azimuth;
  float moon_elevation;
  float moon_color[3];
  float moon_intensity;
  float moon_phase; // 0-1

  // Sky
  float zenith_color[3];
  float horizon_color[3];
  float ground_color[3];
  float star_intensity;
  float atmosphere_thickness;

  // Ambient
  float ambient_color[3];
  float ambient_intensity;

  // Fog
  float fog_color[3];
  float shadow_color[3];
} TimeOfDaySettings;

typedef struct TimeOfDayPreset {
  char name[32];
  float time; // 0-24
  TimeOfDaySettings settings;
} TimeOfDayPreset;

typedef struct TimeOfDaySystem {
  float current_time; // 0-24
  float time_scale;
  bool is_paused;

  TimeOfDayPreset *presets;
  uint32_t preset_count;

  TimeOfDaySettings current_settings;

  // Calendar
  uint32_t day;
  uint32_t month;
  uint32_t year;
  float day_length_hours;

  // Callbacks
  void (*on_sunrise)(void);
  void (*on_sunset)(void);
  void (*on_midnight)(void);
  void (*on_noon)(void);
} TimeOfDaySystem;

// TODO(AGENT_WORLD_1): Implement time_of_day_init [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement time_of_day_update [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement time_of_day_set_time [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement time_of_day_add_preset [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement time_of_day_interpolate [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement time_of_day_calculate_sun_position
// [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement time_of_day_calculate_moon_position
// [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement time_of_day_calculate_sky_colors [Difficulty:
// 6]
// TODO(AGENT_WORLD_1): Implement time_of_day_trigger_events [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement time_of_day_serialize [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement time_of_day_deserialize [Difficulty: 4]

/* =================================================================================================
 *                                    CLOUD SYSTEM
 * =================================================================================================
 */

typedef struct VolumetricCloudSettings {
  bool enabled;
  float coverage;
  float cloud_type; // 0=stratus, 0.5=cumulus, 1=cumulonimbus
  float density;
  float altitude_min;
  float altitude_max;
  float detail_scale;
  float weather_scale;
  float wind_speed;
  float wind_direction[2];
  float light_absorption;
  float ambient_occlusion;
  float shadow_strength;
  uint32_t shape_noise;
  uint32_t detail_noise;
  uint32_t weather_map;
  int32_t ray_march_steps;
  int32_t light_march_steps;
} VolumetricCloudSettings;

// TODO(AGENT_WORLD_1): Implement cloud_ray_march [Difficulty: 8]
// TODO(AGENT_WORLD_1): Implement cloud_density_sample [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement cloud_light_march [Difficulty: 7]
// TODO(AGENT_WORLD_1): Implement cloud_shape_noise [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement cloud_weather_map [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement cloud_temporal_reprojection [Difficulty: 7]
// TODO(AGENT_WORLD_1): Implement cloud_render [Difficulty: 8]

/* =================================================================================================
 *                                    PRECIPITATION SYSTEM
 * =================================================================================================
 */

typedef struct PrecipitationSystem {
  bool active;
  WeatherType type;
  float intensity;

  // GPU particles
  uint32_t particle_buffer;
  uint32_t max_particles;
  uint32_t active_particles;

  // Render settings
  uint32_t particle_texture;
  float particle_size;
  float streak_length;
  float opacity;

  // Physics
  float fall_speed;
  float wind_influence;
  float collision_offset;

  // Splash effects
  bool spawn_splashes;
  uint32_t splash_texture;
  float splash_rate;
  float splash_size;
} PrecipitationSystem;

// TODO(AGENT_WORLD_1): Implement precipitation_init [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement precipitation_update [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement precipitation_emit [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement precipitation_simulate [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement precipitation_render [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement precipitation_collision [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement precipitation_splash [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement precipitation_accumulation [Difficulty: 6]

/* =================================================================================================
 *                                    LIGHTNING SYSTEM
 * =================================================================================================
 */

typedef struct LightningBolt {
  float start[3];
  float end[3];
  float segments[32][3];
  uint32_t segment_count;
  float intensity;
  float lifetime;
  float age;
  bool has_flash;
  float flash_intensity;
} LightningBolt;

typedef struct LightningSystem {
  LightningBolt *bolts;
  uint32_t bolt_count;
  uint32_t max_bolts;

  float strike_frequency;
  float next_strike_time;
  float strike_interval_min;
  float strike_interval_max;
  float bolt_lifetime;
  float flash_duration;
  bool trigger_thunder;
} LightningSystem;

// TODO(AGENT_WORLD_1): Implement lightning_generate_bolt [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement lightning_subdivide [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement lightning_render [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement lightning_flash [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement lightning_thunder_delay [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement lightning_strike_at [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement lightning_damage [Difficulty: 4]

/* =================================================================================================
 *                                    WEATHER MANAGER
 * =================================================================================================
 */

typedef struct WeatherManager {
  WeatherParameters weather_presets[15];
  WeatherType current_weather;
  WeatherTransition transition;
  TimeOfDaySystem time_of_day;
  VolumetricCloudSettings clouds;
  PrecipitationSystem precipitation;
  LightningSystem lightning;

  WeatherZone *zones;
  uint32_t zone_count;

  // Random weather
  bool auto_weather_enabled;
  float weather_change_interval;
  float next_weather_change;
  float weather_probabilities[15];

  // Season affects weather
  bool seasonal_weather;
  uint32_t current_season;
} WeatherManager;

// TODO(AGENT_WORLD_1): Implement weather_manager_init [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_shutdown [Difficulty: 4]
// TODO(AGENT_WORLD_1): Implement weather_manager_update [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement weather_manager_set_weather [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_transition_to [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_random_weather [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_get_at_position [Difficulty:
// 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_apply_to_rendering
// [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement weather_manager_serialize [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_deserialize [Difficulty: 5]

#endif // WEATHER_SYSTEM_H
