/*
 * instance_data_gpu.h
 * GPU instance buffer
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_INSTANCE_DATA_GPU_H
#define RENDERING_INSTANCE_DATA_GPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_instance_data_gpu_handle {
    uint32_t id;
} rendering_instance_data_gpu_handle_t;

typedef struct rendering_instance_data_gpu_desc {
    uint32_t flags;
    void* user_data;
} rendering_instance_data_gpu_desc_t;

typedef struct rendering_instance_data_gpu_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_instance_data_gpu_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_instance_data_gpu_init(void);
void rendering_instance_data_gpu_shutdown(void);

/* Lifecycle */
int rendering_instance_data_gpu_create(rendering_instance_data_gpu_handle_t* out_handle, const rendering_instance_data_gpu_desc_t* desc);
void rendering_instance_data_gpu_destroy(rendering_instance_data_gpu_handle_t handle);

/* Operations */
int rendering_instance_data_gpu_update(rendering_instance_data_gpu_handle_t handle, const void* data, size_t size);
bool rendering_instance_data_gpu_is_valid(rendering_instance_data_gpu_handle_t handle);
int rendering_instance_data_gpu_get_info(rendering_instance_data_gpu_handle_t handle, rendering_instance_data_gpu_info_t* out_info);
void rendering_instance_data_gpu_mark_dirty(rendering_instance_data_gpu_handle_t handle);
int rendering_instance_data_gpu_process_pending(void);

/* Statistics */
uint32_t rendering_instance_data_gpu_get_count(void);
size_t rendering_instance_data_gpu_get_memory_usage(void);
void rendering_instance_data_gpu_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_INSTANCE_DATA_GPU_H */
