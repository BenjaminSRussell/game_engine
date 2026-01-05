/*
 * instance_buffer.h
 * Per-instance data buffers
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCE_BUFFER_H
#define GEOMETRY_INSTANCE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_instance_buffer_handle {
    uint32_t id;
} geometry_instance_buffer_handle_t;

typedef struct geometry_instance_buffer_desc {
    uint32_t flags;
    void* user_data;
} geometry_instance_buffer_desc_t;

typedef struct geometry_instance_buffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_instance_buffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_instance_buffer_init(void);
void geometry_instance_buffer_shutdown(void);

/* Lifecycle */
int geometry_instance_buffer_create(geometry_instance_buffer_handle_t* out_handle, const geometry_instance_buffer_desc_t* desc);
void geometry_instance_buffer_destroy(geometry_instance_buffer_handle_t handle);

/* Operations */
int geometry_instance_buffer_update(geometry_instance_buffer_handle_t handle, const void* data, size_t size);
bool geometry_instance_buffer_is_valid(geometry_instance_buffer_handle_t handle);
int geometry_instance_buffer_get_info(geometry_instance_buffer_handle_t handle, geometry_instance_buffer_info_t* out_info);
void geometry_instance_buffer_mark_dirty(geometry_instance_buffer_handle_t handle);
int geometry_instance_buffer_process_pending(void);

/* Statistics */
uint32_t geometry_instance_buffer_get_count(void);
size_t geometry_instance_buffer_get_memory_usage(void);
void geometry_instance_buffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCE_BUFFER_H */
