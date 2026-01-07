/*
 * stream_cache.h
 * Streamed mesh caching
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_STREAM_CACHE_H
#define GEOMETRY_STREAM_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_stream_cache_handle {
    uint32_t id;
} geometry_stream_cache_handle_t;

typedef struct geometry_stream_cache_desc {
    uint32_t flags;
    void* user_data;
} geometry_stream_cache_desc_t;

typedef struct geometry_stream_cache_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_stream_cache_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_stream_cache_init(void);
void geometry_stream_cache_shutdown(void);

/* Lifecycle */
int geometry_stream_cache_create(geometry_stream_cache_handle_t* out_handle, const geometry_stream_cache_desc_t* desc);
void geometry_stream_cache_destroy(geometry_stream_cache_handle_t handle);

/* Operations */
int geometry_stream_cache_update(geometry_stream_cache_handle_t handle, const void* data, size_t size);
bool geometry_stream_cache_is_valid(geometry_stream_cache_handle_t handle);
int geometry_stream_cache_get_info(geometry_stream_cache_handle_t handle, geometry_stream_cache_info_t* out_info);
void geometry_stream_cache_mark_dirty(geometry_stream_cache_handle_t handle);
int geometry_stream_cache_process_pending(void);

/* Statistics */
uint32_t geometry_stream_cache_get_count(void);
size_t geometry_stream_cache_get_memory_usage(void);
void geometry_stream_cache_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_STREAM_CACHE_H */
