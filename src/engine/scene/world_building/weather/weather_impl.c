/**
 * =================================================================================================
 *                              WEATHER & TIME OF DAY SYSTEM
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Implementation of dynamic weather with smooth transitions, time of day, and atmospheric
 * effects.
 *
 * =================================================================================================
 */

#include "weather_system.h"
#include "unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* =================================================================================================
 *                                    WEATHER PARAMETERS
 * =================================================================================================
 */

static WeatherParameters weather_presets[15] = {0};

WeatherParameters* weather_params_create(WeatherType type) {
    WeatherParameters *params = malloc(sizeof(WeatherParameters));
    if (!params) return NULL;
    
    memset(params, 0, sizeof(WeatherParameters));
    params->type = type;
    
    // Set default values based on weather type
    switch (type) {
        case WEATHER_CLEAR:
            strcpy(params->name, "Clear");
            params->sky_color[0] = 0.53f; params->sky_color[1] = 0.81f; params->sky_color[2] = 0.92f;
            params->sun_intensity_multiplier = 1.0f;
            params->visibility_range = 1000.0f;
            break;
            
        case WEATHER_PARTLY_CLOUDY:
            strcpy(params->name, "Partly Cloudy");
            params->sky_color[0] = 0.63f; params->sky_color[1] = 0.78f; params->sky_color[2] = 0.85f;
            params->cloud_coverage = 0.4f;
            params->sun_intensity_multiplier = 0.9f;
            params->visibility_range = 800.0f;
            break;
            
        case WEATHER_CLOUDY:
            strcpy(params->name, "Cloudy");
            params->sky_color[0] = 0.73f; params->sky_color[1] = 0.76f; params->sky_color[2] = 0.78f;
            params->cloud_coverage = 0.8f;
            params->sun_intensity_multiplier = 0.7f;
            params->visibility_range = 600.0f;
            break;
            
        case WEATHER_OVERCAST:
            strcpy(params->name, "Overcast");
            params->sky_color[0] = 0.5f; params->sky_color[1] = 0.52f; params->sky_color[2] = 0.54f;
            params->cloud_coverage = 1.0f;
            params->sun_intensity_multiplier = 0.5f;
            params->visibility_range = 400.0f;
            break;
            
        case WEATHER_LIGHT_RAIN:
            strcpy(params->name, "Light Rain");
            params->sky_color[0] = 0.45f; params->sky_color[1] = 0.48f; params->sky_color[2] = 0.5f;
            params->cloud_coverage = 0.9f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.3f;
            params->sun_intensity_multiplier = 0.4f;
            params->visibility_range = 300.0f;
            strcpy(params->ambient_sound, "rain_light");
            params->ambient_volume = 0.3f;
            break;
            
        case WEATHER_RAIN:
            strcpy(params->name, "Rain");
            params->sky_color[0] = 0.35f; params->sky_color[1] = 0.38f; params->sky_color[2] = 0.4f;
            params->cloud_coverage = 1.0f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.6f;
            params->sun_intensity_multiplier = 0.3f;
            params->visibility_range = 200.0f;
            strcpy(params->ambient_sound, "rain_medium");
            params->ambient_volume = 0.5f;
            break;
            
        case WEATHER_HEAVY_RAIN:
            strcpy(params->name, "Heavy Rain");
            params->sky_color[0] = 0.25f; params->sky_color[1] = 0.28f; params->sky_color[2] = 0.3f;
            params->cloud_coverage = 1.0f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.9f;
            params->sun_intensity_multiplier = 0.2f;
            params->visibility_range = 100.0f;
            strcpy(params->ambient_sound, "rain_heavy");
            params->ambient_volume = 0.7f;
            break;
            
        case WEATHER_THUNDERSTORM:
            strcpy(params->name, "Thunderstorm");
            params->sky_color[0] = 0.15f; params->sky_color[1] = 0.18f; params->sky_color[2] = 0.2f;
            params->cloud_coverage = 1.0f;
            params->has_precipitation = true;
            params->precipitation_intensity = 1.0f;
            params->sun_intensity_multiplier = 0.1f;
            params->visibility_range = 80.0f;
            strcpy(params->ambient_sound, "thunderstorm");
            params->ambient_volume = 0.9f;
            break;
            
        case WEATHER_LIGHT_SNOW:
            strcpy(params->name, "Light Snow");
            params->sky_color[0] = 0.75f; params->sky_color[1] = 0.78f; params->sky_color[2] = 0.8f;
            params->cloud_coverage = 0.8f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.2f;
            params->sun_intensity_multiplier = 0.6f;
            params->visibility_range = 250.0f;
            strcpy(params->ambient_sound, "snow_light");
            params->ambient_volume = 0.2f;
            break;
            
        case WEATHER_SNOW:
            strcpy(params->name, "Snow");
            params->sky_color[0] = 0.7f; params->sky_color[1] = 0.73f; params->sky_color[2] = 0.75f;
            params->cloud_coverage = 0.9f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.5f;
            params->sun_intensity_multiplier = 0.5f;
            params->visibility_range = 150.0f;
            strcpy(params->ambient_sound, "snow_medium");
            params->ambient_volume = 0.4f;
            break;
            
        case WEATHER_BLIZZARD:
            strcpy(params->name, "Blizzard");
            params->sky_color[0] = 0.6f; params->sky_color[1] = 0.63f; params->sky_color[2] = 0.65f;
            params->cloud_coverage = 1.0f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.8f;
            params->sun_intensity_multiplier = 0.2f;
            params->visibility_range = 50.0f;
            strcpy(params->ambient_sound, "blizzard");
            params->ambient_volume = 0.8f;
            break;
            
        case WEATHER_FOG:
            strcpy(params->name, "Fog");
            params->sky_color[0] = 0.65f; params->sky_color[1] = 0.68f; params->sky_color[2] = 0.7f;
            params->fog_density = 0.8f;
            params->fog_start_distance = 5.0f;
            params->fog_end_distance = 50.0f;
            params->sun_intensity_multiplier = 0.4f;
            params->visibility_range = 30.0f;
            break;
            
        case WEATHER_SANDSTORM:
            strcpy(params->name, "Sandstorm");
            params->sky_color[0] = 0.8f; params->sky_color[1] = 0.7f; params->sky_color[2] = 0.5f;
            params->fog_density = 0.6f;
            params->wind_speed = 15.0f;
            params->sun_intensity_multiplier = 0.3f;
            params->visibility_range = 40.0f;
            strcpy(params->ambient_sound, "sandstorm");
            params->ambient_volume = 0.7f;
            break;
            
        case WEATHER_HAIL:
            strcpy(params->name, "Hail");
            params->sky_color[0] = 0.4f; params->sky_color[1] = 0.43f; params->sky_color[2] = 0.45f;
            params->cloud_coverage = 1.0f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.7f;
            params->sun_intensity_multiplier = 0.3f;
            params->visibility_range = 120.0f;
            strcpy(params->ambient_sound, "hail");
            params->ambient_volume = 0.6f;
            break;
            
        case WEATHER_SLEET:
            strcpy(params->name, "Sleet");
            params->sky_color[0] = 0.5f; params->sky_color[1] = 0.53f; params->sky_color[2] = 0.55f;
            params->cloud_coverage = 0.9f;
            params->has_precipitation = true;
            params->precipitation_intensity = 0.5f;
            params->sun_intensity_multiplier = 0.4f;
            params->visibility_range = 180.0f;
            strcpy(params->ambient_sound, "sleet");
            params->ambient_volume = 0.5f;
            break;
    }
    
    // Set common defaults
    params->cloud_speed = 1.0f;
    params->cloud_height = 1000.0f;
    params->fog_height_falloff = 0.1f;
    params->ambient_intensity_multiplier = 0.8f;
    params->shadow_intensity_multiplier = 0.7f;
    params->wind_direction[0] = 1.0f;
    params->wind_direction[1] = 0.0f;
    params->wind_direction[2] = 0.0f;
    params->wind_gustiness = 0.2f;
    params->movement_speed_modifier = 1.0f;
    params->fire_extinguish_rate = 0.0f;
    
    return params;
}

