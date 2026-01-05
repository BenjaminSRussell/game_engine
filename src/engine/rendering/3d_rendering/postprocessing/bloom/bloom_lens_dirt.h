/*
 * bloom_lens_dirt.h
 * Lens dirt overlay
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_BLOOM_LENS_DIRT_H
#define POSTPROCESSING_BLOOM_LENS_DIRT_H

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

typedef struct postprocessing_bloom_lens_dirt_handle {
    uint32_t id;
} postprocessing_bloom_lens_dirt_handle_t;

typedef struct bloom_lens_dirt_params {
    float intensity; // Intensity of dirt effect (e.g. 1.0)
    // In a real engine, this would be a texture handle.
    // For this simulation, we assume user passes texture data in the apply function.
} bloom_lens_dirt_params_t;

typedef struct postprocessing_bloom_lens_dirt_desc {
    uint32_t flags;
    bloom_lens_dirt_params_t initial_params;
    void* user_data;
} postprocessing_bloom_lens_dirt_desc_t;

typedef struct postprocessing_bloom_lens_dirt_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    bloom_lens_dirt_params_t current_params;
} postprocessing_bloom_lens_dirt_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_bloom_lens_dirt_init(void);
void postprocessing_bloom_lens_dirt_shutdown(void);

/* Lifecycle */
int postprocessing_bloom_lens_dirt_create(postprocessing_bloom_lens_dirt_handle_t* out_handle, const postprocessing_bloom_lens_dirt_desc_t* desc);
void postprocessing_bloom_lens_dirt_destroy(postprocessing_bloom_lens_dirt_handle_t handle);

/* Operations */
int postprocessing_bloom_lens_dirt_update(postprocessing_bloom_lens_dirt_handle_t handle, const void* data, size_t size);
void postprocessing_bloom_lens_dirt_set_params(postprocessing_bloom_lens_dirt_handle_t handle, const bloom_lens_dirt_params_t* params);
bool postprocessing_bloom_lens_dirt_is_valid(postprocessing_bloom_lens_dirt_handle_t handle);
int postprocessing_bloom_lens_dirt_get_info(postprocessing_bloom_lens_dirt_handle_t handle, postprocessing_bloom_lens_dirt_info_t* out_info);
void postprocessing_bloom_lens_dirt_mark_dirty(postprocessing_bloom_lens_dirt_handle_t handle);
int postprocessing_bloom_lens_dirt_process_pending(void);

/* Processing */
// Applies lens dirt overlay.
// output = bloom + bloom * dirt_texture * intensity
void postprocessing_bloom_lens_dirt_apply(const bloom_lens_dirt_params_t* params, const float* bloom_buffer, const float* dirt_texture, float* output_buffer, size_t pixel_count);

/* Statistics */
uint32_t postprocessing_bloom_lens_dirt_get_count(void);
size_t postprocessing_bloom_lens_dirt_get_memory_usage(void);
void postprocessing_bloom_lens_dirt_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_BLOOM_LENS_DIRT_H */
