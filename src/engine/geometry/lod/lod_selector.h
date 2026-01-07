/*
 * lod_selector.h
 * Runtime LOD selection
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_SELECTOR_H
#define GEOMETRY_LOD_SELECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/lod/lod_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Camera data for LOD selection
typedef struct lod_camera {
    float position[3];          // World position
    float velocity[3];          // Camera velocity for prediction
    float view_matrix[16];      // View matrix
    float proj_matrix[16];      // Projection matrix
    float fov_y;                // Vertical field of view (radians)
    float aspect_ratio;         // Aspect ratio
    float near_plane;           // Near clipping plane
    float far_plane;            // Far clipping plane
    uint32_t viewport_width;    // Viewport width in pixels
    uint32_t viewport_height;   // Viewport height in pixels
} lod_camera_t;

// Per-object LOD selection state
typedef struct lod_selection_state {
    uint32_t current_lod;       // Currently selected LOD
    uint32_t target_lod;        // Target LOD (for transitions)
    float last_distance;        // Last measured distance
    float last_screen_size;     // Last measured screen size
    float hysteresis_timer;     // Time remaining in hysteresis window
    uint32_t frames_stable;     // Frames at current LOD
} lod_selection_state_t;

// LOD selection configuration
typedef struct lod_selection_config {
    float global_bias;          // Global quality bias [0.5 - 2.0]
    float hysteresis_distance;  // Distance hysteresis (fraction)
    float hysteresis_time;      // Time hysteresis (seconds)
    float prediction_weight;    // Weight for velocity prediction [0-1]
    bool use_screen_space;      // Use screen-space metrics
    bool use_distance;          // Use distance-based metrics
    float pixel_error_threshold; // Max acceptable pixel error
    uint32_t min_stable_frames; // Min frames before LOD change
} lod_selection_config_t;

// LOD selection result
typedef struct lod_selection_result {
    uint32_t selected_lod;
    float distance;
    float screen_coverage;
    float projected_error;
    bool should_transition;
} lod_selection_result_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Configuration */
lod_selection_config_t lod_selection_get_default_config(void);

/* Simple Selection */
// Select appropriate LOD based on screen coverage and bias
uint32_t lod_select(const lod_chain_t* chain, float screen_coverage, float bias);

/* Advanced Selection */
// Select LOD with camera and object data
lod_selection_result_t lod_select_advanced(
    const lod_chain_t* chain,
    const lod_camera_t* camera,
    const float object_position[3],
    float object_radius,
    const lod_selection_config_t* config,
    lod_selection_state_t* state,
    float delta_time
);

/* Distance-Based Selection */
uint32_t lod_select_by_distance(
    const lod_chain_t* chain,
    float distance,
    const float* distance_thresholds
);

/* Screen-Space Selection */
uint32_t lod_select_by_screen_size(
    const lod_chain_t* chain,
    float screen_size_pixels,
    const float* size_thresholds
);

/* Utilities */
// Calculate distance from camera to object
float lod_calculate_distance(const float camera_pos[3], const float object_pos[3]);

// Calculate screen-space coverage
float lod_calculate_screen_coverage(
    const lod_camera_t* camera,
    const float object_position[3],
    float object_radius
);

// Calculate projected error in pixels
float lod_calculate_projected_error(
    const lod_camera_t* camera,
    float distance,
    float geometric_error
);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_SELECTOR_H */