bool weather_params_lerp(const WeatherParameters *from, const WeatherParameters *to, 
                        float t, WeatherParameters *result) {
    if (!from || !to || !result || t < 0.0f || t > 1.0f) return false;
    
    // Linear interpolation for most parameters
    result->type = t < 0.5f ? from->type : to->type;
    
    // Colors
    for (int i = 0; i < 3; i++) {
        result->sky_color[i] = from->sky_color[i] * (1.0f - t) + to->sky_color[i] * t;
        result->horizon_color[i] = from->horizon_color[i] * (1.0f - t) + to->horizon_color[i] * t;
        result->fog_color[i] = from->fog_color[i] * (1.0f - t) + to->fog_color[i] * t;
        result->wind_direction[i] = from->wind_direction[i] * (1.0f - t) + to->wind_direction[i] * t;
    }
    
    // Float parameters
    result->cloud_coverage = from->cloud_coverage * (1.0f - t) + to->cloud_coverage * t;
    result->cloud_speed = from->cloud_speed * (1.0f - t) + to->cloud_speed * t;
    result->cloud_height = from->cloud_height * (1.0f - t) + to->cloud_height * t;
    result->precipitation_intensity = from->precipitation_intensity * (1.0f - t) + to->precipitation_intensity * t;
    result->precipitation_size = from->precipitation_size * (1.0f - t) + to->precipitation_size * t;
    result->precipitation_speed = from->precipitation_speed * (1.0f - t) + to->precipitation_speed * t;
    result->fog_density = from->fog_density * (1.0f - t) + to->fog_density * t;
    result->fog_height_falloff = from->fog_height_falloff * (1.0f - t) + to->fog_height_falloff * t;
    result->fog_start_distance = from->fog_start_distance * (1.0f - t) + to->fog_start_distance * t;
    result->fog_end_distance = from->fog_end_distance * (1.0f - t) + to->fog_end_distance * t;
    result->sun_intensity_multiplier = from->sun_intensity_multiplier * (1.0f - t) + to->sun_intensity_multiplier * t;
    result->ambient_intensity_multiplier = from->ambient_intensity_multiplier * (1.0f - t) + to->ambient_intensity_multiplier * t;
    result->shadow_intensity_multiplier = from->shadow_intensity_multiplier * (1.0f - t) + to->shadow_intensity_multiplier * t;
    result->wind_speed = from->wind_speed * (1.0f - t) + to->wind_speed * t;
    result->wind_gustiness = from->wind_gustiness * (1.0f - t) + to->wind_gustiness * t;
    result->ambient_volume = from->ambient_volume * (1.0f - t) + to->ambient_volume * t;
    result->visibility_range = from->visibility_range * (1.0f - t) + to->visibility_range * t;
    result->movement_speed_modifier = from->movement_speed_modifier * (1.0f - t) + to->movement_speed_modifier * t;
    result->fire_extinguish_rate = from->fire_extinguish_rate * (1.0f - t) + to->fire_extinguish_rate * t;
    
    // Boolean parameters (threshold-based)
    result->has_precipitation = t < 0.5f ? from->has_precipitation : to->has_precipitation;
    result->precipitation_affects_water = t < 0.5f ? from->precipitation_affects_water : to->precipitation_affects_water;
    
    // String parameters
    strcpy(result->name, t < 0.5f ? from->name : to->name);
    strcpy(result->ambient_sound, t < 0.5f ? from->ambient_sound : to->ambient_sound);
    
    // Texture parameters
    result->cloud_texture = t < 0.5f ? from->cloud_texture : to->cloud_texture;
    result->precipitation_texture = t < 0.5f ? from->precipitation_texture : to->precipitation_texture;
    
    return true;
}

