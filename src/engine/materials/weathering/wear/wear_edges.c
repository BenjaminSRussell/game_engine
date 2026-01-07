/*
 * wear_edges.c
 * Curvature-based edge wear implementation
 *
 * Part of the Wear & Tear subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/weathering/wear/wear_edges.h"
#include "math/math.h"
#include <math/math.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float wear_calculate_edge_mask(const wear_params_t* params, float curvature, vec3_t world_pos) {
    // High curvature (convex) edges wear out first
    float base_wear = saturate(curvature * params->amount * 2.0f);
    
    // Add noise to make it look non-uniform (chipped/rubbed)
    float noise = (sinf(world_pos.x * 20.0f) + cosf(world_pos.z * 21.0f)) * 0.1f + 0.9f;
    
    float mask = powf(base_wear * noise, params->contrast);
    return saturate(mask * params->intensity);
}

void wear_apply_to_properties(float mask, float* albedo_factor, float* roughness) {
    if (!albedo_factor || !roughness) return;
    
    // Wear typically reveals brighter base material or bare metal
    *albedo_factor = 1.0f + mask * 0.3f;
    
    // Worn areas usually get polished (smoother)
    *roughness = lerp(*roughness, 0.15f, mask);
}
