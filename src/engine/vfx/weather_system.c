#include "../include/vfx/weather_system.h"
#include "../core/logger.h"
#include "../core/memory.h"
#include "../core/math.h"
#include <string.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   WEATHER SYSTEM - IMPLEMENTATION
 * =================================================================================================
 */

// TASK_2000: Implement "Time of Day" cycle (Sun/Moon position)
static f32 weather_calculate_sun_angle(f32 hours) {
    // Sun angle: 0° at midnight, 90° at sunrise (6am), 180° at noon, 270° at sunset (6pm)
    return (hours - 6.0f) * 15.0f;  // 15 degrees per hour
}

static f32 weather_calculate_moon_angle(f32 hours) {
    // Moon is opposite to sun (roughly)
    return weather_calculate_sun_angle(hours) + 180.0f;
}

static Vec3 weather_calculate_celestial_position(f32 angle_degrees, f32 elevation) {
    f32 angle_rad = angle_degrees * PI_F / 180.0f;
    f32 elevation_rad = elevation * PI_F / 180.0f;
    
    // Convert spherical to Cartesian coordinates
    return vec3(
        cosf(elevation_rad) * cosf(angle_rad),
        sinf(elevation_rad),
        cosf(elevation_rad) * sinf(angle_rad)
    );
}

static Vec3 weather_calculate_sun_color(f32 sun_angle) {
    // Color changes based on sun position (dawn, noon, dusk)
    f32 normalized_angle = fmodf(sun_angle + 360.0f, 360.0f);
    
    if (normalized_angle < 90.0f) {  // Dawn
        f32 t = normalized_angle / 90.0f;
        return vec3(
            1.0f,
            0.4f + 0.4f * t,
            0.2f + 0.3f * t
        );
    } else if (normalized_angle < 180.0f) {  // Morning to noon
        return vec3(1.0f, 0.9f, 0.8f);
    } else if (normalized_angle < 270.0f) {  // Noon to dusk
        f32 t = (normalized_angle - 180.0f) / 90.0f;
        return vec3(
            1.0f,
            0.9f - 0.3f * t,
            0.8f - 0.4f * t
        );
    } else {  // Night
        return vec3(0.1f, 0.1f, 0.2f);
    }
}

static Vec3 weather_calculate_moon_color(f32 moon_angle) {
    // Moon is generally white but slightly yellow/orange
    return vec3(0.9f, 0.9f, 0.8f);
}

void weather_update_time_of_day(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Update time (1 real second = 1 game minute by default)
    f32 time_scale = 60.0f;  // Can be adjusted for faster/slower time
    system->time_of_day.current_time += (delta_time * time_scale) / 3600.0f;  // Convert to hours
    
    // Wrap around 24 hours
    if (system->time_of_day.current_time >= 24.0f) {
        system->time_of_day.current_time -= 24.0f;
        system->time_of_day.day_count++;
    }
    
    // Calculate celestial positions
    system->time_of_day.sun_angle = weather_calculate_sun_angle(system->time_of_day.current_time);
    system->time_of_day.moon_angle = weather_calculate_moon_angle(system->time_of_day.current_time);
    
    // Calculate sun elevation (varies with time of day)
    f32 sun_elevation = 0.0f;
    f32 normalized_sun_angle = fmodf(system->time_of_day.sun_angle + 360.0f, 360.0f);
    
    if (normalized_sun_angle >= 0.0f && normalized_sun_angle <= 180.0f) {
        // Sun is above horizon
        sun_elevation = 90.0f - fabsf(normalized_sun_angle - 90.0f);
    }
    
    // Calculate moon elevation (opposite to sun)
    f32 moon_elevation = 0.0f;
    f32 normalized_moon_angle = fmodf(system->time_of_day.moon_angle + 360.0f, 360.0f);
    
    if (normalized_moon_angle >= 0.0f && normalized_moon_angle <= 180.0f) {
        moon_elevation = 90.0f - fabsf(normalized_moon_angle - 90.0f);
    }
    
    // Update celestial body positions and colors
    system->time_of_day.sun_direction = weather_calculate_celestial_position(
        system->time_of_day.sun_angle, sun_elevation);
    system->time_of_day.moon_direction = weather_calculate_celestial_position(
        system->time_of_day.moon_angle, moon_elevation);
    
    system->time_of_day.sun_color = weather_calculate_sun_color(system->time_of_day.sun_angle);
    system->time_of_day.moon_color = weather_calculate_moon_color(system->time_of_day.moon_angle);
    
    // Update ambient intensity based on sun position
    if (sun_elevation > 0.0f) {
        system->time_of_day.ambient_intensity = sun_elevation / 90.0f;
    } else {
        system->time_of_day.ambient_intensity = 0.1f;  // Minimal ambient light at night
    }
}

