#include "environment/skybox.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * Skybox Implementation
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Skybox {
    SkyboxConfig config;
};

// Create skybox
Skybox* skybox_create(void) {
    Skybox* skybox = calloc(1, sizeof(Skybox));
    if (!skybox) return NULL;
    
    // Default configuration
    skybox->config.type = SKYBOX_PROCEDURAL;
    skybox->config.sky_model = SKY_MODEL_HOSEK_WILKIE;
    
    // Default sun direction (noon)
    skybox->config.sun_direction[0] = 0.0f;
    skybox->config.sun_direction[1] = 1.0f;
    skybox->config.sun_direction[2] = 0.0f;
    skybox->config.sun_intensity = 1.0f;
    
    skybox->config.turbidity = 2.0f;
    skybox->config.ground_albedo = 0.3f;
    
    skybox->config.rotation = 0.0f;
    skybox->config.exposure = 1.0f;
    skybox->config.tint[0] = 1.0f;
    skybox->config.tint[1] = 1.0f;
    skybox->config.tint[2] = 1.0f;
    
    skybox->config.show_stars = true;
    skybox->config.star_intensity = 0.5f;
    
    skybox->config.show_moon = true;
    skybox->config.moon_phase = 0.5f; // Half moon
    
    printf("[Skybox] Created with procedural sky\n");
    return skybox;
}

// Destroy skybox
void skybox_destroy(Skybox* skybox) {
    if (!skybox) return;
    free(skybox);
    printf("[Skybox] Destroyed\n");
}

// Configuration
void skybox_set_type(Skybox* skybox, SkyboxType type) {
    if (!skybox) return;
    skybox->config.type = type;
    printf("[Skybox] Set type: %d\n", type);
}

void skybox_set_cubemap(Skybox* skybox, uint32_t texture_id) {
    if (!skybox) return;
    skybox->config.type = SKYBOX_CUBEMAP;
    skybox->config.cubemap_texture = texture_id;
}

void skybox_set_hdri(Skybox* skybox, uint32_t texture_id) {
    if (!skybox) return;
    skybox->config.type = SKYBOX_HDRI;
    skybox->config.hdri_texture = texture_id;
}

void skybox_set_procedural(Skybox* skybox, SkyModel model) {
    if (!skybox) return;
    skybox->config.type = SKYBOX_PROCEDURAL;
    skybox->config.sky_model = model;
}

// Procedural sky
void skybox_set_sun_direction(Skybox* skybox, float x, float y, float z) {
    if (!skybox) return;
    
    // Normalize
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 1e-6f) {
        skybox->config.sun_direction[0] = x / len;
        skybox->config.sun_direction[1] = y / len;
        skybox->config.sun_direction[2] = z / len;
    }
}

void skybox_set_sun_intensity(Skybox* skybox, float intensity) {
    if (!skybox) return;
    skybox->config.sun_intensity = intensity;
}

void skybox_set_turbidity(Skybox* skybox, float turbidity) {
    if (!skybox) return;
    // Clamp to valid range (1-10)
    skybox->config.turbidity = turbidity < 1.0f ? 1.0f : (turbidity > 10.0f ? 10.0f : turbidity);
}

// Common settings
void skybox_set_rotation(Skybox* skybox, float rotation) {
    if (!skybox) return;
    skybox->config.rotation = rotation;
}

void skybox_set_exposure(Skybox* skybox, float exposure) {
    if (!skybox) return;
    skybox->config.exposure = exposure;
}

void skybox_set_tint(Skybox* skybox, float r, float g, float b) {
    if (!skybox) return;
    skybox->config.tint[0] = r;
    skybox->config.tint[1] = g;
    skybox->config.tint[2] = b;
}

// Stars and moon
void skybox_enable_stars(Skybox* skybox, bool enabled) {
    if (!skybox) return;
    skybox->config.show_stars = enabled;
}

void skybox_set_star_texture(Skybox* skybox, uint32_t texture_id) {
    if (!skybox) return;
    skybox->config.star_texture = texture_id;
}

void skybox_enable_moon(Skybox* skybox, bool enabled) {
    if (!skybox) return;
    skybox->config.show_moon = enabled;
}

void skybox_set_moon_direction(Skybox* skybox, float x, float y, float z) {
    if (!skybox) return;
    
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 1e-6f) {
        skybox->config.moon_direction[0] = x / len;
        skybox->config.moon_direction[1] = y / len;
        skybox->config.moon_direction[2] = z / len;
    }
}

void skybox_set_moon_phase(Skybox* skybox, float phase) {
    if (!skybox) return;
    skybox->config.moon_phase = phase < 0.0f ? 0.0f : (phase > 1.0f ? 1.0f : phase);
}

// Time of day helper
void skybox_set_time_of_day(Skybox* skybox, float time_hours) {
    if (!skybox) return;
    
    // Convert hours (0-24) to angle
    float angle = (time_hours / 24.0f) * 2.0f * M_PI;
    
    // Sun moves in arc across sky
    // At 6am: sun at horizon (east)
    // At 12pm: sun at zenith
    // At 6pm: sun at horizon (west)
    float sun_elevation = sinf(angle - M_PI / 2.0f);
    float sun_azimuth = angle;
    
    skybox->config.sun_direction[0] = cosf(sun_azimuth) * cosf(sun_elevation);
    skybox->config.sun_direction[1] = sinf(sun_elevation);
    skybox->config.sun_direction[2] = sinf(sun_azimuth) * cosf(sun_elevation);
    
    // Moon is opposite to sun
    skybox->config.moon_direction[0] = -skybox->config.sun_direction[0];
    skybox->config.moon_direction[1] = -skybox->config.sun_direction[1];
    skybox->config.moon_direction[2] = -skybox->config.sun_direction[2];
    
    // Adjust star visibility based on time
    bool is_night = sun_elevation < 0.0f;
    skybox->config.star_intensity = is_night ? 1.0f : 0.0f;
    
    printf("[Skybox] Set time of day: %.2f hours\n", time_hours);
}

// Rendering (stub - would integrate with actual renderer)
void skybox_render(const Skybox* skybox, const float* view_matrix, const float* projection_matrix) {
    if (!skybox) return;
    
    // TODO: Actual rendering would happen here
    // - Bind skybox shader
    // - Upload config uniforms
    // - Render fullscreen quad or skybox cube
}

// Get config
const SkyboxConfig* skybox_get_config(const Skybox* skybox) {
    return skybox ? &skybox->config : NULL;
}