bool weather_params_load(WeatherParameters *params, const char *filename) {
    if (!params || !filename) return false;
    
    FILE *file = fopen(filename, "r");
    if (!file) return false;
    
    // Simple text-based loading (in production, use JSON or binary format)
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[64], value[128];
        if (sscanf(line, "%63s = %127s", key, value) != 2) continue;
        
        if (strcmp(key, "name") == 0) {
            strcpy(params->name, value);
        } else if (strcmp(key, "sky_color_r") == 0) {
            params->sky_color[0] = atof(value);
        } else if (strcmp(key, "sky_color_g") == 0) {
            params->sky_color[1] = atof(value);
        } else if (strcmp(key, "sky_color_b") == 0) {
            params->sky_color[2] = atof(value);
        } else if (strcmp(key, "cloud_coverage") == 0) {
            params->cloud_coverage = atof(value);
        } else if (strcmp(key, "precipitation_intensity") == 0) {
            params->precipitation_intensity = atof(value);
        } else if (strcmp(key, "visibility_range") == 0) {
            params->visibility_range = atof(value);
        }
        // Add more parameters as needed
    }
    
    fclose(file);
    return true;
}

bool weather_params_save(const WeatherParameters *params, const char *filename) {
    if (!params || !filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "name = %s\n", params->name);
    fprintf(file, "sky_color_r = %.3f\n", params->sky_color[0]);
    fprintf(file, "sky_color_g = %.3f\n", params->sky_color[1]);
    fprintf(file, "sky_color_b = %.3f\n", params->sky_color[2]);
    fprintf(file, "cloud_coverage = %.3f\n", params->cloud_coverage);
    fprintf(file, "precipitation_intensity = %.3f\n", params->precipitation_intensity);
    fprintf(file, "visibility_range = %.1f\n", params->visibility_range);
    fprintf(file, "sun_intensity_multiplier = %.3f\n", params->sun_intensity_multiplier);
    fprintf(file, "ambient_intensity_multiplier = %.3f\n", params->ambient_intensity_multiplier);
    fprintf(file, "wind_speed = %.2f\n", params->wind_speed);
    fprintf(file, "ambient_sound = %s\n", params->ambient_sound);
    
    fclose(file);
    return true;
}

