/*
 * histogram_compute.h
 * Luminance histogram
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_HISTOGRAM_COMPUTE_H
#define POSTPROCESSING_HISTOGRAM_COMPUTE_H

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

typedef struct postprocessing_histogram_compute_handle {
    uint32_t id;
} postprocessing_histogram_compute_handle_t;

#define HISTOGRAM_BIN_COUNT 64

typedef enum metering_mode {
    METERING_MODE_AVERAGE = 0,
    METERING_MODE_CENTER_WEIGHTED = 1,
    METERING_MODE_SPOT = 2,
    METERING_MODE_CUSTOM = 3
} metering_mode_t;

typedef struct histogram_params {
    float min_log_lum;   // Minimum log luminance (e.g., -10.0)
    float log_lum_range; // Range of log luminance (e.g., 20.0)
    float low_percentile; // For filtering outliers (e.g., 0.1)
    float high_percentile; // For filtering outliers (e.g., 0.9)
    
    // Metering
    metering_mode_t metering_mode;
    float center_weight; // Used for center-weighted metering (e.g., 0.5)
    float spot_range;    // Used for spot metering (e.g., 0.1)
} histogram_params_t;

typedef struct postprocessing_histogram_compute_desc {
    uint32_t flags;
    histogram_params_t initial_params;
    void* user_data;
} postprocessing_histogram_compute_desc_t;

typedef struct postprocessing_histogram_compute_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    histogram_params_t current_params;
    float computed_average_luminance;
} postprocessing_histogram_compute_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_histogram_compute_init(void);
void postprocessing_histogram_compute_shutdown(void);

/* Lifecycle */
int postprocessing_histogram_compute_create(postprocessing_histogram_compute_handle_t* out_handle, const postprocessing_histogram_compute_desc_t* desc);
void postprocessing_histogram_compute_destroy(postprocessing_histogram_compute_handle_t handle);

/* Operations */
int postprocessing_histogram_compute_update(postprocessing_histogram_compute_handle_t handle, const void* data, size_t size);
void postprocessing_histogram_compute_set_params(postprocessing_histogram_compute_handle_t handle, const histogram_params_t* params);
bool postprocessing_histogram_compute_is_valid(postprocessing_histogram_compute_handle_t handle);
int postprocessing_histogram_compute_get_info(postprocessing_histogram_compute_handle_t handle, postprocessing_histogram_compute_info_t* out_info);
void postprocessing_histogram_compute_mark_dirty(postprocessing_histogram_compute_handle_t handle);
int postprocessing_histogram_compute_process_pending(void);

/* Processing */
// Simulates histogram generation on CPU
void postprocessing_histogram_compute_cpu(const histogram_params_t* params, const float* input_rgb, size_t pixel_count, uint32_t* out_histogram);
// Analyzes histogram to find average luminance
float postprocessing_histogram_get_average_luminance(const histogram_params_t* params, const uint32_t* histogram, size_t total_pixels);

/* Statistics */
uint32_t postprocessing_histogram_compute_get_count(void);
size_t postprocessing_histogram_compute_get_memory_usage(void);
void postprocessing_histogram_compute_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_HISTOGRAM_COMPUTE_H */
