/*
 * stream_priority.h
 * Mesh streaming priority
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_STREAM_PRIORITY_H
#define GEOMETRY_STREAM_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_stream_priority_handle {
    uint32_t id;
} geometry_stream_priority_handle_t;

typedef struct geometry_stream_priority_desc {
    uint32_t flags;
    void* user_data;
} geometry_stream_priority_desc_t;

typedef struct geometry_stream_priority_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_stream_priority_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_stream_priority_init(void);
void geometry_stream_priority_shutdown(void);

/* Lifecycle */
int geometry_stream_priority_create(geometry_stream_priority_handle_t* out_handle, const geometry_stream_priority_desc_t* desc);
void geometry_stream_priority_destroy(geometry_stream_priority_handle_t handle);

/* Operations */
int geometry_stream_priority_update(geometry_stream_priority_handle_t handle, const void* data, size_t size);
bool geometry_stream_priority_is_valid(geometry_stream_priority_handle_t handle);
int geometry_stream_priority_get_info(geometry_stream_priority_handle_t handle, geometry_stream_priority_info_t* out_info);
void geometry_stream_priority_mark_dirty(geometry_stream_priority_handle_t handle);
int geometry_stream_priority_process_pending(void);

/* Statistics */
uint32_t geometry_stream_priority_get_count(void);
size_t geometry_stream_priority_get_memory_usage(void);
void geometry_stream_priority_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_STREAM_PRIORITY_H */
