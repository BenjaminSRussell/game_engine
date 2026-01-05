/*
 * ssao_pass.h
 * Screen-space AO
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSAO_PASS_H
#define POSTPROCESSING_SSAO_PASS_H

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

typedef struct postprocessing_ssao_pass_handle {
    uint32_t id;
} postprocessing_ssao_pass_handle_t;

typedef struct ssao_params {
    float radius;       // Hemisphere radius (e.g. 0.5)
    float bias;         // Depth bias to avoid acne (e.g. 0.025)
    float intensity;    // Occlusion intensity (e.g. 1.0)
    int kernel_size;    // Number of samples (e.g. 16, 32, 64)
} ssao_params_t;

typedef struct postprocessing_ssao_pass_desc {
    uint32_t flags;
    ssao_params_t initial_params;
    void* user_data;
} postprocessing_ssao_pass_desc_t;

typedef struct postprocessing_ssao_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    ssao_params_t current_params;
} postprocessing_ssao_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_ssao_pass_init(void);
void postprocessing_ssao_pass_shutdown(void);

/* Lifecycle */
int postprocessing_ssao_pass_create(postprocessing_ssao_pass_handle_t* out_handle, const postprocessing_ssao_pass_desc_t* desc);
void postprocessing_ssao_pass_destroy(postprocessing_ssao_pass_handle_t handle);

/* Operations */
int postprocessing_ssao_pass_update(postprocessing_ssao_pass_handle_t handle, const void* data, size_t size);
void postprocessing_ssao_pass_set_params(postprocessing_ssao_pass_handle_t handle, const ssao_params_t* params);
bool postprocessing_ssao_pass_is_valid(postprocessing_ssao_pass_handle_t handle);
int postprocessing_ssao_pass_get_info(postprocessing_ssao_pass_handle_t handle, postprocessing_ssao_pass_info_t* out_info);
void postprocessing_ssao_pass_mark_dirty(postprocessing_ssao_pass_handle_t handle);
int postprocessing_ssao_pass_process_pending(void);

/* Processing */
// Computes SSAO on CPU.
// input_depth: linear view-space depth per pixel.
// input_normals: view-space normals (rgb encoded as -1..1 or 0..1).
// output_ao: single channel AO factor (0=occluded, 1=visible).
void postprocessing_ssao_compute_cpu(
    const ssao_params_t* params,
    const float* input_depth,
    const float* input_normals,
    float* output_ao,
    uint32_t width,
    uint32_t height,
    const float* projection_matrix
);

/* Statistics */
uint32_t postprocessing_ssao_pass_get_count(void);
size_t postprocessing_ssao_pass_get_memory_usage(void);
void postprocessing_ssao_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSAO_PASS_H */
