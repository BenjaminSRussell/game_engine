#ifndef SKYBOX_H
#define SKYBOX_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Skybox Rendering System
 * Supports cubemap, HDRI, and procedural skies
 */

// Skybox type
typedef enum {
    SKYBOX_CUBEMAP,      // 6-sided cubemap
    SKYBOX_HDRI,         // Equirectangular HDRI
    SKYBOX_PROCEDURAL    // Procedural sky (Preetham/Hosek-Wilkie)
} SkyboxType;

// Procedural sky model
typedef enum {
    SKY_MODEL_PREETHAM,
    SKY_MODEL_HOSEK_WILKIE
} SkyModel;

// Skybox configuration
typedef struct {
    SkyboxType type;
    
    // Cubemap
    uint32_t cubemap_texture;
    
    // HDRI
    uint32_t hdri_texture;
    
    // Procedural sky
    SkyModel sky_model;
    float sun_direction[3];
    float sun_intensity;
    float turbidity;        // Atmospheric haze (1-10)
    float ground_albedo;
    
    // Common settings
    float rotation;         // Y-axis rotation in radians
    float exposure;
    float tint[3];         // Color tint
    
    // Stars
    bool show_stars;
    uint32_t star_texture;
    float star_intensity;
    
    // Moon
    bool show_moon;
    float moon_direction[3];
    float moon_phase;      // 0-1 (new to full)
    uint32_t moon_texture;
} SkyboxConfig;

// Skybox system
typedef struct Skybox Skybox;

// Initialization
Skybox* skybox_create(void);
void skybox_destroy(Skybox* skybox);

// Configuration
void skybox_set_type(Skybox* skybox, SkyboxType type);
void skybox_set_cubemap(Skybox* skybox, uint32_t texture_id);
void skybox_set_hdri(Skybox* skybox, uint32_t texture_id);
void skybox_set_procedural(Skybox* skybox, SkyModel model);

// Procedural sky parameters
void skybox_set_sun_direction(Skybox* skybox, float x, float y, float z);
void skybox_set_sun_intensity(Skybox* skybox, float intensity);
void skybox_set_turbidity(Skybox* skybox, float turbidity);

// Common settings
void skybox_set_rotation(Skybox* skybox, float rotation);
void skybox_set_exposure(Skybox* skybox, float exposure);
void skybox_set_tint(Skybox* skybox, float r, float g, float b);

// Stars and moon
void skybox_enable_stars(Skybox* skybox, bool enabled);
void skybox_set_star_texture(Skybox* skybox, uint32_t texture_id);
void skybox_enable_moon(Skybox* skybox, bool enabled);
void skybox_set_moon_direction(Skybox* skybox, float x, float y, float z);
void skybox_set_moon_phase(Skybox* skybox, float phase);

// Time of day helper
void skybox_set_time_of_day(Skybox* skybox, float time_hours);

// Rendering
void skybox_render(const Skybox* skybox, const float* view_matrix, const float* projection_matrix);

// Get config (for shaders)
const SkyboxConfig* skybox_get_config(const Skybox* skybox);

#endif // SKYBOX_H
