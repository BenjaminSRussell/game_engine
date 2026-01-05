/*
 * gpu_instance_data.h
 * GPU-side instance storage
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_GPU_INSTANCE_DATA_H
#define GEOMETRY_GPU_INSTANCE_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_gpu_instance_data_handle {
    uint32_t id;
} geometry_gpu_instance_data_handle_t;

typedef struct geometry_gpu_instance_data_desc {
    uint32_t flags;
    void* user_data;
} geometry_gpu_instance_data_desc_t;

typedef struct geometry_gpu_instance_data_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_gpu_instance_data_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_gpu_instance_data_init(void);
void geometry_gpu_instance_data_shutdown(void);

/* Lifecycle */
int geometry_gpu_instance_data_create(geometry_gpu_instance_data_handle_t* out_handle, const geometry_gpu_instance_data_desc_t* desc);
void geometry_gpu_instance_data_destroy(geometry_gpu_instance_data_handle_t handle);

/* Operations */
int geometry_gpu_instance_data_update(geometry_gpu_instance_data_handle_t handle, const void* data, size_t size);
bool geometry_gpu_instance_data_is_valid(geometry_gpu_instance_data_handle_t handle);
int geometry_gpu_instance_data_get_info(geometry_gpu_instance_data_handle_t handle, geometry_gpu_instance_data_info_t* out_info);
void geometry_gpu_instance_data_mark_dirty(geometry_gpu_instance_data_handle_t handle);
int geometry_gpu_instance_data_process_pending(void);

/* Statistics */
uint32_t geometry_gpu_instance_data_get_count(void);
size_t geometry_gpu_instance_data_get_memory_usage(void);
void geometry_gpu_instance_data_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_GPU_INSTANCE_DATA_H */
