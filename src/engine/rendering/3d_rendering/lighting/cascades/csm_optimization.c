/*
 * csm_optimization.c
 * optimization strategies for CSM
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "csm_optimization.h"
#include <math.h>

bool csm_opt_should_update_cascade(uint32_t cascade_index, uint32_t frame_count, csm_update_strategy_t strategy) {
    if (strategy == CSM_UPDATE_STRATEGY_EVERY_FRAME) return true;
    
    // Simple interleaved update:
    // Cascade 0: Every frame
    // Cascade 1: Every 2nd frame
    // Cascade 2: Every 4th frame
    // Cascade 3: Every 8th frame
    
    // If STATIC_DISTANT is set, we might not update far cascades at all unless camera moves significantly.
    
    if (cascade_index == 0) return true;
    
    uint32_t interval = 1 << cascade_index; // 2, 4, 8...
    return (frame_count % interval) == 0;
}

float csm_opt_calculate_hysteresis(float current_val, float target_val, float lerp_factor) {
    // Basic linear interpolation for smooth transitions
    return current_val + (target_val - current_val) * lerp_factor;
}