void weather_set_time_of_day(WeatherSystem* system, f32 hours) {
    if (!system) return;
    
    // Clamp to valid range
    hours = fmaxf(0.0f, fminf(23.999f, hours));
    system->time_of_day.current_time = hours;
    
    // Recalculate celestial positions
    weather_update_time_of_day(system, 0.0f);
    
    LOG_INFO("Time of day set to %.2f hours", hours);
}

void weather_get_sun_moon_positions(WeatherSystem* system, Vec3* sun_dir, Vec3* moon_dir) {
    if (!system) return;
    
    if (sun_dir) *sun_dir = system->time_of_day.sun_direction;
    if (moon_dir) *moon_dir = system->time_of_day.moon_direction;
}

void weather_compute_celestial_bodies(WeatherSystem* system) {
    if (!system) return;
    
    // This function can be used to update more complex celestial calculations
    // such as moon phases, seasonal variations, etc.
    
    // For now, just ensure time of day is up to date
    weather_update_time_of_day(system, 0.0f);
}

// TASK_2001: Implement "Sky Atmosphere": Rayleigh and Mie scattering
void weather_compute_atmospheric_scattering(WeatherSystem* system, Vec3 view_dir, Vec3 light_dir, Vec3* out_color) {
    if (!system || !out_color) return;
    
    // Simplified atmospheric scattering model
    f32 cos_angle = vec3_dot(view_dir, light_dir);
    
    // Rayleigh scattering (blue sky)
    f32 rayleigh_phase = 1.0f + cos_angle * cos_angle;
    Vec3 rayleigh_scattering = vec3_scale(system->atmosphere.rayleigh_scattering, rayleigh_phase);
    
    // Mie scattering (haze and fog)
    f32 mie_phase = (1.0f - system->atmosphere.mie_g * system->atmosphere.mie_g) / 
                    powf(1.0f + system->atmosphere.mie_g * system->atmosphere.mie_g - 
                         2.0f * system->atmosphere.mie_g * cos_angle, 1.5f);
    Vec3 mie_scattering = vec3_scale(system->atmosphere.mie_scattering, mie_phase);
    
    // Combine scattering contributions
    Vec3 total_scattering = vec3_add(rayleigh_scattering, mie_scattering);
    
    // Apply sun color and intensity
    Vec3 sun_contribution = vec3_mul(system->time_of_day.sun_color, total_scattering);
    sun_contribution = vec3_scale(sun_contribution, system->time_of_day.ambient_intensity);
    
    *out_color = sun_contribution;
}

void weather_update_sky_colors(WeatherSystem* system) {
    if (!system) return;
    
    // Update sky color based on time of day and weather
    Vec3 sky_color = vec3_zero();
    
    if (system->time_of_day.ambient_intensity > 0.1f) {
        // Daytime
        Vec3 zenith_dir = vec3(0.0f, 1.0f, 0.0f);
        weather_compute_atmospheric_scattering(system, zenith_dir, 
                                              system->time_of_day.sun_direction, &sky_color);
        
        // Add weather-based color modification
        if (system->current_state == WEATHER_CLOUDY || system->current_state == WEATHER_OVERCAST) {
            sky_color = vec3_scale(sky_color, 0.6f);  // Dimmer sky
        }
    } else {
        // Nighttime
        sky_color = vec3(0.05f, 0.05f, 0.1f);  // Dark blue night sky
    }
    
    system->ambient_color = sky_color;
}

