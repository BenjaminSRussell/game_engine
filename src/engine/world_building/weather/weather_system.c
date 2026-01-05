#include "engine/world_building/weather/weather_system.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef LERP
#define LERP(a,b,t) ((a) + ((b)-(a))*(t))
#endif

static float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

static float clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

// Forward Declarations
void weather_manager_random_weather(WeatherManager *wm);
void time_of_day_update(TimeOfDaySystem *tod, float dt);
void transition_update(WeatherManager *wm, float dt);
void precipitation_update(PrecipitationSystem *precip, float dt);
void transition_start(WeatherManager *wm, WeatherType to, float duration);

/* =================================================================================================
 *                                    WEATHER PARAMETERS
 * =================================================================================================
 */

WeatherParameters weather_params_create(WeatherType type) {
    WeatherParameters params;
    memset(&params, 0, sizeof(WeatherParameters));
    params.type = type;
    
    switch (type) {
        case WEATHER_CLEAR:
            strcpy(params.name, "Clear");
            params.cloud_coverage = 0.1f;
            params.sun_intensity_multiplier = 1.0f;
            params.visibility_range = 1000.0f;
            params.sky_color[0] = 0.4f; params.sky_color[1] = 0.6f; params.sky_color[2] = 0.9f;
            break;
        case WEATHER_RAIN:
            strcpy(params.name, "Rain");
            params.has_precipitation = true;
            params.precipitation_intensity = 0.5f;
            params.cloud_coverage = 0.8f;
            params.sun_intensity_multiplier = 0.5f;
            params.visibility_range = 200.0f;
            params.sky_color[0] = 0.3f; params.sky_color[1] = 0.3f; params.sky_color[2] = 0.35f;
            break;
        case WEATHER_STORM:
            strcpy(params.name, "Storm");
            params.has_precipitation = true;
            params.precipitation_intensity = 1.0f;
            params.cloud_coverage = 1.0f;
            params.sun_intensity_multiplier = 0.2f;
            params.visibility_range = 50.0f;
            params.wind_speed = 20.0f;
            break;
        case WEATHER_CLOUDY:
            strcpy(params.name, "Cloudy");
            params.cloud_coverage = 0.6f;
            params.sun_intensity_multiplier = 0.7f;
            params.visibility_range = 600.0f;
             params.sky_color[0] = 0.6f; params.sky_color[1] = 0.6f; params.sky_color[2] = 0.65f;
            break;
        case WEATHER_FOG:
            strcpy(params.name, "Fog");
            params.fog_density = 0.1f;
            params.visibility_range = 30.0f;
            params.sun_intensity_multiplier = 0.4f;
             params.sky_color[0] = 0.5f; params.sky_color[1] = 0.55f; params.sky_color[2] = 0.6f;
            break;
        case WEATHER_SNOW:
            strcpy(params.name, "Snow");
            params.has_precipitation = true;
            params.precipitation_intensity = 0.4f;
            params.cloud_coverage = 0.7f;
            break;
        default:
            strcpy(params.name, "Unknown");
            break;
    }
    return params;
}

WeatherParameters weather_params_lerp(WeatherParameters a, WeatherParameters b, float t) {
    WeatherParameters result;
    t = clamp(t, 0.0f, 1.0f);

    result.type = (t < 0.5f) ? a.type : b.type;
    strcpy(result.name, (t < 0.5f) ? a.name : b.name);
    
    result.cloud_coverage = LERP(a.cloud_coverage, b.cloud_coverage, t);
    result.precipitation_intensity = LERP(a.precipitation_intensity, b.precipitation_intensity, t);
    result.sun_intensity_multiplier = LERP(a.sun_intensity_multiplier, b.sun_intensity_multiplier, t);
    result.fog_density = LERP(a.fog_density, b.fog_density, t);
    result.wind_speed = LERP(a.wind_speed, b.wind_speed, t);
    result.visibility_range = LERP(a.visibility_range, b.visibility_range, t);

    for(int i=0; i<3; i++) {
        result.sky_color[i] = LERP(a.sky_color[i], b.sky_color[i], t);
        result.fog_color[i] = LERP(a.fog_color[i], b.fog_color[i], t);
        result.wind_direction[i] = LERP(a.wind_direction[i], b.wind_direction[i], t);
    }
    
    result.has_precipitation = (t > 0.5f) ? b.has_precipitation : a.has_precipitation;

    return result;
}

bool weather_params_load(WeatherParameters *params, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fread(params, sizeof(WeatherParameters), 1, f);
    fclose(f);
    return true;
}

bool weather_params_save(const WeatherParameters *params, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(params, sizeof(WeatherParameters), 1, f);
    fclose(f);
    return true;
}

