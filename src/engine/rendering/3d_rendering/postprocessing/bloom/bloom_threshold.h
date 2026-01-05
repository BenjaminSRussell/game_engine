/*
 * bloom_threshold.h
 * Bloom threshold filter
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_BLOOM_THRESHOLD_H
#define POSTPROCESSING_BLOOM_THRESHOLD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_bloom_threshold_handle {
    uint32_t id;
} postprocessing_bloom_threshold_handle_t;

typedef struct postprocessing_bloom_threshold_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_bloom_threshold_desc_t;

typedef struct postprocessing_bloom_threshold_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_bloom_threshold_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_bloom_threshold_init(void);
void postprocessing_bloom_threshold_shutdown(void);

/* Lifecycle */
int postprocessing_bloom_threshold_create(postprocessing_bloom_threshold_handle_t* out_handle, const postprocessing_bloom_threshold_desc_t* desc);
void postprocessing_bloom_threshold_destroy(postprocessing_bloom_threshold_handle_t handle);

/* Operations */
int postprocessing_bloom_threshold_update(postprocessing_bloom_threshold_handle_t handle, const void* data, size_t size);
bool postprocessing_bloom_threshold_is_valid(postprocessing_bloom_threshold_handle_t handle);
int postprocessing_bloom_threshold_get_info(postprocessing_bloom_threshold_handle_t handle, postprocessing_bloom_threshold_info_t* out_info);
void postprocessing_bloom_threshold_mark_dirty(postprocessing_bloom_threshold_handle_t handle);
int postprocessing_bloom_threshold_process_pending(void);

/* Statistics */
uint32_t postprocessing_bloom_threshold_get_count(void);
size_t postprocessing_bloom_threshold_get_memory_usage(void);
void postprocessing_bloom_threshold_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_BLOOM_THRESHOLD_H */
