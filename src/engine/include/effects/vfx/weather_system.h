#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include "include/core/types.h"
#include "../math/vec3.h"
#include "../math/mat4.h"
#include "include/core/memory.h"

// Weather system configuration
#define WEATHER_MAX_CLOUD_LAYERS 8
#define WEATHER_MAX_PARTICLES 500000
#define WEATHER_MAX_LIGHTNING_BOLTS 64
#define WEATHER_MAX_PRECIPITATION_ZONES 16
#define WEATHER_SKDOME_RESOLUTION 256
#define WEATHER_TIME_OF_DAY_STEPS 1440  // Minutes in a day

// Weather states
typedef enum {
    WEATHER_CLEAR,
    WEATHER_CLOUDY,
    WEATHER_OVERCAST,
    WEATHER_LIGHT_RAIN,
    WEATHER_HEAVY_RAIN,
    WEATHER_STORM,
    WEATHER_LIGHT_SNOW,
    WEATHER_HEAVY_SNOW,
    WEATHER_BLIZZARD,
    WEATHER_FOG,
    WEATHER_HAZE
} WeatherState;

// Precipitation types
typedef enum {
    PRECIPITATION_NONE,
    PRECIPITATION_RAIN,
    PRECIPITATION_SNOW,
    PRECIPITATION_HAIL,
    PRECIPITATION_SLEET
} PrecipitationType;

// Time of day
typedef struct {
    f32 current_time;        // 0.0 to 24.0 hours
    u32 day_count;
    f32 sun_angle;
    f32 moon_angle;
    Vec3 sun_direction;
    Vec3 moon_direction;
    Vec3 sun_color;
    Vec3 moon_color;
    f32 ambient_intensity;
} TimeOfDay;

// Atmospheric scattering
typedef struct {
    Vec3 rayleigh_scattering;    // RGB coefficients
    Vec3 mie_scattering;         // RGB coefficients
    f32 rayleigh_scale;
    f32 mie_scale;
    f32 mie_g;                   // Anisotropy parameter
    f32 atmosphere_height;
    f32 planet_radius;
} AtmosphericScattering;

// Cloud layer
typedef struct {
    f32 altitude;
    f32 thickness;
    f32 coverage;
    f32 density;
    Vec3 albedo;
    f32 absorption;
    u32 noise_texture_id;
    f32 noise_scale;
    f32 animation_speed;
    Vec3 wind_direction;
    f32 wind_speed;
} CloudLayer;

// Volumetric cloud data
typedef struct {
    u32 volume_texture_id;
    f32 voxel_size;
    u32 grid_width, grid_height, grid_depth;
    f32 scattering_coefficient;
    f32 absorption_coefficient;
    f32 phase_function_g;
} VolumetricClouds;

// Precipitation zone
typedef struct {
    Vec3 center;
    f32 radius;
    PrecipitationType type;
    f32 intensity;
    f32 particle_rate;
    u32 particle_count;
    Vec3 wind_effect;
    f32 accumulation_rate;
} PrecipitationZone;

// Lightning bolt
typedef struct {
    Vec3 start_point;
    Vec3 end_point;
    Vec3* branch_points;
    u32 branch_count;
    f32 intensity;
    f64 creation_time;
    f64 duration;
    Vec3 color;
    f32 thickness;
} LightningBolt;

// Surface wetness
typedef struct {
    f32 global_wetness;          // 0.0 to 1.0
    f32 puddle_intensity;
    u32 puddle_texture_id;
    f32 evaporation_rate;
    f32 absorption_rate;
} SurfaceWetness;

// Snow coverage
typedef struct {
    f32 global_coverage;         // 0.0 to 1.0
    f32 accumulation_height;
    u32 snow_texture_id;
    f32 melting_rate;
    f32 compaction_rate;
} SnowCoverage;

// Weather system
typedef struct {
    // Time and atmosphere
    TimeOfDay time_of_day;
    AtmosphericScattering atmosphere;
    
    // Cloud system
    CloudLayer cloud_layers[WEATHER_MAX_CLOUD_LAYERS];
    u32 cloud_layer_count;
    VolumetricClouds volumetric_clouds;
    
    // Precipitation
    PrecipitationZone precipitation_zones[WEATHER_MAX_PRECIPITATION_ZONES];
    u32 precipitation_zone_count;
    
    // Lightning
    LightningBolt lightning_bolts[WEATHER_MAX_LIGHTNING_BOLTS];
    u32 lightning_bolt_count;
    
    // Surface interaction
    SurfaceWetness surface_wetness;
    SnowCoverage snow_coverage;
    
    // Current weather state
    WeatherState current_state;
    WeatherState target_state;
    f32 transition_progress;
    f32 transition_duration;
    
    // Wind system
    Vec3 global_wind;
    f32 wind_variation;
    f32 wind_gust_strength;
    
    // Lighting
    Vec3 ambient_color;
    Vec3 fog_color;
    f32 fog_density;
    f32 visibility_distance;
    
    // Audio
    f32 rain_volume;
    f32 thunder_volume;
    f32 wind_volume;
    u64 next_thunder_time;
    
    // Performance
    bool enable_volumetric_clouds;
    bool enable_high_quality_precipitation;
    u32 particle_lod_level;
    
    // Statistics
    u64 weather_update_time_ms;
    u64 cloud_render_time_ms;
    u64 precipitation_update_time_ms;
} WeatherSystem;