/* =================================================================================================
 *                                    WEATHER ZONES
 * =================================================================================================
 */

WeatherZone weather_zone_create(const char *name, float min[3], float max[3]) {
    WeatherZone zone;
    memset(&zone, 0, sizeof(WeatherZone));
    zone.id = rand(); 
    strncpy(zone.name, name, 31);
    memcpy(zone.bounds_min, min, sizeof(float)*3);
    memcpy(zone.bounds_max, max, sizeof(float)*3);
    return zone;
}

bool weather_zone_check_position(WeatherZone *zone, float pos[3]) {
    if (!zone) return false;
    if (pos[0] < zone->bounds_min[0] || pos[0] > zone->bounds_max[0]) return false;
    if (pos[1] < zone->bounds_min[1] || pos[1] > zone->bounds_max[1]) return false;
    if (pos[2] < zone->bounds_min[2] || pos[2] > zone->bounds_max[2]) return false;
    return true;
}

float weather_zone_blend(WeatherZone *zone, float pos[3]) {
    if (!weather_zone_check_position(zone, pos)) return 0.0f;
    // Simple edge fade could go here
    return 1.0f; 
}

/* =================================================================================================
 *                                    WEATHER TRANSITION
 * =================================================================================================
 */

void transition_start(WeatherManager *wm, WeatherType to, float duration) {
    if (!wm) return;
    wm->transition.from_weather = wm->current_weather;
    wm->transition.to_weather = to;
    wm->transition.duration = duration > 0 ? duration : 0.001f;
    wm->transition.progress = 0.0f;
    wm->transition.is_active = true;
}

void transition_update(WeatherManager *wm, float dt) {
    if (!wm || !wm->transition.is_active) return;
    
    wm->transition.progress += dt / wm->transition.duration;
    
    WeatherParameters pA = weather_params_create(wm->transition.from_weather);
    WeatherParameters pB = weather_params_create(wm->transition.to_weather);
    
    wm->transition.current_params = weather_params_lerp(pA, pB, wm->transition.progress);

    if (wm->transition.progress >= 1.0f) {
        wm->transition.is_active = false;
        wm->current_weather = wm->transition.to_weather;
        wm->transition.progress = 0.0f;
    }
}

void transition_complete(WeatherManager *wm) {
    if(wm && wm->transition.is_active) {
        wm->transition.progress = 1.0f;
        transition_update(wm, 0);
    }
}

void transition_interrupt(WeatherManager *wm) {
    if(wm) wm->transition.is_active = false;
}

/* =================================================================================================
 *                                    TIME OF DAY
 * =================================================================================================
 */

void time_of_day_init(TimeOfDaySystem *tod) {
    if(!tod) return;
    memset(tod, 0, sizeof(TimeOfDaySystem));
    tod->time_scale = 60.0f; // 1 min = 1 hour
    tod->current_time = 12.0f;
    tod->day_length_hours = 24.0f;
}

void time_of_day_update(TimeOfDaySystem *tod, float dt) {
    if(!tod || tod->is_paused) return;
    
    float hours_passed = (dt * tod->time_scale) / 3600.0f; 
    tod->current_time += hours_passed;
    
    if (tod->current_time >= 24.0f) {
        tod->current_time -= 24.0f;
        tod->day++;
        if (tod->on_midnight) tod->on_midnight();
    }
}

void time_of_day_set_time(TimeOfDaySystem *tod, float time) {
    if(tod) tod->current_time = fmodf(time, 24.0f);
}

void time_of_day_add_preset(TimeOfDaySystem *tod, TimeOfDayPreset preset) {
    // Basic stub for preset list management
}

void time_of_day_interpolate(TimeOfDaySystem *tod) {
    // calculate sun pos, update settings
}

void time_of_day_calculate_sun_position(TimeOfDaySystem *tod) {
    float angle = (tod->current_time / 24.0f) * 3.14159f * 2.0f;
    tod->current_settings.sun_elevation = sinf(angle - 3.14159f/2.0f); 
}

void time_of_day_calculate_moon_position(TimeOfDaySystem *tod) {
    // Opposite to sun
}

void time_of_day_calculate_sky_colors(TimeOfDaySystem *tod) {
    // Update colors based on elevation
}

void time_of_day_trigger_events(TimeOfDaySystem *tod) {
    // Check hour triggers
}

// Serialization stubs
void time_of_day_serialize(void) {
    // Stub: Serialize time of day state
}

void time_of_day_deserialize(void) {
    // Stub: Deserialize time of day state
}

/* =================================================================================================
 *                                    CLOUD SYSTEM
 * =================================================================================================
 */

