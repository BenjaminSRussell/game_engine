/*
 * gpu_cull_compute.h
 * GPU culling compute shader
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_GPU_CULL_COMPUTE_H
#define CULLING_GPU_CULL_COMPUTE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_gpu_cull_compute_handle {
    uint32_t id;
} culling_gpu_cull_compute_handle_t;

typedef struct culling_gpu_cull_compute_desc {
    uint32_t flags;
    void* user_data;
} culling_gpu_cull_compute_desc_t;

typedef struct culling_gpu_cull_compute_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_gpu_cull_compute_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_gpu_cull_compute_init(void);
void culling_gpu_cull_compute_shutdown(void);

/* Lifecycle */
int culling_gpu_cull_compute_create(culling_gpu_cull_compute_handle_t* out_handle, const culling_gpu_cull_compute_desc_t* desc);
void culling_gpu_cull_compute_destroy(culling_gpu_cull_compute_handle_t handle);

/* Operations */
int culling_gpu_cull_compute_update(culling_gpu_cull_compute_handle_t handle, const void* data, size_t size);
bool culling_gpu_cull_compute_is_valid(culling_gpu_cull_compute_handle_t handle);
int culling_gpu_cull_compute_get_info(culling_gpu_cull_compute_handle_t handle, culling_gpu_cull_compute_info_t* out_info);
void culling_gpu_cull_compute_mark_dirty(culling_gpu_cull_compute_handle_t handle);
int culling_gpu_cull_compute_process_pending(void);

/* Statistics */
uint32_t culling_gpu_cull_compute_get_count(void);
size_t culling_gpu_cull_compute_get_memory_usage(void);
void culling_gpu_cull_compute_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_GPU_CULL_COMPUTE_H */