// TASK_2002: Implement "Volumetric Clouds": Ray-marching through noise
void weather_init_volumetric_clouds(WeatherSystem* system) {
    if (!system) return;
    
    // Initialize volumetric cloud data
    system->volumetric_clouds.grid_width = 128;
    system->volumetric_clouds.grid_height = 64;
    system->volumetric_clouds.grid_depth = 128;
    system->volumetric_clouds.voxel_size = 16.0f;  // 16 meters per voxel
    
    system->volumetric_clouds.scattering_coefficient = 0.1f;
    system->volumetric_clouds.absorption_coefficient = 0.05f;
    system->volumetric_clouds.phase_function_g = 0.6f;
    
    // In a real implementation, would create 3D texture for cloud density
    system->volumetric_clouds.volume_texture_id = 0;  // Placeholder
    
    LOG_INFO("Volumetric clouds initialized: %dx%dx%d grid", 
             system->volumetric_clouds.grid_width,
             system->volumetric_clouds.grid_height,
             system->volumetric_clouds.grid_depth);
}

void weather_update_volumetric_clouds(WeatherSystem* system, f32 delta_time) {
    if (!system || !system->enable_volumetric_clouds) return;
    
    // Animate cloud layers
    for (u32 i = 0; i < system->cloud_layer_count; i++) {
        CloudLayer* layer = &system->cloud_layers[i];
        
        // Update cloud position based on wind
        Vec3 wind_offset = vec3_scale(system->global_wind, delta_time * 0.1f);
        layer->altitude += wind_offset.y;
        
        // Animate noise texture
        layer->animation_speed = 0.05f;  // Can be varied per layer
    }
}

void weather_render_volumetric_clouds(WeatherSystem* system) {
    if (!system || !system->enable_volumetric_clouds) return;
    
    // In a real implementation, would ray-march through cloud volume
    LOG_TRACE("Rendering volumetric clouds");
}

void weather_generate_cloud_noise(WeatherSystem* system) {
    if (!system) return;
    
    // In a real implementation, would generate procedural noise using Perlin/Simplex noise
    // For now, just log the action
    LOG_INFO("Generating cloud noise texture");
}

// TASK_2003: Add "Star Map" and Moon phases
void weather_render_star_map(WeatherSystem* system) {
    if (!system) return;
    
    // Only render stars at night
    if (system->time_of_day.ambient_intensity < 0.2f) {
        // In a real implementation, would render star field
        LOG_TRACE("Rendering star map");
    }
}

void weather_update_moon_phase(WeatherSystem* system) {
    if (!system) return;
    
    // Calculate moon phase based on day count
    // Moon cycle is approximately 29.5 days
    f32 moon_cycle = (f32)(system->time_of_day.day_count % 30) / 30.0f;
    
    // Moon phases: 0 = new moon, 0.25 = first quarter, 0.5 = full moon, 0.75 = last quarter
    // This could be used to adjust moon brightness and appearance
    
    LOG_TRACE("Moon phase: %.2f", moon_cycle);
}

