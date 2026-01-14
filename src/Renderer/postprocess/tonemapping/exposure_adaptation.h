/*
 * exposure_adaptation.h
 * Auto exposure
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_EXPOSURE_ADAPTATION_H
#define POSTPROCESSING_EXPOSURE_ADAPTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_exposure_adaptation_handle {
    uint32_t id;
} postprocessing_exposure_adaptation_handle_t;

typedef struct exposure_params {
    float min_exposure;     // Minimum allowed exposure (EV or multiplier buffer)
    float max_exposure;     // Maximum allowed exposure
    float target_luminance; // Key value (usually 0.18 for middle gray)
    float adaptation_speed; // Speed of adaptation (tau)
    bool use_ev;            // If true, min/max are in EV units
    
    // Manual override
    bool manual_override;
    float manual_exposure;
    
    // History
    bool use_history;
    uint32_t history_size;  // Max 64
} exposure_params_t;

typedef struct postprocessing_exposure_adaptation_desc {
    uint32_t flags;
    exposure_params_t initial_params;
    void* user_data;
} postprocessing_exposure_adaptation_desc_t;

typedef struct postprocessing_exposure_adaptation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    exposure_params_t current_params;
    float current_exposure;
} postprocessing_exposure_adaptation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_exposure_adaptation_init(void);
void postprocessing_exposure_adaptation_shutdown(void);

/* Lifecycle */
int postprocessing_exposure_adaptation_create(postprocessing_exposure_adaptation_handle_t* out_handle, const postprocessing_exposure_adaptation_desc_t* desc);
void postprocessing_exposure_adaptation_destroy(postprocessing_exposure_adaptation_handle_t handle);

/* Operations */
int postprocessing_exposure_adaptation_update(postprocessing_exposure_adaptation_handle_t handle, const void* data, size_t size);
void postprocessing_exposure_adaptation_set_params(postprocessing_exposure_adaptation_handle_t handle, const exposure_params_t* params);
bool postprocessing_exposure_adaptation_is_valid(postprocessing_exposure_adaptation_handle_t handle);
int postprocessing_exposure_adaptation_get_info(postprocessing_exposure_adaptation_handle_t handle, postprocessing_exposure_adaptation_info_t* out_info);
void postprocessing_exposure_adaptation_mark_dirty(postprocessing_exposure_adaptation_handle_t handle);
int postprocessing_exposure_adaptation_process_pending(void);

/* Algorithm */
// Computes new exposure based on average luminance and time delta
float postprocessing_exposure_compute_target(const exposure_params_t* params, float avg_luminance);
float postprocessing_exposure_adapt(const exposure_params_t* params, float current_exposure, float target_exposure, float dt);

/* Manual Control */
void postprocessing_exposure_set_manual(postprocessing_exposure_adaptation_handle_t handle, bool enabled, float exposure);

/* History Management */
void postprocessing_exposure_push_history(postprocessing_exposure_adaptation_handle_t handle, float exposure);
float postprocessing_exposure_get_average_history(postprocessing_exposure_adaptation_handle_t handle);
void postprocessing_exposure_clear_history(postprocessing_exposure_adaptation_handle_t handle);

/* Statistics */
uint32_t postprocessing_exposure_adaptation_get_count(void);
size_t postprocessing_exposure_adaptation_get_memory_usage(void);
void postprocessing_exposure_adaptation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_EXPOSURE_ADAPTATION_H */
