#ifndef AMBIENT_SYSTEM_H
#define AMBIENT_SYSTEM_H

#include <common.h>
#include <math/vec3.h>
#include "include/audio/audio_system.h"

// Ambient sound types
typedef enum {
    AMBIENT_WIND,
    AMBIENT_WATER,
    AMBIENT_FIRE,
    AMBIENT_CAVE,
    AMBIENT_FOREST,
    AMBIENT_DESERT,
    AMBIENT_SNOW,
    AMBIENT_NIGHT,
    AMBIENT_UNDERGROUND,
    AMBIENT_NETHER,
    AMBIENT_END,
    AMBIENT_COUNT
} AmbientType;

// Environment zones for ambient sounds
typedef enum {
    ENVIRONMENT_SURFACE,
    ENVIRONMENT_UNDERGROUND,
    ENVIRONMENT_UNDERWATER,
    ENVIRONMENT_NETHER,
    ENVIRONMENT_END,
    ENVIRONMENT_COUNT
} EnvironmentZone;

// Ambient sound layer
typedef struct {
    SoundType sound_type;
    u32 channel;
    f32 base_volume;
    f32 current_volume;
    f32 target_volume;
    f32 fade_speed;
    bool active;
    bool looping;
    Vec3 position; // For spatialized ambience
    f32 max_distance;
} AmbientLayer;

// Environmental audio zone
typedef struct {
    EnvironmentZone zone_type;
    Vec3 min_bounds;
    Vec3 max_bounds;
    AmbientLayer layers[AMBIENT_COUNT];
    u32 active_layers;
    f32 transition_fade_time;
    bool active;
} AudioZone;

// Ambient system state
typedef struct AmbientSystem {
    bool initialized;
    
    // Global ambient layers
    AmbientLayer global_layers[AMBIENT_COUNT];
    
    // Environmental zones
    AudioZone zones[32];
    u32 zone_count;
    EnvironmentZone current_zone;
    EnvironmentZone previous_zone;
    f32 zone_transition_time;
    f32 zone_transition_duration;
    
    // Time and weather effects
    f32 time_of_day; // 0.0 to 1.0
    f32 weather_intensity; // 0.0 to 1.0
    Vec3 player_position;
    f32 player_height; // Y coordinate
    
    // Dynamic ambient parameters
    f32 wind_intensity;
    f32 water_proximity; // 0.0 to 1.0
    f32 cave_depth; // 0.0 = surface, 1.0 = deep underground
    f32 forest_density; // 0.0 to 1.0
    f32 biome_temperature; // 0.0 = cold, 1.0 = hot
    
    // Audio system reference
    AudioSystem *audio;
    
    // Volume control
    f32 master_volume;
    f32 zone_volumes[ENVIRONMENT_COUNT];
    f32 ambient_volumes[AMBIENT_COUNT];
    
    // Performance settings
    u32 max_concurrent_ambients;
    u32 active_ambient_count;
    bool enable_spatial_ambience;
    f32 update_rate; // seconds between updates
    
    // Internal timing
    f32 accumulated_time;
} AmbientSystem;

// Ambient system lifecycle
void ambient_system_init(AmbientSystem *ambient, AudioSystem *audio);
void ambient_system_free(AmbientSystem *ambient);
void ambient_system_update(AmbientSystem *ambient, f32 delta_time);

// Zone management
u32 ambient_add_zone(AmbientSystem *ambient, EnvironmentZone zone_type, 
                     Vec3 min_bounds, Vec3 max_bounds);
void ambient_remove_zone(AmbientSystem *ambient, u32 zone_index);
void ambient_set_zone_volume(AmbientSystem *ambient, EnvironmentZone zone, f32 volume);

// Ambient layer control
void ambient_play_layer(AmbientSystem *ambient, AmbientType type, f32 volume);
void ambient_stop_layer(AmbientSystem *ambient, AmbientType type);
void ambient_fade_layer(AmbientSystem *ambient, AmbientType type, f32 target_volume, f32 duration);
void ambient_set_layer_volume(AmbientSystem *ambient, AmbientType type, f32 volume);

// Environmental updates
void ambient_set_player_position(AmbientSystem *ambient, Vec3 position);
void ambient_set_time_of_day(AmbientSystem *ambient, f32 time);
void ambient_set_weather_intensity(AmbientSystem *ambient, f32 intensity);
void ambient_set_biome_data(AmbientSystem *ambient, f32 temperature, f32 forest_density);

// Dynamic ambient parameters
void ambient_set_wind_intensity(AmbientSystem *ambient, f32 intensity);
void ambient_set_water_proximity(AmbientSystem *ambient, f32 proximity);
void ambient_set_cave_depth(AmbientSystem *ambient, f32 depth);
void ambient_set_environmental_data(AmbientSystem *ambient, f32 wind, f32 water, f32 cave, 
                                   f32 forest, f32 temperature);

// Volume control
void ambient_set_master_volume(AmbientSystem *ambient, f32 volume);
void ambient_set_ambient_volume(AmbientSystem *ambient, AmbientType type, f32 volume);

// Utility functions
const char* ambient_get_type_name(AmbientType type);
const char* ambient_get_zone_name(EnvironmentZone zone);
EnvironmentZone ambient_detect_environment(AmbientSystem *ambient, Vec3 position);
bool ambient_is_position_in_zone(AudioZone *zone, Vec3 position);

#endif
