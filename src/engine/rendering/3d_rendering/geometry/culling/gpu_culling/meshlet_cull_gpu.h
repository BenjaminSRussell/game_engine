/*
 * meshlet_cull_gpu.h
 * GPU meshlet culling
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_MESHLET_CULL_GPU_H
#define CULLING_MESHLET_CULL_GPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_meshlet_cull_gpu_handle {
    uint32_t id;
} culling_meshlet_cull_gpu_handle_t;

typedef struct culling_meshlet_cull_gpu_desc {
    uint32_t flags;
    void* user_data;
} culling_meshlet_cull_gpu_desc_t;

typedef struct culling_meshlet_cull_gpu_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_meshlet_cull_gpu_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_meshlet_cull_gpu_init(void);
void culling_meshlet_cull_gpu_shutdown(void);

/* Lifecycle */
int culling_meshlet_cull_gpu_create(culling_meshlet_cull_gpu_handle_t* out_handle, const culling_meshlet_cull_gpu_desc_t* desc);
void culling_meshlet_cull_gpu_destroy(culling_meshlet_cull_gpu_handle_t handle);

/* Operations */
int culling_meshlet_cull_gpu_update(culling_meshlet_cull_gpu_handle_t handle, const void* data, size_t size);
bool culling_meshlet_cull_gpu_is_valid(culling_meshlet_cull_gpu_handle_t handle);
int culling_meshlet_cull_gpu_get_info(culling_meshlet_cull_gpu_handle_t handle, culling_meshlet_cull_gpu_info_t* out_info);
void culling_meshlet_cull_gpu_mark_dirty(culling_meshlet_cull_gpu_handle_t handle);
int culling_meshlet_cull_gpu_process_pending(void);

/* Statistics */
uint32_t culling_meshlet_cull_gpu_get_count(void);
size_t culling_meshlet_cull_gpu_get_memory_usage(void);
void culling_meshlet_cull_gpu_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_MESHLET_CULL_GPU_H */
