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

typedef struct postprocessing_histogram_compute_handle {
    uint32_t id;
} postprocessing_histogram_compute_handle_t;

typedef struct postprocessing_histogram_compute_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_histogram_compute_desc_t;

typedef struct postprocessing_histogram_compute_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
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
bool postprocessing_histogram_compute_is_valid(postprocessing_histogram_compute_handle_t handle);
int postprocessing_histogram_compute_get_info(postprocessing_histogram_compute_handle_t handle, postprocessing_histogram_compute_info_t* out_info);
void postprocessing_histogram_compute_mark_dirty(postprocessing_histogram_compute_handle_t handle);
int postprocessing_histogram_compute_process_pending(void);

/* Statistics */
uint32_t postprocessing_histogram_compute_get_count(void);
size_t postprocessing_histogram_compute_get_memory_usage(void);
void postprocessing_histogram_compute_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_HISTOGRAM_COMPUTE_H */
