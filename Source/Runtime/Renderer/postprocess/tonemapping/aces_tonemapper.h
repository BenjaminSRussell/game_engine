/*
 * aces_tonemapper.h
 * ACES filmic tonemapping
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_ACES_TONEMAPPER_H
#define POSTPROCESSING_ACES_TONEMAPPER_H

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

typedef struct postprocessing_aces_tonemapper_handle {
    uint32_t id;
} postprocessing_aces_tonemapper_handle_t;

typedef struct aces_tonemap_params {
    float exposure;
    float white_point; // Not strictly used in standard ACES fit, but useful for pre-scale
    float saturation;  // Optional saturation adjustment
} aces_tonemap_params_t;

typedef struct postprocessing_aces_tonemapper_desc {
    uint32_t flags;
    aces_tonemap_params_t initial_params;
    void* user_data;
} postprocessing_aces_tonemapper_desc_t;

typedef struct postprocessing_aces_tonemapper_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    aces_tonemap_params_t current_params;
} postprocessing_aces_tonemapper_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_aces_tonemapper_init(void);
void postprocessing_aces_tonemapper_shutdown(void);

/* Lifecycle */
int postprocessing_aces_tonemapper_create(postprocessing_aces_tonemapper_handle_t* out_handle, const postprocessing_aces_tonemapper_desc_t* desc);
void postprocessing_aces_tonemapper_destroy(postprocessing_aces_tonemapper_handle_t handle);

/* Operations */
int postprocessing_aces_tonemapper_update(postprocessing_aces_tonemapper_handle_t handle, const void* data, size_t size);
void postprocessing_aces_tonemapper_set_params(postprocessing_aces_tonemapper_handle_t handle, const aces_tonemap_params_t* params);
bool postprocessing_aces_tonemapper_is_valid(postprocessing_aces_tonemapper_handle_t handle);
int postprocessing_aces_tonemapper_get_info(postprocessing_aces_tonemapper_handle_t handle, postprocessing_aces_tonemapper_info_t* out_info);
void postprocessing_aces_tonemapper_mark_dirty(postprocessing_aces_tonemapper_handle_t handle);
int postprocessing_aces_tonemapper_process_pending(void);

/* Processing */
// Helper for CPU-side processing (e.g. for color picking or debug)
// For full frame processing, use the render graph / GPU paths
void postprocessing_aces_tonemap_color(const aces_tonemap_params_t* params, float r, float g, float b, float* out_r, float* out_g, float* out_b);
void postprocessing_aces_tonemap_buffer(const aces_tonemap_params_t* params, const float* input_rgb, float* output_rgb, size_t pixel_count);

/* Statistics */
uint32_t postprocessing_aces_tonemapper_get_count(void);
size_t postprocessing_aces_tonemapper_get_memory_usage(void);
void postprocessing_aces_tonemapper_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_ACES_TONEMAPPER_H */
