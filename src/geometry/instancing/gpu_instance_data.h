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
#include <math/mat4.h>
#include <math/vec4.h>

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
    uint32_t initial_capacity;
    uint32_t flags;
    void* user_data;
} geometry_gpu_instance_data_desc_t;

typedef struct geometry_gpu_instance_data_info {
    uint32_t id;
    uint32_t flags;
    uint32_t capacity;
    uint32_t count;
    bool initialized;
} geometry_gpu_instance_data_info_t;

// GPU-side instance structure (must match shader)
typedef struct {
    mat4 model;
    vec4 color;
    uint32_t texture_id;
    uint32_t padding[3]; // Align to 16 bytes if needed, though std430 handles it
} GPUInstance;

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
int geometry_gpu_instance_data_update(geometry_gpu_instance_data_handle_t handle, const void* data, size_t count);
bool geometry_gpu_instance_data_is_valid(geometry_gpu_instance_data_handle_t handle);
int geometry_gpu_instance_data_get_info(geometry_gpu_instance_data_handle_t handle, geometry_gpu_instance_data_info_t* out_info);
void geometry_gpu_instance_data_mark_dirty(geometry_gpu_instance_data_handle_t handle);

/* Access */
// Returns the SSBO/Buffer ID for binding
uint32_t geometry_gpu_instance_data_get_buffer_id(geometry_gpu_instance_data_handle_t handle);

/* Statistics */
uint32_t geometry_gpu_instance_data_get_count(void);
size_t geometry_gpu_instance_data_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_GPU_INSTANCE_DATA_H */
