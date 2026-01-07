/*
 * aerial_perspective.c
 * Aerial perspective / Distant fog
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "environment/atmosphere/core/atmosphere/sky/aerial_perspective.h"
#include "include/math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct aerial_perspective_params {
    float global_density;
    float start_distance;
    vec3_t fog_color;
    
    // LUT references
    uint32_t lut_texture_id;
    bool use_lut;
} aerial_perspective_params_t;

static aerial_perspective_params_t g_aerial_params = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_aerial_perspective_init(void) {
    g_aerial_params.global_density = 1.0f;
    g_aerial_params.start_distance = 10.0f;
    g_aerial_params.fog_color = vec3_set(0.5f, 0.6f, 0.7f); // Default bluish
    g_aerial_params.use_lut = false;
    
    return 0;
}

void atmosphere_aerial_perspective_shutdown(void) {
    // Cleanup
}

void atmosphere_aerial_perspective_set_lut(uint32_t texture_id) {
    g_aerial_params.lut_texture_id = texture_id;
    g_aerial_params.use_lut = true;
}

// Apply aerial perspective to a fragment
// This simulates the shader logic on CPU or setup
vec3_t atmosphere_apply_aerial_perspective(vec3_t original_color, vec3_t world_pos, vec3_t cam_pos) {
    vec3_t view_vec = vec3_sub(world_pos, cam_pos);
    float distance = sqrtf(vec3_dot(view_vec, view_vec));
    
    // Simple exponential squared fog (if no LUT)
    // f = exp(-(dist * density)^2)
    
    float fog_dist = distance - g_aerial_params.start_distance;
    if (fog_dist < 0.0f) fog_dist = 0.0f;
    
    float fog_factor;
    
    if (g_aerial_params.use_lut) {
        // Ideal: Sample 3D texture at (uv, depth)
        // Simulated result:
        fog_factor = 1.0f / (1.0f + fog_dist * 0.0001f); 
        // Real logic would depend on view angle and sun position implicitly via LUT
    } else {
        // Analytical fallback
        float d = fog_dist * 0.00005f * g_aerial_params.global_density; // Scale for typical scene units
        fog_factor = expf(-(d * d));
    }
    
    // Blend: color * fog_factor + fog_color * (1 - fog_factor)
    vec3_t result;
    result.x = original_color.x * fog_factor + g_aerial_params.fog_color.x * (1.0f - fog_factor);
    result.y = original_color.y * fog_factor + g_aerial_params.fog_color.y * (1.0f - fog_factor);
    result.z = original_color.z * fog_factor + g_aerial_params.fog_color.z * (1.0f - fog_factor);
    
    return result;
}

void atmosphere_aerial_perspective_set_color(vec3_t color) {
    g_aerial_params.fog_color = color;
}
