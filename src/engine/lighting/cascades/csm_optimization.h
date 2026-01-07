/*
 * csm_optimization.h
 * Optimization definitions
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CSM_OPTIMIZATION_H
#define LIGHTING_CSM_OPTIMIZATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum csm_update_strategy {
    CSM_UPDATE_STRATEGY_EVERY_FRAME,
    CSM_UPDATE_STRATEGY_INTERLEAVED,
    CSM_UPDATE_STRATEGY_STATIC_DISTANT
} csm_update_strategy_t;

/**
 * @brief Determine if a cascade needs updating this frame
 * 
 * @param cascade_index Index of the cascade
 * @param frame_count Current frame number
 * @param strategy Selected update strategy
 * @return true if update is required
 */
bool csm_opt_should_update_cascade(uint32_t cascade_index, uint32_t frame_count, csm_update_strategy_t strategy);

/**
 * @brief Calculate hysteresis value for smooth transitions
 */
float csm_opt_calculate_hysteresis(float current_val, float target_val, float lerp_factor);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CSM_OPTIMIZATION_H */
