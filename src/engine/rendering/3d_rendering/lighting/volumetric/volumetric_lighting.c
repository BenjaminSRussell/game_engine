/*
 * volumetric_lighting.c
 * God rays, light shafts and volumetric lighting effects
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "volumetric_lighting.h"
#include "../../math/vec3.h"
#include "../../math/vec2.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_VOLUMETRIC_MAX_LIGHTS 16
#define MAX_SAMPLES 128

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct volumetric_light_source {
    vec3_t position;
    vec3_t color;
    float intensity;
    float range;
    float cone_angle; // For spotlights
    vec3_t direction;
    bool cast_shadows;
} volumetric_light_source_t;

typedef struct lighting_volumetric_context {
    volumetric_light_source_t lights[LIGHTING_VOLUMETRIC_MAX_LIGHTS];
    uint32_t active_light_count;
    
    // Light shaft settings
    float exposure;
    float decay;
    float density;
    float weight;
    int samples;
    
    bool initialized;
    bool dirty;
} lighting_volumetric_context_t;

static lighting_volumetric_context_t g_volumetric_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Pseudo-random number generator for dithering
static float random_float(float seed) {
    int n = (int)(seed * 10000.0f);
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_volumetric_init(void) {
    if (g_volumetric_ctx.initialized) return 0;
    
    memset(&g_volumetric_ctx, 0, sizeof(g_volumetric_ctx));
    
    // Default settings for good shafts
    g_volumetric_ctx.exposure = 0.0034f;
    g_volumetric_ctx.decay = 1.0f;
    g_volumetric_ctx.density = 0.84f;
    g_volumetric_ctx.weight = 5.65f;
    g_volumetric_ctx.samples = 100;
    
    g_volumetric_ctx.initialized = true;
    return 0;
}

void lighting_volumetric_shutdown(void) {
    g_volumetric_ctx.initialized = false;
    g_volumetric_ctx.active_light_count = 0;
}

// Compute screen-space light shafts (God Rays)
// Performs a radial blur from the light source position on screen
void lighting_volumetric_compute_shafts(
    vec2_t light_screen_pos,
    vec2_t* uv_coords,
    vec3_t* out_color,
    const float* occlusion_texture, // Sampled occlusion (e.g., from depth or shadow map)
    int width,
    int height
) {
    if (!uv_coords || !out_color || !occlusion_texture) return;
    
    // Radial blur parameters
    float density = g_volumetric_ctx.density;
    float weight = g_volumetric_ctx.weight;
    float decay = g_volumetric_ctx.decay;
    float exposure = g_volumetric_ctx.exposure;
    int num_samples = g_volumetric_ctx.samples;
    
    vec2_t delta_tex_coord_vec;
    // delta = (uv - light_pos)
    // we step towards the light
    delta_tex_coord_vec.x = uv_coords->x - light_screen_pos.x;
    delta_tex_coord_vec.y = uv_coords->y - light_screen_pos.y;
    
    // Divide by num_samples * density for step size
    float step_factor = 1.0f / ((float)num_samples * density);
    delta_tex_coord_vec.x *= step_factor;
    delta_tex_coord_vec.y *= step_factor;
    
    vec3_t color = vec3_zero();
    float illumination_decay = 1.0f;
    vec2_t current_uv = *uv_coords;
    
    for (int i = 0; i < num_samples; i++) {
        // Step UV
        current_uv.x -= delta_tex_coord_vec.x;
        current_uv.y -= delta_tex_coord_vec.y;
        
        // Sample occlusion (simulate texture sample)
        // int x = clamp ... 
        // float sample = occlusion_texture[...];
        
        // For C simulation, we skip texture sample
        float sample = 1.0f; // Placeholder: fully visible
        
        sample *= illumination_decay * weight;
        color.x += sample;
        color.y += sample;
        color.z += sample;
        
        illumination_decay *= decay;
    }
    
    // Apply exposure
    out_color->x = color.x * exposure;
    out_color->y = color.y * exposure;
    out_color->z = color.z * exposure;
}

// Add a light source to contribute to volumetric fog
int lighting_volumetric_add_light(
    vec3_t position,
    vec3_t color,
    float intensity,
    float range,
    bool cast_shadows
) {
    if (g_volumetric_ctx.active_light_count >= LIGHTING_VOLUMETRIC_MAX_LIGHTS) return -1;
    
    volumetric_light_source_t* light = &g_volumetric_ctx.lights[g_volumetric_ctx.active_light_count++];
    light->position = position;
    light->color = color;
    light->intensity = intensity;
    light->range = range;
    light->cast_shadows = cast_shadows;
    
    return g_volumetric_ctx.active_light_count - 1;
}

void lighting_volumetric_update_settings(float density, float weight, float decay, float exposure) {
    g_volumetric_ctx.density = density;
    g_volumetric_ctx.weight = weight;
    g_volumetric_ctx.decay = decay;
    g_volumetric_ctx.exposure = exposure;
    g_volumetric_ctx.dirty = true;
}

void lighting_volumetric_clear_lights(void) {
    g_volumetric_ctx.active_light_count = 0;
}