void weather_compute_star_brightness(WeatherSystem* system) {
    if (!system) return;
    
    // Star brightness varies based on moon phase and atmospheric conditions
    f32 base_brightness = 1.0f;
    
    // Dimmer stars when moon is full
    f32 moon_phase = (f32)(system->time_of_day.day_count % 30) / 30.0f;
    f32 moon_brightness = fabsf(sinf(moon_phase * 2.0f * PI_F));
    
    f32 star_brightness = base_brightness * (1.0f - moon_brightness * 0.5f);
    
    // Further dim based on weather
    if (system->current_state == WEATHER_CLOUDY || system->current_state == WEATHER_OVERCAST) {
        star_brightness *= 0.3f;
    }
    
// TASK_2010: Implement "Rain" Particle system (GPU driven)
void weather_init_rain_system(WeatherSystem* system) {
    if (!system) return;
    
    // Initialize rain precipitation zone
    if (system->precipitation_zone_count < WEATHER_MAX_PRECIPITATION_ZONES) {
        PrecipitationZone* rain_zone = &system->precipitation_zones[system->precipitation_zone_count];
        rain_zone->center = vec3_zero();
        rain_zone->radius = 1000.0f;  // 1km radius
        rain_zone->type = PRECIPITATION_RAIN;
        rain_zone->intensity = 0.0f;
        rain_zone->particle_rate = 0.0f;
        rain_zone->particle_count = 0;
        rain_zone->wind_effect = vec3_zero();
        rain_zone->accumulation_rate = 0.0f;
        
        system->precipitation_zone_count++;
    }
    
    LOG_INFO("Rain system initialized");
}

void weather_update_rain_particles(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Find rain precipitation zone
    PrecipitationZone* rain_zone = NULL;
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_RAIN) {
            rain_zone = &system->precipitation_zones[i];
            break;
        }
    }
    
    if (!rain_zone) return;
    
    // Update rain intensity based on weather state
    switch (system->current_state) {
        case WEATHER_LIGHT_RAIN:
            rain_zone->intensity = 0.3f;
            rain_zone->particle_rate = 1000.0f;  // particles per second
            break;
        case WEATHER_HEAVY_RAIN:
            rain_zone->intensity = 0.7f;
            rain_zone->particle_rate = 5000.0f;
            break;
        case WEATHER_STORM:
            rain_zone->intensity = 1.0f;
            rain_zone->particle_rate = 10000.0f;
            break;
        default:
            rain_zone->intensity = 0.0f;
            rain_zone->particle_rate = 0.0f;
            break;
    }
    
    // Update wind effect on rain
    rain_zone->wind_effect = vec3_scale(system->global_wind, rain_zone->intensity * 0.5f);
    
    // Update particle count (simplified - in real implementation would manage GPU particles)
    rain_zone->particle_count = (u32)(rain_zone->particle_rate * delta_time);
    rain_zone->particle_count = (rain_zone->particle_count > WEATHER_MAX_PARTICLES) ? 
                                WEATHER_MAX_PARTICLES : rain_zone->particle_count;
}

void weather_render_rain_particles(WeatherSystem* system) {
    if (!system) return;
    
    // Find rain precipitation zone
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_RAIN && 
            system->precipitation_zones[i].particle_count > 0) {
            // In a real implementation, would render GPU particles
            LOG_TRACE("Rendering %u rain particles", system->precipitation_zones[i].particle_count);
            break;
        }
    }
}

void weather_set_rain_intensity(WeatherSystem* system, f32 intensity) {
    if (!system) return;
    
    intensity = fmaxf(0.0f, fminf(1.0f, intensity));
    
    // Update rain precipitation zone
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_RAIN) {
            system->precipitation_zones[i].intensity = intensity;
            system->precipitation_zones[i].particle_rate = intensity * 10000.0f;
            break;
        }
    }
}

// TASK_2011: Implement "Snow" Particle system (Accumulation on surfaces)
void weather_init_snow_system(WeatherSystem* system) {
    if (!system) return;
    
    // Initialize snow precipitation zone
    if (system->precipitation_zone_count < WEATHER_MAX_PRECIPITATION_ZONES) {
        PrecipitationZone* snow_zone = &system->precipitation_zones[system->precipitation_zone_count];
        snow_zone->center = vec3_zero();
        snow_zone->radius = 1000.0f;
        snow_zone->type = PRECIPITATION_SNOW;
        snow_zone->intensity = 0.0f;
        snow_zone->particle_rate = 0.0f;
        snow_zone->particle_count = 0;
        snow_zone->wind_effect = vec3_zero();
        snow_zone->accumulation_rate = 0.0f;
        
        system->precipitation_zone_count++;
    }
    
    LOG_INFO("Snow system initialized");
}

