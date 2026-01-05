/*
 * stream_priority.h
 * Streaming priority
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAM_PRIORITY_H
#define TEXTURE_STREAM_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_stream_priority_handle {
    uint32_t id;
} texture_stream_priority_handle_t;

typedef struct texture_stream_priority_desc {
    uint32_t flags;
    void* user_data;
} texture_stream_priority_desc_t;

typedef struct texture_stream_priority_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_stream_priority_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_stream_priority_init(void);
void texture_stream_priority_shutdown(void);

/* Lifecycle */
int texture_stream_priority_create(texture_stream_priority_handle_t* out_handle, const texture_stream_priority_desc_t* desc);
void texture_stream_priority_destroy(texture_stream_priority_handle_t handle);

/* Operations */
int texture_stream_priority_update(texture_stream_priority_handle_t handle, const void* data, size_t size);
bool texture_stream_priority_is_valid(texture_stream_priority_handle_t handle);
int texture_stream_priority_get_info(texture_stream_priority_handle_t handle, texture_stream_priority_info_t* out_info);
void texture_stream_priority_mark_dirty(texture_stream_priority_handle_t handle);
int texture_stream_priority_process_pending(void);

/* Statistics */
uint32_t texture_stream_priority_get_count(void);
size_t texture_stream_priority_get_memory_usage(void);
void texture_stream_priority_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAM_PRIORITY_H */
