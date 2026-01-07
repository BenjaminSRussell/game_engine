/*
 * instance_buffer_rt.h
 * RT instance data
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_INSTANCE_BUFFER_RT_H
#define RAYTRACING_INSTANCE_BUFFER_RT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_instance_buffer_rt_handle {
    uint32_t id;
} raytracing_instance_buffer_rt_handle_t;

typedef struct raytracing_instance_buffer_rt_desc {
    uint32_t flags;
    void* user_data;
} raytracing_instance_buffer_rt_desc_t;

typedef struct raytracing_instance_buffer_rt_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_instance_buffer_rt_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_instance_buffer_rt_init(void);
void raytracing_instance_buffer_rt_shutdown(void);

/* Lifecycle */
int raytracing_instance_buffer_rt_create(raytracing_instance_buffer_rt_handle_t* out_handle, const raytracing_instance_buffer_rt_desc_t* desc);
void raytracing_instance_buffer_rt_destroy(raytracing_instance_buffer_rt_handle_t handle);

/* Operations */
int raytracing_instance_buffer_rt_update(raytracing_instance_buffer_rt_handle_t handle, const void* data, size_t size);
bool raytracing_instance_buffer_rt_is_valid(raytracing_instance_buffer_rt_handle_t handle);
int raytracing_instance_buffer_rt_get_info(raytracing_instance_buffer_rt_handle_t handle, raytracing_instance_buffer_rt_info_t* out_info);
void raytracing_instance_buffer_rt_mark_dirty(raytracing_instance_buffer_rt_handle_t handle);
int raytracing_instance_buffer_rt_process_pending(void);

/* Statistics */
uint32_t raytracing_instance_buffer_rt_get_count(void);
size_t raytracing_instance_buffer_rt_get_memory_usage(void);
void raytracing_instance_buffer_rt_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_INSTANCE_BUFFER_RT_H */