void weather_update_snow_particles(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Find snow precipitation zone
    PrecipitationZone* snow_zone = NULL;
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_SNOW) {
            snow_zone = &system->precipitation_zones[i];
            break;
        }
    }
    
    if (!snow_zone) return;
    
    // Update snow intensity based on weather state
    switch (system->current_state) {
        case WEATHER_LIGHT_SNOW:
            snow_zone->intensity = 0.3f;
            snow_zone->particle_rate = 500.0f;
            snow_zone->accumulation_rate = 0.001f;
            break;
        case WEATHER_HEAVY_SNOW:
            snow_zone->intensity = 0.7f;
            snow_zone->particle_rate = 2000.0f;
            snow_zone->accumulation_rate = 0.005f;
            break;
        case WEATHER_BLIZZARD:
            snow_zone->intensity = 1.0f;
            snow_zone->particle_rate = 5000.0f;
            snow_zone->accumulation_rate = 0.01f;
            break;
        default:
            snow_zone->intensity = 0.0f;
            snow_zone->particle_rate = 0.0f;
            snow_zone->accumulation_rate = 0.0f;
            break;
    }
    
    // Update wind effect on snow (more affected than rain)
    snow_zone->wind_effect = vec3_scale(system->global_wind, snow_zone->intensity * 0.8f);
    
    // Update particle count
    snow_zone->particle_count = (u32)(snow_zone->particle_rate * delta_time);
    snow_zone->particle_count = (snow_zone->particle_count > WEATHER_MAX_PARTICLES) ? 
                                WEATHER_MAX_PARTICLES : snow_zone->particle_count;
    
    // Update snow coverage
    weather_update_snow_coverage(system, delta_time);
}

void weather_render_snow_particles(WeatherSystem* system) {
    if (!system) return;
    
    // Find snow precipitation zone
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_SNOW && 
            system->precipitation_zones[i].particle_count > 0) {
            // In a real implementation, would render GPU particles
            LOG_TRACE("Rendering %u snow particles", system->precipitation_zones[i].particle_count);
            break;
        }
    }
}

void weather_set_snow_intensity(WeatherSystem* system, f32 intensity) {
    if (!system) return;
    
    intensity = fmaxf(0.0f, fminf(1.0f, intensity));
    
    // Update snow precipitation zone
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_SNOW) {
            system->precipitation_zones[i].intensity = intensity;
            system->precipitation_zones[i].particle_rate = intensity * 5000.0f;
            system->precipitation_zones[i].accumulation_rate = intensity * 0.01f;
            break;
        }
    }
}

// TASK_2022: Implement "Snow Coverage": additive displacement on meshes
void weather_update_snow_coverage(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Find snow precipitation zone
    f32 accumulation_rate = 0.0f;
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_SNOW) {
            accumulation_rate = system->precipitation_zones[i].accumulation_rate;
            break;
        }
    }
    
    if (accumulation_rate > 0.0f) {
        // Accumulate snow
        system->snow_coverage.global_coverage += accumulation_rate * delta_time;
        system->snow_coverage.global_coverage = fminf(1.0f, system->snow_coverage.global_coverage);
        
        // Increase accumulation height
        system->snow_coverage.accumulation_height += accumulation_rate * delta_time * 0.1f;
    } else {
        // Melt snow (temperature-based melting)
        f32 melt_rate = 0.0001f;  // Base melt rate
        if (system->time_of_day.ambient_intensity > 0.5f) {
            melt_rate *= 2.0f;  // Faster melting during day
        }
        
        system->snow_coverage.global_coverage -= melt_rate * delta_time;
        system->snow_coverage.global_coverage = fmaxf(0.0f, system->snow_coverage.global_coverage);
        
        system->snow_coverage.accumulation_height -= melt_rate * delta_time * 0.05f;
        system->snow_coverage.accumulation_height = fmaxf(0.0f, system->snow_coverage.accumulation_height);
    }
}

void weather_apply_snow_displacement(WeatherSystem* system) {
    if (!system) return;
    
    // In a real implementation, would apply vertex displacement to meshes
    // based on snow coverage and accumulation height
    LOG_TRACE("Applying snow displacement: coverage=%.2f, height=%.3f", 
              system->snow_coverage.global_coverage, system->snow_coverage.accumulation_height);
}

