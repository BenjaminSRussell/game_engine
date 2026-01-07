/*
 * lod_selector.c
 * Runtime LOD selection
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_selector.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

lod_selection_config_t lod_selection_get_default_config(void) {
    lod_selection_config_t config = {
        .global_bias = 1.0f,
        .hysteresis_distance = 0.1f,  // 10% distance change required
        .hysteresis_time = 0.5f,       // 0.5 second minimum between switches
        .prediction_weight = 0.3f,     // 30% weight on prediction
        .use_screen_space = true,
        .use_distance = true,
        .pixel_error_threshold = 2.0f, // 2 pixels max error
        .min_stable_frames = 3         // 3 frames minimum stability
    };
    return config;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

float lod_calculate_distance(const float camera_pos[3], const float object_pos[3]) {
    float dx = object_pos[0] - camera_pos[0];
    float dy = object_pos[1] - camera_pos[1];
    float dz = object_pos[2] - camera_pos[2];
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

float lod_calculate_screen_coverage(
    const lod_camera_t* camera,
    const float object_position[3],
    float object_radius
) {
    if (!camera) return 0.0f;
    
    // Calculate distance to object
    float distance = lod_calculate_distance(camera->position, object_position);
    if (distance < 0.001f) distance = 0.001f; // Avoid divide by zero
    
    // Project sphere to screen using perspective projection
    // Screen height of sphere = (2 * radius * focal_length) / distance
    // focal_length = viewport_height / (2 * tan(fov_y/2))
    
    float tan_half_fov = tanf(camera->fov_y * 0.5f);
    float screen_height_pixels = (object_radius * camera->viewport_height) / (distance * tan_half_fov);
    
    // Return as fraction of viewport height
    return screen_height_pixels / (float)camera->viewport_height;
}

float lod_calculate_projected_error(
    const lod_camera_t* camera,
    float distance,
    float geometric_error
) {
    if (!camera || distance < 0.001f) return 0.0f;
    
    // Project geometric error to screen space
    float tan_half_fov = tanf(camera->fov_y * 0.5f);
    float pixels = (geometric_error * camera->viewport_height) / (distance * tan_half_fov);
    
    return pixels;
}

/* ============================================================================
 * SIMPLE LOD SELECTION
 * ============================================================================ */

uint32_t lod_select(const lod_chain_t* chain, float screen_coverage, float bias) {
    if (!chain || chain->lod_count == 0) {
        return 0;
    }
    
    // Apply bias to screen coverage
    // bias > 1.0 = prefer higher quality (higher LODs selected)
    // bias < 1.0 = prefer performance (lower LODs selected)
    float adjusted_coverage = screen_coverage * bias;
    
    // Select LOD based on screen coverage thresholds
    // LODs are ordered from highest detail (0) to lowest detail (lod_count-1)
    for (uint32_t i = 0; i < chain->lod_count; i++) {
        if (adjusted_coverage >= chain->switch_distances[i]) {
            return i;
        }
    }
    
    // If coverage is below all thresholds, use lowest LOD
    return chain->lod_count - 1;
}

uint32_t lod_select_by_distance(
    const lod_chain_t* chain,
    float distance,
    const float* distance_thresholds
) {
    if (!chain || chain->lod_count == 0) return 0;
    
    // Select based on distance thresholds
    // Closer objects use higher detail LODs
    for (uint32_t i = 0; i < chain->lod_count - 1; i++) {
        if (distance < distance_thresholds[i]) {
            return i;
        }
    }
    
    return chain->lod_count - 1;
}

uint32_t lod_select_by_screen_size(
    const lod_chain_t* chain,
    float screen_size_pixels,
    const float* size_thresholds
) {
    if (!chain || chain->lod_count == 0) return 0;
    
    // Select based on screen size
    // Larger on-screen size uses higher detail
    for (uint32_t i = 0; i < chain->lod_count - 1; i++) {
        if (screen_size_pixels >= size_thresholds[i]) {
            return i;
        }
    }
    
    return chain->lod_count - 1;
}

