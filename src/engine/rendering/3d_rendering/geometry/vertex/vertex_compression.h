/*
 * vertex_compression.h
 * Quantized vertex compression
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_VERTEX_COMPRESSION_H
#define GEOMETRY_VERTEX_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_vertex_compression_handle {
    uint32_t id;
} geometry_vertex_compression_handle_t;

typedef struct geometry_vertex_compression_desc {
    uint32_t flags;
    void* user_data;
} geometry_vertex_compression_desc_t;

typedef struct geometry_vertex_compression_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_vertex_compression_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_vertex_compression_init(void);
void geometry_vertex_compression_shutdown(void);

/* Lifecycle */
int geometry_vertex_compression_create(geometry_vertex_compression_handle_t* out_handle, const geometry_vertex_compression_desc_t* desc);
void geometry_vertex_compression_destroy(geometry_vertex_compression_handle_t handle);

/* Operations */
int geometry_vertex_compression_update(geometry_vertex_compression_handle_t handle, const void* data, size_t size);
bool geometry_vertex_compression_is_valid(geometry_vertex_compression_handle_t handle);
int geometry_vertex_compression_get_info(geometry_vertex_compression_handle_t handle, geometry_vertex_compression_info_t* out_info);
void geometry_vertex_compression_mark_dirty(geometry_vertex_compression_handle_t handle);
int geometry_vertex_compression_process_pending(void);

/* Statistics */
uint32_t geometry_vertex_compression_get_count(void);
size_t geometry_vertex_compression_get_memory_usage(void);
void geometry_vertex_compression_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_VERTEX_COMPRESSION_H */