// Core weather system functions
void weather_system_init(WeatherSystem* system) {
    if (!system) return;
    
    memset(system, 0, sizeof(WeatherSystem));
    
    // Initialize time of day
    system->time_of_day.current_time = 12.0f;  // Start at noon
    system->time_of_day.day_count = 0;
    system->time_of_day.ambient_intensity = 1.0f;
    
    // Initialize atmosphere
    system->atmosphere.rayleigh_scattering = vec3(5.8e-6f, 13.5e-6f, 33.1e-6f);
    system->atmosphere.mie_scattering = vec3(2.0e-5f, 2.0e-5f, 2.0e-5f);
    system->atmosphere.rayleigh_scale = 8000.0f;
    system->atmosphere.mie_scale = 1200.0f;
    system->atmosphere.mie_g = 0.76f;
    system->atmosphere.atmosphere_height = 100000.0f;
    system->atmosphere.planet_radius = 6371000.0f;
    
    // Initialize weather state
    system->current_state = WEATHER_CLEAR;
    system->target_state = WEATHER_CLEAR;
    system->transition_progress = 0.0f;
    system->transition_duration = 10.0f;
    
    // Initialize wind
    system->global_wind = vec3(5.0f, 0.0f, 3.0f);
    system->wind_variation = 0.2f;
    system->wind_gust_strength = 0.5f;
    
    // Initialize lighting
    system->ambient_color = vec3(0.4f, 0.5f, 0.6f);
    system->fog_color = vec3(0.7f, 0.8f, 0.9f);
    system->fog_density = 0.0f;
    system->visibility_distance = 1000.0f;
    
    // Initialize audio
    system->rain_volume = 0.0f;
    system->thunder_volume = 0.0f;
    system->wind_volume = 0.1f;
    system->next_thunder_time = 0;
    
    // Initialize performance settings
    system->enable_volumetric_clouds = true;
    system->enable_high_quality_precipitation = true;
    system->particle_lod_level = 0;
    
    // Initialize sub-systems
    weather_init_volumetric_clouds(system);
    weather_init_rain_system(system);
    weather_init_snow_system(system);
    
    // Initialize surface systems
    system->surface_wetness.global_wetness = 0.0f;
    system->surface_wetness.puddle_intensity = 0.0f;
    system->surface_wetness.evaporation_rate = 0.001f;
    system->surface_wetness.absorption_rate = 0.01f;
    
    system->snow_coverage.global_coverage = 0.0f;
    system->snow_coverage.accumulation_height = 0.0f;
    system->snow_coverage.melting_rate = 0.0001f;
    system->snow_coverage.compaction_rate = 0.00001f;
    
    LOG_INFO("Weather system initialized");
}

void weather_system_shutdown(WeatherSystem* system) {
    if (!system) return;
    
    // Clean up precipitation zones
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        system->precipitation_zones[i].particle_count = 0;
    }
    system->precipitation_zone_count = 0;
    
    // Clean up cloud layers
    for (u32 i = 0; i < system->cloud_layer_count; i++) {
        system->cloud_layers[i].noise_texture_id = 0;
    }
    system->cloud_layer_count = 0;
    
    // Clean up lightning
    for (u32 i = 0; i < system->lightning_bolt_count; i++) {
        if (system->lightning_bolts[i].branch_points) {
            free(system->lightning_bolts[i].branch_points);
        }
    }
    system->lightning_bolt_count = 0;
    
    memset(system, 0, sizeof(WeatherSystem));
    LOG_INFO("Weather system shutdown");
}

void weather_system_update(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    u64 start_time = time_get_current_ms();
    
    // Update time of day
    weather_update_time_of_day(system, delta_time);
    
    // Update weather state transitions
    weather_update_state_transition(system, delta_time);
    
    // Update atmospheric conditions
    weather_update_sky_colors(system);
    weather_update_volumetric_clouds(system, delta_time);
    
    // Update precipitation
    weather_update_rain_particles(system, delta_time);
    weather_update_snow_particles(system, delta_time);
    
    // Update wind
    weather_update_wind_system(system, delta_time);
    
    // Update celestial bodies
    weather_update_moon_phase(system);
    weather_compute_star_brightness(system);
    
    // Update audio
    weather_update_dynamic_ambience(system);
    
    u64 end_time = time_get_current_ms();
    system->weather_update_time_ms += (end_time - start_time);
}

// Weather state management
void weather_set_weather_state(WeatherSystem* system, WeatherState state) {
    if (!system) return;
    
    if (system->current_state != state) {
        system->target_state = state;
        system->transition_progress = 0.0f;
        
        LOG_INFO("Weather state transition: %d -> %d", system->current_state, state);
    }
}

