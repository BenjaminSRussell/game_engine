/*
 * gtao_pass.h
 * Ground truth AO
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_GTAO_PASS_H
#define POSTPROCESSING_GTAO_PASS_H

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

typedef struct postprocessing_gtao_pass_handle {
    uint32_t id;
} postprocessing_gtao_pass_handle_t;

typedef struct gtao_params {
    float radius;           // Search radius (world space)
    int slice_count;        // Number of slices (directions), e.g. 3-8
    int step_count;         // Steps per slice, e.g. 4-16
    float falloff;          // Distance falloff
} gtao_params_t;

typedef struct postprocessing_gtao_pass_desc {
    uint32_t flags;
    gtao_params_t initial_params;
    void* user_data;
} postprocessing_gtao_pass_desc_t;

typedef struct postprocessing_gtao_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    gtao_params_t current_params;
} postprocessing_gtao_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_gtao_pass_init(void);
void postprocessing_gtao_pass_shutdown(void);

/* Lifecycle */
int postprocessing_gtao_pass_create(postprocessing_gtao_pass_handle_t* out_handle, const postprocessing_gtao_pass_desc_t* desc);
void postprocessing_gtao_pass_destroy(postprocessing_gtao_pass_handle_t handle);

/* Operations */
int postprocessing_gtao_pass_update(postprocessing_gtao_pass_handle_t handle, const void* data, size_t size);
void postprocessing_gtao_pass_set_params(postprocessing_gtao_pass_handle_t handle, const gtao_params_t* params);
bool postprocessing_gtao_pass_is_valid(postprocessing_gtao_pass_handle_t handle);
int postprocessing_gtao_pass_get_info(postprocessing_gtao_pass_handle_t handle, postprocessing_gtao_pass_info_t* out_info);
void postprocessing_gtao_pass_mark_dirty(postprocessing_gtao_pass_handle_t handle);
int postprocessing_gtao_pass_process_pending(void);

/* Processing */
// Computes GTAO on CPU.
void postprocessing_gtao_compute_cpu(
    const gtao_params_t* params,
    const float* input_depth,
    const float* input_normals,
    float* output_ao,
    uint32_t width,
    uint32_t height,
    const float* projection_matrix
);

/* Statistics */
uint32_t postprocessing_gtao_pass_get_count(void);
size_t postprocessing_gtao_pass_get_memory_usage(void);
void postprocessing_gtao_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_GTAO_PASS_H */