void cloud_ray_march(VolumetricCloudSettings *clouds) {
    if (!clouds) return;
    // Stub: Ray march volumetric clouds
}

float cloud_density_sample(VolumetricCloudSettings *clouds, float p[3]) {
    (void)clouds; (void)p;
    return 0.5f; // Stub density
}

void cloud_light_march(void) {
    // Stub: Calculate light scattering
}

void cloud_shape_noise(void) {
    // Stub: Generate/sample shape noise
}

void cloud_weather_map(void) {
    // Stub: Update weather map
}

void cloud_temporal_reprojection(void) {
    // Stub: Reproject previous frame
}

void cloud_render(void) {
    // Stub: Render clouds to framebuffer
}

/* =================================================================================================
 *                                    PRECIPITATION SYSTEM
 * =================================================================================================
 */

void precipitation_init(PrecipitationSystem *precip) {
    if(precip) {
        memset(precip, 0, sizeof(PrecipitationSystem));
        precip->max_particles = 10000;
        precip->active = true;
    }
}

void precipitation_update(PrecipitationSystem *precip, float dt) {
    if(!precip || !precip->active) return;
    // Iterate particles, move down
}

void precipitation_emit(PrecipitationSystem *precip, int count) {
    // Spawn particles around camera
}

// Stubs for complex physics
void precipitation_simulate(void) {
    // Stub: Update particle physics
}

void precipitation_render(void) {
    // Stub: Render particles
}

void precipitation_collision(void) {
    // Stub: Handle particle collision
}

void precipitation_splash(void) {
    // Stub: Create splash effects
}

void precipitation_accumulation(void) {
    // Stub: Accumulate snow/water
}

/* =================================================================================================
 *                                    LIGHTNING SYSTEM
 * =================================================================================================
 */

void lightning_generate_bolt(LightningSystem *sys, float start[3], float end[3]) {
    if(!sys) return;
    // Generate fractal bolt
}

// Stubs
void lightning_subdivide(void) {
    // Stub: Fractal subdivision
}

void lightning_render(void) {
    // Stub: Render bolts
}

void lightning_flash(void) {
    // Stub: Screen flash effect
}

void lightning_thunder_delay(void) {
    // Stub: Audio delay
}

void lightning_strike_at(void) {
    // Stub: Create strike entity/damage
}

void lightning_damage(void) {
    // Stub: Apply damage area
}

/* =================================================================================================
 *                                    WEATHER MANAGER
 * =================================================================================================
 */

void weather_manager_init(WeatherManager *wm) {
    if(!wm) return;
    memset(wm, 0, sizeof(WeatherManager));
    time_of_day_init(&wm->time_of_day);
    precipitation_init(&wm->precipitation);
    wm->current_weather = WEATHER_CLEAR;
    wm->auto_weather_enabled = true;
    wm->weather_change_interval = 300.0f;
    wm->next_weather_change = 300.0f;
}

void weather_manager_shutdown(WeatherManager *wm) {
    // Cleanup
}

void weather_manager_update(WeatherManager *wm, float dt) {
    if(!wm) return;
    
    time_of_day_update(&wm->time_of_day, dt);
    
    if(wm->transition.is_active) {
        transition_update(wm, dt);
    }
    
    precipitation_update(&wm->precipitation, dt);
    
    if (wm->auto_weather_enabled) {
        wm->next_weather_change -= dt;
        if(wm->next_weather_change <= 0) {
            weather_manager_random_weather(wm);
            wm->next_weather_change = wm->weather_change_interval;
        }
    }
}

void weather_manager_set_weather(WeatherManager *wm, WeatherType type) {
    if(wm) {
        wm->current_weather = type;
        wm->transition.is_active = false;
    }
}

void weather_manager_transition_to(WeatherManager *wm, WeatherType type, float duration) {
    transition_start(wm, type, duration);
}

void weather_manager_random_weather(WeatherManager *wm) {
    int r = rand() % 5;
    WeatherType next = WEATHER_CLEAR;
    if (r == 1) next = WEATHER_RAIN;
    if (r == 2) next = WEATHER_CLOUDY;
    if (r == 3) next = WEATHER_FOG;
    if (r == 4) next = WEATHER_STORM;
    
    weather_manager_transition_to(wm, next, 10.0f);
}

WeatherType weather_manager_get_at_position(WeatherManager *wm, float pos[3]) {
    if (!wm) return WEATHER_CLEAR;
    // Check zones
    return wm->current_weather;
}

void weather_manager_apply_to_rendering(WeatherManager *wm) {
    // Send uniforms
}

void weather_manager_serialize(void) {
    // Stub: Serialize weather state
}

void weather_manager_deserialize(void) {
    // Stub: Deserialize weather state
}