// Core API functions
void weather_system_init(WeatherSystem* system);
void weather_system_shutdown(WeatherSystem* system);
void weather_system_update(WeatherSystem* system, f32 delta_time);

// Time of day (TASK_2000)
void weather_update_time_of_day(WeatherSystem* system, f32 delta_time);
void weather_set_time_of_day(WeatherSystem* system, f32 hours);
void weather_get_sun_moon_positions(WeatherSystem* system, Vec3* sun_dir, Vec3* moon_dir);
void weather_compute_celestial_bodies(WeatherSystem* system);

// Atmospheric scattering (TASK_2001)
void weather_compute_atmospheric_scattering(WeatherSystem* system, Vec3 view_dir, Vec3 light_dir, Vec3* out_color);
void weather_update_sky_colors(WeatherSystem* system);
void weather_render_sky_dome(WeatherSystem* system);

// Volumetric clouds (TASK_2002)
void weather_init_volumetric_clouds(WeatherSystem* system);
void weather_update_volumetric_clouds(WeatherSystem* system, f32 delta_time);
void weather_render_volumetric_clouds(WeatherSystem* system);
void weather_generate_cloud_noise(WeatherSystem* system);

// Star map and moon phases (TASK_2003)
void weather_render_star_map(WeatherSystem* system);
void weather_update_moon_phase(WeatherSystem* system);
void weather_compute_star_brightness(WeatherSystem* system);

// Rain particles (TASK_2010)
void weather_init_rain_system(WeatherSystem* system);
void weather_update_rain_particles(WeatherSystem* system, f32 delta_time);
void weather_render_rain_particles(WeatherSystem* system);
void weather_set_rain_intensity(WeatherSystem* system, f32 intensity);

// Snow particles (TASK_2011)
void weather_init_snow_system(WeatherSystem* system);
void weather_update_snow_particles(WeatherSystem* system, f32 delta_time);
void weather_render_snow_particles(WeatherSystem* system);
void weather_set_snow_intensity(WeatherSystem* system, f32 intensity);

// Hail and fog (TASK_2012-2013)
void weather_init_hail_system(WeatherSystem* system);
void weather_update_hail_particles(WeatherSystem* system, f32 delta_time);
void weather_update_fog_density(WeatherSystem* system);
void weather_generate_lightning(WeatherSystem* system);

// Surface wetness (TASK_2020)
void weather_update_global_wetness(WeatherSystem* system, f32 delta_time);
void weather_apply_wetness_shader(WeatherSystem* system);
f32 weather_get_surface_wetness_at_position(WeatherSystem* system, Vec3 position);

// Puddles (TASK_2021)
void weather_update_puddle_simulation(WeatherSystem* system);
void weather_render_puddles(WeatherSystem* system);
void weather_generate_puddle_texture(WeatherSystem* system);

// Snow coverage (TASK_2022)
void weather_update_snow_coverage(WeatherSystem* system, f32 delta_time);
void weather_apply_snow_displacement(WeatherSystem* system);
void weather_compact_snow(WeatherSystem* system);

// Wind effects (TASK_2023)
void weather_update_wind_system(WeatherSystem* system, f32 delta_time);
void weather_apply_wind_to_vegetation(WeatherSystem* system);
Vec3 weather_get_wind_at_position(WeatherSystem* system, Vec3 position);

// Global illumination (TASK_2030)
void weather_update_gi_based_on_weather(WeatherSystem* system);
void weather_compute_cloud_lighting(WeatherSystem* system);

// Cloud shadows (TASK_2031)
void weather_render_cloud_shadows(WeatherSystem* system);
void weather_update_shadow_maps(WeatherSystem* system);

// Ambient color (TASK_2032)
void weather_update_ambient_colors(WeatherSystem* system);
Vec3 weather_get_current_ambient_color(WeatherSystem* system);

// Dynamic ambience (TASK_2040)
void weather_update_dynamic_ambience(WeatherSystem* system);
void weather_set_weather_audio_parameters(WeatherSystem* system);

// 3D thunder (TASK_2041)
void weather_generate_thunder_event(WeatherSystem* system, Vec3 lightning_position);
void weather_update_thunder_audio(WeatherSystem* system);

// Weather states (TASK_2050)
void weather_set_weather_state(WeatherSystem* system, WeatherState state);
void weather_transition_to_state(WeatherSystem* system, WeatherState target_state, f32 duration);
WeatherState weather_get_current_state(WeatherSystem* system);

// Smooth transitions (TASK_2051)
void weather_update_state_transition(WeatherSystem* system, f32 delta_time);
f32 weather_get_transition_progress(WeatherSystem* system);

// Local weather zones (TASK_2052)
void weather_create_local_weather_zone(WeatherSystem* system, Vec3 center, f32 radius, WeatherState state);
void weather_update_local_zones(WeatherSystem* system);
WeatherState weather_get_weather_at_position(WeatherSystem* system, Vec3 position);

// Utility functions
Vec3 weather_get_current_sun_color(WeatherSystem* system);
Vec3 weather_get_current_moon_color(WeatherSystem* system);
f32 weather_get_current_visibility(WeatherSystem* system);
f32 weather_get_current_humidity(WeatherSystem* system);

#endif // WEATHER_SYSTEM_H
