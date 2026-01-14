/*
 * reinhard_tonemapper.h
 * Reinhard tonemapping
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_REINHARD_TONEMAPPER_H
#define POSTPROCESSING_REINHARD_TONEMAPPER_H

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

typedef struct postprocessing_reinhard_tonemapper_handle {
    uint32_t id;
} postprocessing_reinhard_tonemapper_handle_t;

typedef struct reinhard_tonemap_params {
    float white_point; // L_white squared in the formula
    float exposure;    // Pre-exposure
} reinhard_tonemap_params_t;

typedef struct postprocessing_reinhard_tonemapper_desc {
    uint32_t flags;
    reinhard_tonemap_params_t initial_params;
    void* user_data;
} postprocessing_reinhard_tonemapper_desc_t;

typedef struct postprocessing_reinhard_tonemapper_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    reinhard_tonemap_params_t current_params;
} postprocessing_reinhard_tonemapper_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_reinhard_tonemapper_init(void);
void postprocessing_reinhard_tonemapper_shutdown(void);

/* Lifecycle */
int postprocessing_reinhard_tonemapper_create(postprocessing_reinhard_tonemapper_handle_t* out_handle, const postprocessing_reinhard_tonemapper_desc_t* desc);
void postprocessing_reinhard_tonemapper_destroy(postprocessing_reinhard_tonemapper_handle_t handle);

/* Operations */
int postprocessing_reinhard_tonemapper_update(postprocessing_reinhard_tonemapper_handle_t handle, const void* data, size_t size);
void postprocessing_reinhard_tonemapper_set_params(postprocessing_reinhard_tonemapper_handle_t handle, const reinhard_tonemap_params_t* params);
bool postprocessing_reinhard_tonemapper_is_valid(postprocessing_reinhard_tonemapper_handle_t handle);
int postprocessing_reinhard_tonemapper_get_info(postprocessing_reinhard_tonemapper_handle_t handle, postprocessing_reinhard_tonemapper_info_t* out_info);
void postprocessing_reinhard_tonemapper_mark_dirty(postprocessing_reinhard_tonemapper_handle_t handle);
int postprocessing_reinhard_tonemapper_process_pending(void);

/* Processing */
void postprocessing_reinhard_tonemap_color(const reinhard_tonemap_params_t* params, float r, float g, float b, float* out_r, float* out_g, float* out_b);
void postprocessing_reinhard_tonemap_buffer(const reinhard_tonemap_params_t* params, const float* input_rgb, float* output_rgb, size_t pixel_count);

/* Statistics */
uint32_t postprocessing_reinhard_tonemapper_get_count(void);
size_t postprocessing_reinhard_tonemapper_get_memory_usage(void);
void postprocessing_reinhard_tonemapper_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_REINHARD_TONEMAPPER_H */
