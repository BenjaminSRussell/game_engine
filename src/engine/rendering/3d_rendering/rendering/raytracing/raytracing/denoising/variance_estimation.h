/*
 * variance_estimation.h
 * Noise variance estimation
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_VARIANCE_ESTIMATION_H
#define RAYTRACING_VARIANCE_ESTIMATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_variance_estimation_handle {
    uint32_t id;
} raytracing_variance_estimation_handle_t;

typedef struct raytracing_variance_estimation_desc {
    uint32_t flags;
    void* user_data;
} raytracing_variance_estimation_desc_t;

typedef struct raytracing_variance_estimation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_variance_estimation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_variance_estimation_init(void);
void raytracing_variance_estimation_shutdown(void);

/* Lifecycle */
int raytracing_variance_estimation_create(raytracing_variance_estimation_handle_t* out_handle, const raytracing_variance_estimation_desc_t* desc);
void raytracing_variance_estimation_destroy(raytracing_variance_estimation_handle_t handle);

/* Operations */
int raytracing_variance_estimation_update(raytracing_variance_estimation_handle_t handle, const void* data, size_t size);
bool raytracing_variance_estimation_is_valid(raytracing_variance_estimation_handle_t handle);
int raytracing_variance_estimation_get_info(raytracing_variance_estimation_handle_t handle, raytracing_variance_estimation_info_t* out_info);
void raytracing_variance_estimation_mark_dirty(raytracing_variance_estimation_handle_t handle);
int raytracing_variance_estimation_process_pending(void);

/* Statistics */
uint32_t raytracing_variance_estimation_get_count(void);
size_t raytracing_variance_estimation_get_memory_usage(void);
void raytracing_variance_estimation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_VARIANCE_ESTIMATION_H */
