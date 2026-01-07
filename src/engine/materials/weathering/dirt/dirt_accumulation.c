/*
 * dirt_accumulation.c
 * Dirt in crevices implementation
 *
 * Part of the Dirt & Grunge subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/weathering/dirt/dirt_accumulation.h"
#include "math/math.h"
#include <math/math.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float dirt_calculate_mask(const dirt_params_t* params, float cavity, vec3_t normal, vec3_t world_pos) {
    // Dirt naturally collects in cavities (recessed areas)
    float cavity_factor = saturate(cavity * params->cavity_bias * 2.0f);
    
    // Gravity effect: Dirt settles on top-facing surfaces
    float up_factor = saturate(normal.y);
    float gravity_factor = lerp(1.0f, up_factor, params->gravity_weight);
    
    // Spatial noise for variation
    float noise = (sinf(world_pos.x * 5.0f) * cosf(world_pos.z * 5.0f)) * 0.1f + 0.9f;
    
    float mask = params->intensity * cavity_factor * gravity_factor * noise;
    return saturate(mask);
}

vec3_t dirt_apply_to_albedo(vec3_t albedo, float mask, vec3_t dirt_color) {
    // Dirt usually darkens and tints the albedo
    return vec3_lerp(albedo, dirt_color, mask);
}
