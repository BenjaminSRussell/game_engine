/*
 * age_simulation.h
 * Material aging simulation API
 *
 * Part of the Age Weathering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef AGE_WEATHERING_AGE_SIMULATION_H
#define AGE_WEATHERING_AGE_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct age_simulation_handle {
    uint32_t id;
} age_simulation_handle_t;

typedef struct age_params {
    float time_years;           // Age in years
    float moisture_exposure;    // 0-1, likelihood of rust/moss
    float uv_exposure;          // 0-1, sun bleaching
    float physical_stress;      // 0-1, wear and tear factor
    
    vec3_t environment_color;   // Ambient environmental tint (dust/dirt)
} age_params_t;

typedef struct age_simulation_desc {
    uint32_t flags;
    void* user_data;
} age_simulation_desc_t;

typedef struct age_simulation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} age_simulation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int age_simulation_init(void);
void age_simulation_shutdown(void);

/* Lifecycle */
int age_simulation_create(age_simulation_handle_t* out_handle, const age_simulation_desc_t* desc);
void age_simulation_destroy(age_simulation_handle_t handle);

/* Operations */
int age_simulation_update(age_simulation_handle_t handle, const age_params_t* params);
bool age_simulation_is_valid(age_simulation_handle_t handle);

/**
 * Calculate weathering factors based on material age and environmental conditions.
 * Returns relative amounts (0-1) for various effects.
 */
void age_simulation_calculate_factors(age_simulation_handle_t handle, 
                                    float* out_rust, 
                                    float* out_dirt, 
                                    float* out_wear);

/* Statistics */
uint32_t age_simulation_get_count(void);
size_t age_simulation_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* AGE_WEATHERING_AGE_SIMULATION_H */