/* =================================================================================================
 *                                    WEATHER ZONES
 * =================================================================================================
 */

static WeatherZone *weather_zones = NULL;
static uint32_t weather_zone_capacity = 0;
static uint32_t weather_zone_count = 0;
static uint32_t next_weather_zone_id = 1;

WeatherZone* weather_zone_create(const char *name, const float bounds_min[3], 
                                const float bounds_max[3], WeatherType forced_weather) {
    if (!name || !bounds_min || !bounds_max) return NULL;
    
    // Expand zones array if needed
    if (weather_zone_count >= weather_zone_capacity) {
        uint32_t new_capacity = weather_zone_capacity == 0 ? 16 : weather_zone_capacity * 2;
        WeatherZone *new_zones = realloc(weather_zones, new_capacity * sizeof(WeatherZone));
        if (!new_zones) return NULL;
        
        weather_zones = new_zones;
        weather_zone_capacity = new_capacity;
    }
    
    WeatherZone *zone = &weather_zones[weather_zone_count++];
    memset(zone, 0, sizeof(WeatherZone));
    
    zone->id = next_weather_zone_id++;
    strncpy(zone->name, name, 31);
    zone->name[31] = '\0';
    
    zone->bounds_min[0] = bounds_min[0];
    zone->bounds_min[1] = bounds_min[1];
    zone->bounds_min[2] = bounds_min[2];
    
    zone->bounds_max[0] = bounds_max[0];
    zone->bounds_max[1] = bounds_max[1];
    zone->bounds_max[2] = bounds_max[2];
    
    zone->blend_distance = 10.0f;
    zone->forced_weather = forced_weather;
    zone->override_global = true;
    
    // Initialize probability modifiers (all neutral)
    for (int i = 0; i < 15; i++) {
        zone->probability_modifiers[i] = 1.0f;
    }
    
    return zone;
}

