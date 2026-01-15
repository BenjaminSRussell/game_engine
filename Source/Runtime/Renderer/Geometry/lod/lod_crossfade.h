/*
 * lod_crossfade.h
 * LOD transition blending
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_CROSSFADE_H
#define GEOMETRY_LOD_CROSSFADE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// LOD transition state
typedef struct lod_transition_state {
    uint32_t current_lod;         // Current LOD index
    uint32_t target_lod;          // Target LOD index
    float transition_t;           // Transition parameter [0.0, 1.0]
    float hysteresis_timer;       // Timer to prevent rapid switching
    float last_screen_coverage;   // Last measured screen coverage
} lod_transition_state_t;

// Crossfade configuration
typedef struct lod_crossfade_config {
    float transition_speed;       // Speed of transition (units per second)
    float hysteresis_time;        // Minimum time between LOD switches (seconds)
    float hysteresis_threshold;   // Coverage change threshold to trigger switch
} lod_crossfade_config_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Crossfade Management */
// Initialize transition state
void lod_transition_init(lod_transition_state_t* state);

// Update transition state based on screen coverage
void lod_crossfade_update(lod_transition_state_t* state, 
                          const lod_crossfade_config_t* config,
                          uint32_t selected_lod, 
                          float screen_coverage,
                          float delta_time);

// Get current blend factor for dithering
float lod_get_blend_factor(const lod_transition_state_t* state);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_CROSSFADE_H */
