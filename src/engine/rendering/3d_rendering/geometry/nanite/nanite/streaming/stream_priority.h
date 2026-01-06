/*
 * stream_priority.h
 * Streaming priority
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_STREAM_PRIORITY_H
#define NANITE_STREAM_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_stream_priority_handle {
    uint32_t id;
} nanite_stream_priority_handle_t;

typedef struct nanite_stream_priority_desc {
    uint32_t flags;
    void* user_data;
} nanite_stream_priority_desc_t;

typedef struct nanite_stream_priority_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_stream_priority_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_stream_priority_init(void);
void nanite_stream_priority_shutdown(void);

/* Lifecycle */
int nanite_stream_priority_create(nanite_stream_priority_handle_t* out_handle, const nanite_stream_priority_desc_t* desc);
void nanite_stream_priority_destroy(nanite_stream_priority_handle_t handle);

/* Operations */
int nanite_stream_priority_update(nanite_stream_priority_handle_t handle, const void* data, size_t size);
bool nanite_stream_priority_is_valid(nanite_stream_priority_handle_t handle);
int nanite_stream_priority_get_info(nanite_stream_priority_handle_t handle, nanite_stream_priority_info_t* out_info);
void nanite_stream_priority_mark_dirty(nanite_stream_priority_handle_t handle);
int nanite_stream_priority_process_pending(void);

/* Statistics */
uint32_t nanite_stream_priority_get_count(void);
size_t nanite_stream_priority_get_memory_usage(void);
void nanite_stream_priority_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_STREAM_PRIORITY_H */
