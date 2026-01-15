/*
 * vertex_buffer_pool.h
 * Vertex buffer allocation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_VERTEX_BUFFER_POOL_H
#define GEOMETRY_VERTEX_BUFFER_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_vertex_buffer_pool_handle {
    uint32_t id;
} geometry_vertex_buffer_pool_handle_t;

typedef struct geometry_vertex_buffer_pool_desc {
    uint32_t flags;
    void* user_data;
} geometry_vertex_buffer_pool_desc_t;

typedef struct geometry_vertex_buffer_pool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_vertex_buffer_pool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_vertex_buffer_pool_init(void);
void geometry_vertex_buffer_pool_shutdown(void);

/* Lifecycle */
int geometry_vertex_buffer_pool_create(geometry_vertex_buffer_pool_handle_t* out_handle, const geometry_vertex_buffer_pool_desc_t* desc);
void geometry_vertex_buffer_pool_destroy(geometry_vertex_buffer_pool_handle_t handle);

/* Operations */
int geometry_vertex_buffer_pool_update(geometry_vertex_buffer_pool_handle_t handle, const void* data, size_t size);
bool geometry_vertex_buffer_pool_is_valid(geometry_vertex_buffer_pool_handle_t handle);
int geometry_vertex_buffer_pool_get_info(geometry_vertex_buffer_pool_handle_t handle, geometry_vertex_buffer_pool_info_t* out_info);
void geometry_vertex_buffer_pool_mark_dirty(geometry_vertex_buffer_pool_handle_t handle);
int geometry_vertex_buffer_pool_process_pending(void);

/* Statistics */
uint32_t geometry_vertex_buffer_pool_get_count(void);
size_t geometry_vertex_buffer_pool_get_memory_usage(void);
void geometry_vertex_buffer_pool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_VERTEX_BUFFER_POOL_H */