void weather_transition_to_state(WeatherSystem* system, WeatherState target_state, f32 duration) {
    if (!system) return;
    
    system->target_state = target_state;
    system->transition_duration = duration;
    system->transition_progress = 0.0f;
    
    LOG_INFO("Starting weather transition to state %d over %.1f seconds", target_state, duration);
}

void weather_update_state_transition(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    if (system->current_state != system->target_state) {
        system->transition_progress += delta_time / system->transition_duration;
        
        if (system->transition_progress >= 1.0f) {
            system->current_state = system->target_state;
            system->transition_progress = 0.0f;
            
            LOG_INFO("Weather transition complete: now in state %d", system->current_state);
        }
    }
}

WeatherState weather_get_current_state(WeatherSystem* system) {
    return system ? system->current_state : WEATHER_CLEAR;
}

// Utility functions
Vec3 weather_get_current_sun_color(WeatherSystem* system) {
    return system ? system->time_of_day.sun_color : vec3(1.0f, 1.0f, 1.0f);
}

Vec3 weather_get_current_moon_color(WeatherSystem* system) {
    return system ? system->time_of_day.moon_color : vec3(0.9f, 0.9f, 0.8f);
}

f32 weather_get_current_visibility(WeatherSystem* system) {
    if (!system) return 1000.0f;
    
    // Adjust visibility based on weather state
    switch (system->current_state) {
        case WEATHER_CLEAR:
            return 1000.0f;
        case WEATHER_CLOUDY:
            return 800.0f;
        case WEATHER_OVERCAST:
            return 600.0f;
        case WEATHER_FOG:
            return 100.0f;
        case WEATHER_HAZE:
            return 500.0f;
        case WEATHER_LIGHT_RAIN:
        case WEATHER_LIGHT_SNOW:
            return 700.0f;
        case WEATHER_HEAVY_RAIN:
        case WEATHER_HEAVY_SNOW:
            return 400.0f;
        case WEATHER_STORM:
        case WEATHER_BLIZZARD:
            return 200.0f;
        default:
            return 1000.0f;
    }
}

// Wind system (TASK_2023)
void weather_update_wind_system(WeatherSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Add some variation to wind
    f32 time_factor = (f32)time_get_current_ms() / 1000.0f;
    f32 variation = sinf(time_factor * 0.1f) * system->wind_variation;
    
    // Update wind with gusts
    Vec3 base_wind = vec3(5.0f, 0.0f, 3.0f);
    Vec3 gust_wind = vec3(
        sinf(time_factor * 0.3f) * system->wind_gust_strength,
        0.0f,
        cosf(time_factor * 0.2f) * system->wind_gust_strength
    );
    
    system->global_wind = vec3_add(base_wind, gust_wind);
    system->global_wind = vec3_add(system->global_wind, vec3_scale(system->global_wind, variation));
}

Vec3 weather_get_wind_at_position(WeatherSystem* system, Vec3 position) {
    if (!system) return vec3_zero();
    
    // Add position-based variation to wind
    f32 position_factor = sinf(position.x * 0.01f) * cosf(position.z * 0.01f);
    Vec3 local_wind = vec3_add(system->global_wind, 
                              vec3_scale(system->global_wind, position_factor * 0.2f));
    
    return local_wind;
}

// Audio system (TASK_2040)
void weather_update_dynamic_ambience(WeatherSystem* system) {
    if (!system) return;
    
    // Update rain volume based on intensity
    for (u32 i = 0; i < system->precipitation_zone_count; i++) {
        if (system->precipitation_zones[i].type == PRECIPITATION_RAIN) {
            system->rain_volume = system->precipitation_zones[i].intensity;
            break;
        }
    }
    
    // Update wind volume based on wind strength
    f32 wind_strength = vec3_length(system->global_wind);
    system->wind_volume = fminf(1.0f, wind_strength / 20.0f);
    
    // Update thunder volume for storms
    system->thunder_volume = (system->current_state == WEATHER_STORM) ? 0.8f : 0.0f;
}
