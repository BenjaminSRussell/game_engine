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

#ifdef __cplusplus
extern "C" {
#endif

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

bool time_of_day_init(TimeOfDaySystem *system, float day_length_hours);
void time_of_day_update(TimeOfDaySystem *system, float delta_time);
void time_of_day_set_time(TimeOfDaySystem *system, float time);
bool time_of_day_add_preset(TimeOfDaySystem *system, const char *name, float time, const TimeOfDaySettings *settings);
void time_of_day_interpolate(TimeOfDaySystem *system, float time);
void time_of_day_calculate_sun_position(float time, float *azimuth, float *elevation);
void time_of_day_calculate_moon_position(float time, float moon_phase, float *azimuth, float *elevation);
void time_of_day_calculate_sky_colors(float sun_elevation, float *zenith_color, float *horizon_color, float *ambient_color);
void time_of_day_trigger_events(TimeOfDaySystem *system);
bool time_of_day_serialize(const TimeOfDaySystem *system, char *buffer, size_t buffer_size);
bool time_of_day_deserialize(TimeOfDaySystem *system, const char *buffer);

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

float cloud_ray_march(const VolumetricCloudSettings *settings, const float *ray_start, const float *ray_dir, float max_distance, float *scattered_light);
float cloud_density_sample(const VolumetricCloudSettings *settings, float x, float y, float z);
float cloud_light_march(const VolumetricCloudSettings *settings, const float *start, const float *end, const float *light_dir);
float cloud_shape_noise(const VolumetricCloudSettings *settings, float x, float y, float z);
void cloud_weather_map(const VolumetricCloudSettings *settings, float time,
                       float *out_map, uint32_t width, uint32_t height);
void cloud_temporal_reprojection(const float *previous_map,
                                 float *current_map, uint32_t width,
                                 uint32_t height, float blend_factor);
void cloud_render(const VolumetricCloudSettings *settings,
                  const float *weather_map, uint32_t width, uint32_t height,
                  float *out_rgba);

/* =================================================================================================
 *                                    PRECIPITATION SYSTEM
 * =================================================================================================
 */

typedef struct PrecipitationSystem {
  bool active;
  WeatherType type;
  float intensity;
  float precipitation_amount;
  float precipitation_rate;

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

void precipitation_init(PrecipitationSystem *system, WeatherType type);
void precipitation_update(PrecipitationSystem *system, float delta_time);
void precipitation_emit(PrecipitationSystem *system, uint32_t emit_count);
void precipitation_simulate(PrecipitationSystem *system, float delta_time);
void precipitation_render(PrecipitationSystem *system);
void precipitation_collision(PrecipitationSystem *system, float ground_height);
void precipitation_splash(PrecipitationSystem *system, uint32_t splash_count);
float precipitation_accumulation(float current_accumulation,
                                 const PrecipitationSystem *system,
                                 float delta_time);

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

bool lightning_generate_bolt(LightningSystem *system, const float start[3],
                             const float end[3]);
void lightning_subdivide(LightningBolt *bolt, float displacement);
void lightning_render(LightningSystem *system);
void lightning_flash(LightningBolt *bolt, float intensity, float duration);
float lightning_thunder_delay(float distance);
// TODO(AGENT_WORLD_1): Implement lightning_strike_at [Difficulty: 5]
float lightning_damage(float distance, float base_damage);

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

void weather_manager_init(WeatherManager *manager);
void weather_manager_shutdown(WeatherManager *manager);
void weather_manager_update(WeatherManager *manager, float delta_time);
void weather_manager_set_weather(WeatherManager *manager, WeatherType type);
void weather_manager_transition_to(WeatherManager *manager, WeatherType type,
                                   float duration);
WeatherType weather_manager_random_weather(WeatherManager *manager);
bool weather_manager_get_at_position(const WeatherManager *manager,
                                     const float position[3],
                                     WeatherParameters *out_params);
// TODO(AGENT_WORLD_1): Implement weather_manager_apply_to_rendering
// [Difficulty: 6]
// TODO(AGENT_WORLD_1): Implement weather_manager_serialize [Difficulty: 5]
// TODO(AGENT_WORLD_1): Implement weather_manager_deserialize [Difficulty: 5]

#ifdef __cplusplus
}
#endif

#endif // WEATHER_SYSTEM_H
