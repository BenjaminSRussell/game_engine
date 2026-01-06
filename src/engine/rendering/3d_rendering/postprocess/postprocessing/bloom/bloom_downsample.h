/*
 * bloom_downsample.h
 * Bloom downsample chain
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_BLOOM_DOWNSAMPLE_H
#define POSTPROCESSING_BLOOM_DOWNSAMPLE_H

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

typedef struct postprocessing_bloom_downsample_handle {
    uint32_t id;
} postprocessing_bloom_downsample_handle_t;

typedef struct postprocessing_bloom_downsample_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_bloom_downsample_desc_t;

typedef struct postprocessing_bloom_downsample_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_bloom_downsample_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_bloom_downsample_init(void);
void postprocessing_bloom_downsample_shutdown(void);

/* Lifecycle */
int postprocessing_bloom_downsample_create(postprocessing_bloom_downsample_handle_t* out_handle, const postprocessing_bloom_downsample_desc_t* desc);
void postprocessing_bloom_downsample_destroy(postprocessing_bloom_downsample_handle_t handle);

/* Operations */
int postprocessing_bloom_downsample_update(postprocessing_bloom_downsample_handle_t handle, const void* data, size_t size);
bool postprocessing_bloom_downsample_is_valid(postprocessing_bloom_downsample_handle_t handle);
int postprocessing_bloom_downsample_get_info(postprocessing_bloom_downsample_handle_t handle, postprocessing_bloom_downsample_info_t* out_info);
void postprocessing_bloom_downsample_mark_dirty(postprocessing_bloom_downsample_handle_t handle);
int postprocessing_bloom_downsample_process_pending(void);

/* Processing */
// Downsamples input buffer to output buffer (half size) using 13-tap filter
void postprocessing_bloom_downsample_buffer(const float* input_rgb, uint32_t in_width, uint32_t in_height, float* output_rgb, uint32_t out_width, uint32_t out_height);

/* Statistics */
uint32_t postprocessing_bloom_downsample_get_count(void);
size_t postprocessing_bloom_downsample_get_memory_usage(void);
void postprocessing_bloom_downsample_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_BLOOM_DOWNSAMPLE_H */
