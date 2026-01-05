/*
 * puddle_rendering.c
 * Puddle rendering implementation
 *
 * Part of the Wetness System
 * Advanced 3D Rendering Engine
 */

#include "puddle_rendering.h"
#include "../../../include/math/math.h"
#include <math.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float puddle_calculate_mask(const puddle_params_t* params, float world_height, float noise) {
    // Puddles form where the ground is below the water level
    // We add some noise to the ground height to make puddles irregular
    float ground_height = world_height + noise * 0.1f;
    float delta = params->water_level - ground_height;
    
    if (delta <= 0.0f) return 0.0f;
    
    // Smooth transition at the edges
    return smoothstep(0.0f, params->edge_softness + 0.001f, delta);
}

vec3_t puddle_get_ripple_normal(vec3_t world_pos, float time) {
    // Procedural raindrops/ripples
    float x = world_pos.x * 10.0f + time;
    float z = world_pos.z * 10.0f + time * 1.1f;
    
    float nx = sinf(x) * 0.05f;
    float nz = sinf(z) * 0.05f;
    
    vec3_t normal = {nx, 1.0f, nz};
    return vec3_normalize(normal);
}
