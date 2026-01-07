/*
 * lod_crossfade.c
 * LOD transition blending
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_crossfade.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

void lod_transition_init(lod_transition_state_t* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(lod_transition_state_t));
    state->current_lod = 0;
    state->target_lod = 0;
    state->transition_t = 1.0f; // Fully at current LOD
}

/* ============================================================================
 * CROSSFADE UPDATE
 * ============================================================================ */

void lod_crossfade_update(lod_transition_state_t* state,
                         const lod_crossfade_config_t* config,
                         uint32_t selected_lod,
                         float screen_coverage,
                         float delta_time) {
    if (!state || !config) return;
    
    // Check if screen coverage changed significantly (hysteresis)
    float coverage_change = fabsf(screen_coverage - state->last_screen_coverage);
    bool significant_change = coverage_change >= config->hysteresis_threshold;
    
    // Update hysteresis timer
    if (state->hysteresis_timer > 0.0f) {
        state->hysteresis_timer -= delta_time;
    }
    
    // Determine if we should start a new transition
    bool should_transition = (selected_lod != state->target_lod) &&
                            (state->hysteresis_timer <= 0.0f) &&
                            significant_change;
    
    if (should_transition) {
        // Start new transition
        state->current_lod = state->target_lod; // Previous target becomes current
        state->target_lod = selected_lod;
        state->transition_t = 0.0f; // Start transition
        state->hysteresis_timer = config->hysteresis_time;
    }
    
    // Update transition parameter
    if (state->transition_t < 1.0f) {
        // Advance transition
        state->transition_t += config->transition_speed * delta_time;
        
        if (state->transition_t >= 1.0f) {
            state->transition_t = 1.0f;
            // Transition complete
            state->current_lod = state->target_lod;
        }
    }
    
    state->last_screen_coverage = screen_coverage;
}

/* ============================================================================
 * BLEND FACTOR
 * ============================================================================ */

float lod_get_blend_factor(const lod_transition_state_t* state) {
    if (!state) return 1.0f;
    
    // Apply smooth easing curve (smoothstep)
    float t = state->transition_t;
    
    // Smoothstep: 3t^2 - 2t^3
    float smooth_t = t * t * (3.0f - 2.0f * t);
    
    return smooth_t;
}

/* ============================================================================
 * ADVANCED BLEND CURVES
 * ============================================================================ */

float lod_blend_curve_linear(float t) {
    return t;
}

float lod_blend_curve_smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float lod_blend_curve_smootherstep(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lod_blend_curve_ease_in_quad(float t) {
    return t * t;
}

float lod_blend_curve_ease_out_quad(float t) {
    return t * (2.0f - t);
}

float lod_blend_curve_ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

/* End of lod_crossfade.c */