/* ============================================================================
 * ADVANCED LOD SELECTION
 * ============================================================================ */

lod_selection_result_t lod_select_advanced(
    const lod_chain_t* chain,
    const lod_camera_t* camera,
    const float object_position[3],
    float object_radius,
    const lod_selection_config_t* config,
    lod_selection_state_t* state,
    float delta_time
) {
    lod_selection_result_t result = {0};
    
    if (!chain || !camera || !config || !state) {
        return result;
    }
    
    // 1. Calculate metrics
    float distance = lod_calculate_distance(camera->position, object_position);
    float screen_coverage = 0.0f;
    float projected_error = 0.0f;
    
    if (config->use_screen_space) {
        screen_coverage = lod_calculate_screen_coverage(camera, object_position, object_radius);
    }
    
    // 2. Apply velocity prediction for frame N+1
    if (config->prediction_weight > 0.0f && camera->velocity[0] != 0.0f) {
        // Predict future distance based on velocity
        float vx = camera->velocity[0];
        float vy = camera->velocity[1];
        float vz = camera->velocity[2];
        
        // Simple prediction: where will camera be in 1 frame at 60fps?
        float predict_time = 1.0f / 60.0f;
        float predicted_cam_pos[3] = {
            camera->position[0] + vx * predict_time,
            camera->position[1] + vy * predict_time,
            camera->position[2] + vz * predict_time
        };
        
        float predicted_distance = lod_calculate_distance(predicted_cam_pos, object_position);
        
        // Blend current and predicted distance
        distance = distance * (1.0f - config->prediction_weight) + 
                   predicted_distance * config->prediction_weight;
    }
    
    // 3. Select ideal LOD based on screen space or distance
    uint32_t ideal_lod = 0;
    
    if (config->use_screen_space && screen_coverage > 0.0f) {
        // Screen-space based selection with bias
        ideal_lod = lod_select(chain, screen_coverage, config->global_bias);
    } else if (config->use_distance) {
        // Fallback to distance-based
        // Create distance thresholds from switch distances (heuristic)
        float dist_thresholds[8];
        for (uint32_t i = 0; i < chain->lod_count; i++) {
            dist_thresholds[i] = object_radius / (chain->switch_distances[i] + 0.001f);
        }
        ideal_lod = lod_select_by_distance(chain, distance, dist_thresholds);
    }
    
    // 4. Calculate projected error for selected LOD
    if (ideal_lod < chain->lod_count) {
        projected_error = lod_calculate_projected_error(
            camera, 
            distance, 
            chain->geometric_errors[ideal_lod]
        );
    }
    
    // 5. Apply hysteresis
    bool should_switch = false;
    
    // Distance hysteresis: require significant distance change
    if (state->last_distance > 0.0f) {
        float distance_change = fabsf(distance - state->last_distance) / state->last_distance;
        if (distance_change < config->hysteresis_distance) {
            // Not enough change, keep current LOD
            ideal_lod = state->current_lod;
        }
    }
    
    // Time hysteresis: require minimum time between switches
    if (state->hysteresis_timer > 0.0f) {
        state->hysteresis_timer -= delta_time;
        ideal_lod = state->current_lod; // Force current LOD during cooldown
    }
    
    // Frame stability: require minimum stable frames
    if (ideal_lod == state->target_lod) {
        state->frames_stable++;
    } else {
        state->frames_stable = 0;
        state->target_lod = ideal_lod;
    }
    
    if (state->frames_stable >= config->min_stable_frames) {
        should_switch = (ideal_lod != state->current_lod);
        
        if (should_switch) {
            state->current_lod = ideal_lod;
            state->hysteresis_timer = config->hysteresis_time;
            state->frames_stable = 0;
        }
    }
    
    // 6. Update state
    state->last_distance = distance;
    state->last_screen_size = screen_coverage;
    
    // 7. Fill result
    result.selected_lod = state->current_lod;
    result.distance = distance;
    result.screen_coverage = screen_coverage;
    result.projected_error = projected_error;
    result.should_transition = should_switch;
    
    return result;
}

/* End of lod_selector.c */
