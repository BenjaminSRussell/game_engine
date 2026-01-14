// Integrated Atmospheric Effects System
// Combines volumetric fog and atmospheric scattering for unified weather and time-of-day effects

#ifndef ATMOSPHERIC_EFFECTS_H
#define ATMOSPHERIC_EFFECTS_H

#include <common.h>
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "lighting/volumetric/volumetric_fog.h"
#include "environment/atmosphere/sky/atmosphere_system.h"
#include "environment/atmosphere/sky/atmosphere_scattering.h"
#include <stdbool.h>

// Weather conditions
typedef enum {
  WEATHER_CLEAR,
  WEATHER_PARTLY_CLOUDY,
  WEATHER_OVERCAST,
  WEATHER_LIGHT_RAIN,
  WEATHER_HEAVY_RAIN,
  WEATHER_THUNDERSTORM,
  WEATHER_LIGHT_SNOW,
  WEATHER_HEAVY_SNOW,
  WEATHER_BLIZZARD,
  WEATHER_FOG,
  WEATHER_MIST,
  WEATHER_HAZE
} WeatherCondition;

// Time of day
typedef enum {
  TIME_DAWN,        // 5:00 - 7:00
  TIME_MORNING,     // 7:00 - 10:00
  TIME_NOON,        // 10:00 - 14:00
  TIME_AFTERNOON,   // 14:00 - 17:00
  TIME_DUSK,        // 17:00 - 19:00
  TIME_EVENING,     // 19:00 - 22:00
  TIME_NIGHT,       // 22:00 - 5:00
  TIME_MIDNIGHT     // 0:00 - 2:00
} TimeOfDay;

// Season
typedef enum {
  SEASON_SPRING,
  SEASON_SUMMER,
  SEASON_AUTUMN,
  SEASON_WINTER
} Season;

// Atmospheric parameters
typedef struct {
  // Sun/Moon parameters
  Vec3 sun_direction;
  Vec3 moon_direction;
  f32 sun_intensity;
  f32 moon_intensity;
  Vec3 sun_color;
  Vec3 moon_color;
  
  // Sky parameters
  Vec3 horizon_color;
  Vec3 zenith_color;
  f32 sky_brightness;
  f32 sky_contrast;
  
  // Fog parameters
  f32 fog_density;
  f32 fog_height_falloff;
  Vec3 fog_color;
  f32 fog_scattering;
  f32 fog_absorption;
  
  // Atmospheric scattering
  f32 rayleigh_scattering;
  f32 mie_scattering;
  f32 ozone_absorption;
  f32 atmosphere_height;
  
  // Weather effects
  f32 precipitation_intensity;
  f32 cloud_coverage;
  f32 cloud_height;
  f32 wind_strength;
  Vec3 wind_direction;
  
  // Quality settings
  u32 volumetric_samples;
  u32 scattering_samples;
  bool enable_aerial_perspective;
  bool enable_multiple_scattering;
  
} AtmosphericParameters;

// Cloud system
typedef struct {
  Vec3 *positions;
  Vec3 *sizes;
  f32 *densities;
  u32 count;
  u32 max_count;
  
  // Cloud rendering
  void *cloud_texture;
  void *cloud_noise_texture;
  f32 cloud_scale;
  f32 cloud_speed;
  
  // Cloud physics
  f32 formation_rate;
  f32 dissipation_rate;
  f32 precipitation_threshold;
  
} CloudSystem;

// Precipitation system
typedef struct {
  Vec3 *particles;
  Vec3 *velocities;
  f32 *lifetimes;
  f32 *sizes;
  u32 count;
  u32 max_count;
  
  // Precipitation type
  bool is_snow;
  f32 particle_size;
  f32 fall_speed;
  f32 wind_effect;
  
  // Rendering
  void *precipitation_texture;
  f32 splash_effect_intensity;
  
} PrecipitationSystem;

// Lightning system (for thunderstorms)
typedef struct {
  Vec3 strike_positions[16];
  f32 strike_times[16];
  u32 active_strikes;
  f32 next_strike_time;
  
  // Lightning parameters
  f32 strike_frequency;
  f32 strike_duration;
  f32 branch_probability;
  
  // Audio/visual
  f32 thunder_delay;
  f32 flash_intensity;
  Vec3 flash_color;
  
} LightningSystem;

// Integrated atmospheric effects system
typedef struct {
  // Core systems
  volumetric_fog_t *volumetric_fog;
  atmosphere_system_t *atmosphere_scattering;
  
  // Atmospheric state
  AtmosphericParameters params;
  WeatherCondition current_weather;
  TimeOfDay current_time;
  Season current_season;
  f32 time_of_day_hours; // 0.0 - 24.0
  
  // Sub-systems
  CloudSystem clouds;
  PrecipitationSystem precipitation;
  LightningSystem lightning;
  
  // Transition state
  WeatherCondition target_weather;
  f32 weather_transition_time;
  f32 weather_transition_duration;
  
  // Rendering resources
  void *lookup_table_texture;
  void *aerial_perspective_texture;
  void *cloud_shadow_texture;
  
  // Performance
  bool high_quality_mode;
  u32 frame_counter;
  f32 average_frame_time;
  
} AtmosphericEffectsSystem;

