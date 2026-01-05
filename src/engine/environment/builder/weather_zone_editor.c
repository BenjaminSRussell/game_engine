#include "environment/builder/weather_zone_editor.h"
#include <string.h>

typedef struct {
    float min_point[3];
    float max_point[3];
    float blend_radius;
    int weather_type;
    float wind_vector[3];
} WeatherZone;

#define MAX_ZONES 32
static WeatherZone g_zones[MAX_ZONES];
static int g_zone_count = 0;

void weather_zone_add(float min[3], float max[3], int type, float blend) {
    if (g_zone_count >= MAX_ZONES) return;
    WeatherZone *z = &g_zones[g_zone_count++];
    memcpy(z->min_point, min, sizeof(float)*3);
    memcpy(z->max_point, max, sizeof(float)*3);
    z->weather_type = type;
    z->blend_radius = blend;
    z->wind_vector[0] = 1.0f; z->wind_vector[1] = 0.0f; z->wind_vector[2] = 0.0f;
}

void weather_zone_get_params_at(float pos[3], int *out_type, float *out_intensity, float out_wind[3]) {
    // Determine which zone we are in and blend
    // Simple AABB check + blend distance
    *out_type = 0; // Default clear
    *out_intensity = 0.0f;
    
    for (int i = 0; i < g_zone_count; i++) {
        // ... (Logic to check if pos is in box)
        // For stub simplicity, return first zone params
    }
}

void weather_zone_spawn_particles(float pos[3], int type) {
    // Spawn rain/snow around camera
    // GPU Instancing setup would be here
    // particle_system_emit(pos, type == WEATHER_RAIN ? PARTICLE_RAIN : PARTICLE_SNOW);
}

void weather_zone_set_wind(int zone_index, float x, float y, float z) {
    if (zone_index < g_zone_count) {
        g_zones[zone_index].wind_vector[0] = x;
        g_zones[zone_index].wind_vector[1] = y;
        g_zones[zone_index].wind_vector[2] = z;
    }
}