bool weather_zone_check_position(const WeatherZone *zone, const float position[3]) {
    if (!zone || !position) return false;
    
    return (position[0] >= zone->bounds_min[0] && position[0] <= zone->bounds_max[0] &&
            position[1] >= zone->bounds_min[1] && position[1] <= zone->bounds_max[1] &&
            position[2] >= zone->bounds_min[2] && position[2] <= zone->bounds_max[2]);
}

bool weather_zone_blend(const WeatherZone *zone, const float position[3], 
                       float *blend_factor) {
    if (!zone || !position || !blend_factor) return false;
    
    // Check if position is within zone bounds
    if (!weather_zone_check_position(zone, position)) {
        *blend_factor = 0.0f;
        return false;
    }
    
    // Calculate distance to zone edges
    float dist_to_min_x = position[0] - zone->bounds_min[0];
    float dist_to_max_x = zone->bounds_max[0] - position[0];
    float dist_to_min_y = position[1] - zone->bounds_min[1];
    float dist_to_max_y = zone->bounds_max[1] - position[1];
    float dist_to_min_z = position[2] - zone->bounds_min[2];
    float dist_to_max_z = zone->bounds_max[2] - position[2];
    
    float min_dist = fminf(fminf(fminf(dist_to_min_x, dist_to_max_x),
                                 fminf(dist_to_min_y, dist_to_max_y)),
                           fminf(dist_to_min_z, dist_to_max_z));
    
    // Calculate blend factor based on distance to edge
    if (min_dist >= zone->blend_distance) {
        *blend_factor = 1.0f; // Fully inside zone
    } else {
        *blend_factor = min_dist / zone->blend_distance; // Blend at edge
    }
    
    return true;
}

/* =================================================================================================
 *                                    WEATHER TRANSITION
 * =================================================================================================
 */

bool transition_start(WeatherTransition *transition, WeatherType from_weather, 
                    WeatherType to_weather, float duration) {
    if (!transition || duration <= 0.0f) return false;
    
    transition->from_weather = from_weather;
    transition->to_weather = to_weather;
    transition->duration = duration;
    transition->progress = 0.0f;
    transition->is_active = true;
    
    // Initialize current parameters with "from" weather
    WeatherParameters *from_params = weather_params_create(from_weather);
    if (from_params) {
        transition->current_params = *from_params;
        UNIFIED_FREE(from_params);
    }
    
    return true;
}

bool transition_update(WeatherTransition *transition, float delta_time, 
                     const WeatherParameters *to_params) {
    if (!transition || !to_params || !transition->is_active) return false;
    
    // Update progress
    transition->progress += delta_time / transition->duration;
    
    if (transition->progress >= 1.0f) {
        transition->progress = 1.0f;
        return transition_complete(transition);
    }
    
    // Get "from" parameters
    WeatherParameters *from_params = weather_params_create(transition->from_weather);
    if (!from_params) return false;
    
    // Interpolate between from and to parameters
    bool success = weather_params_lerp(from_params, to_params, transition->progress, 
                                      &transition->current_params);
    
    UNIFIED_FREE(from_params);
    return success;
}

bool transition_complete(WeatherTransition *transition) {
    if (!transition) return false;
    
    transition->is_active = false;
    transition->progress = 1.0f;
    
    // Set final parameters to "to" weather
    WeatherParameters *to_params = weather_params_create(transition->to_weather);
    if (to_params) {
        transition->current_params = *to_params;
        UNIFIED_FREE(to_params);
    }
    
    return true;
}

bool transition_interrupt(WeatherTransition *transition, WeatherType new_weather, 
                         float new_duration) {
    if (!transition) return false;
    
    // Start new transition from current state
    WeatherType current_weather = transition->progress > 0.5f ? 
                                 transition->to_weather : transition->from_weather;
    
    return transition_start(transition, current_weather, new_weather, new_duration);
}
