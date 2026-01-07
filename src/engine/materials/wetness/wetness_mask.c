/*
 * wetness_mask.c
 * Wetness mask implementation
 *
 * Part of the Wetness System
 * Advanced 3D Rendering Engine
 */

#include "materials/wetness/wetness_mask.h"
#include "math/math.h"
#include <math/math.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float wetness_calculate_mask(const wetness_params_t* params, vec3_t normal, float cavity, float humidity) {
    // Top-facing surfaces get wetter from rain
    float up_factor = saturate(normal.y);
    
    // Cavities hold water longer
    float cavity_factor = saturate(cavity * 1.5f);
    
    float rain_wetness = params->global_intensity * (up_factor * 0.7f + cavity_factor * 0.3f);
    float humidity_wetness = humidity * params->absorption;
    
    return saturate(rain_wetness + humidity_wetness - params->drying_factor);
}

vec3_t wetness_apply_darkening(vec3_t albedo, float wetness, float absorption) {
    // Materials darken when wet due to light being trapped in the water film
    // Porous materials darken more
    float darken_factor = lerp(1.0f, 0.5f, wetness * (0.5f + absorption * 0.5f));
    return vec3_multiply_scalar(albedo, darken_factor);
}