// Environmental audio effects
typedef struct {
  f32 wind_volume;
  f32 rain_volume;
  f32 thunder_volume;
  f32 ambient_volume;
  
  // Reverb zones
  f32 outdoor_reverb;
  f32 indoor_reverb;
  f32 cave_reverb;
  
  // Weather-specific audio
  bool thunder_playing;
  f32 thunder_distance;
  f32 wind_gust_intensity;
  
} AtmosphericAudio;

#ifdef __cplusplus
extern "C" {
#endif

// System management
AtmosphericEffectsSystem *atmospheric_effects_create(void *device);
void atmospheric_effects_destroy(AtmosphericEffectsSystem *system);

// Time and season control
void atmospheric_effects_set_time_of_day(AtmosphericEffectsSystem *system, f32 hours);
void atmospheric_effects_set_date(AtmosphericEffectsSystem *system, u32 day, u32 month);
void atmospheric_effects_advance_time(AtmosphericEffectsSystem *system, f32 delta_hours);

// Weather control
void atmospheric_effects_set_weather(AtmosphericEffectsSystem *system, WeatherCondition weather, f32 transition_time);
void atmospheric_effects_set_weather_immediate(AtmosphericEffectsSystem *system, WeatherCondition weather);
WeatherCondition atmospheric_effects_get_weather(const AtmosphericEffectsSystem *system);

// Atmospheric parameters
void atmospheric_effects_update_parameters(AtmosphericEffectsSystem *system);
void atmospheric_effects_set_fog_density(AtmosphericEffectsSystem *system, f32 density);
void atmospheric_effects_set_atmosphere_height(AtmosphericEffectsSystem *system, f32 height);
void atmospheric_effects_set_sun_position(AtmosphericEffectsSystem *system, Vec3 direction, f32 intensity);

// Cloud system
void atmospheric_effects_update_clouds(AtmosphericEffectsSystem *system, f32 delta_time);
void atmospheric_effects_set_cloud_coverage(AtmosphericEffectsSystem *system, f32 coverage);
void atmospheric_effects_generate_clouds(AtmosphericEffectsSystem *system);

// Precipitation
void atmospheric_effects_update_precipitation(AtmosphericEffectsSystem *system, f32 delta_time);
void atmospheric_effects_set_precipitation_intensity(AtmosphericEffectsSystem *system, f32 intensity);
void atmospheric_effects_create_splash_effects(AtmosphericEffectsSystem *system, Vec3 position);

// Lightning (thunderstorms)
void atmospheric_effects_update_lightning(AtmosphericEffectsSystem *system, f32 delta_time);
void atmospheric_effects_trigger_lightning_strike(AtmosphericEffectsSystem *system, Vec3 position);
bool atmospheric_effects_is_lightning_active(const AtmosphericEffectsSystem *system);

// Main update function
void atmospheric_effects_update(AtmosphericEffectsSystem *system, f32 delta_time);

// Rendering
void atmospheric_effects_render_sky(AtmosphericEffectsSystem *system, void *encoder, void *camera);
void atmospheric_effects_render_volumetric(AtmosphericEffectsSystem *system, void *cmd_buffer, void *camera);
void atmospheric_effects_render_clouds(AtmosphericEffectsSystem *system, void *encoder);
void atmospheric_effects_render_precipitation(AtmosphericEffectsSystem *system, void *encoder);

// Lighting integration
Vec3 atmospheric_effects_get_sun_color(const AtmosphericEffectsSystem *system);
Vec3 atmospheric_effects_get_ambient_color(const AtmosphericEffectsSystem *system);
f32 atmospheric_effects_get_ambient_intensity(const AtmosphericEffectsSystem *system);
Vec3 atmospheric_effects_get_fog_color(const AtmosphericEffectsSystem *system);

// Environmental queries
bool atmospheric_effects_is_outdoors(const AtmosphericEffectsSystem *system, Vec3 position);
f32 atmospheric_effects_get_visibility_distance(const AtmosphericEffectsSystem *system);
f32 atmospheric_effects_get_humidity(const AtmosphericEffectsSystem *system);
f32 atmospheric_effects_get_temperature(const AtmosphericEffectsSystem *system);

// Audio integration
AtmosphericAudio atmospheric_effects_get_audio_state(const AtmosphericEffectsSystem *system);
void atmospheric_effects_update_audio(AtmosphericEffectsSystem *system, AtmosphericAudio *audio);

// Quality and performance
void atmospheric_effects_set_quality_level(AtmosphericEffectsSystem *system, u32 level);
void atmospheric_effects_enable_high_quality(AtmosphericEffectsSystem *system, bool enable);
void atmospheric_effects_update_performance_metrics(AtmosphericEffectsSystem *system, f32 frame_time);

// Presets
void atmospheric_effects_load_preset(AtmosphericEffectsSystem *system, const char *preset_name);
void atmospheric_effects_save_preset(const AtmosphericEffectsSystem *system, const char *preset_name);

// Utility functions
const char *atmospheric_weather_to_string(WeatherCondition weather);
const char *atmospheric_time_to_string(TimeOfDay time);
const char *atmospheric_season_to_string(Season season);
f32 atmospheric_effects_get_sun_angle(const AtmosphericEffectsSystem *system);
f32 atmospheric_effects_get_moon_angle(const AtmosphericEffectsSystem *system);

#ifdef __cplusplus
}
#endif

#endif // ATMOSPHERIC_EFFECTS_H
