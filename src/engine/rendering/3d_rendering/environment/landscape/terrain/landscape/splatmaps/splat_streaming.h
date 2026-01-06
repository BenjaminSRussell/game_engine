/*
 * splat_streaming.h
 * Splat map streaming
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_SPLAT_STREAMING_H
#define LANDSCAPE_SPLAT_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_splat_streaming_handle {
    uint32_t id;
} landscape_splat_streaming_handle_t;

typedef struct landscape_splat_streaming_desc {
    uint32_t flags;
    void* user_data;
} landscape_splat_streaming_desc_t;

typedef struct landscape_splat_streaming_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_splat_streaming_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_splat_streaming_init(void);
void landscape_splat_streaming_shutdown(void);

/* Lifecycle */
int landscape_splat_streaming_create(landscape_splat_streaming_handle_t* out_handle, const landscape_splat_streaming_desc_t* desc);
void landscape_splat_streaming_destroy(landscape_splat_streaming_handle_t handle);

/* Operations */
int landscape_splat_streaming_update(landscape_splat_streaming_handle_t handle, const void* data, size_t size);
bool landscape_splat_streaming_is_valid(landscape_splat_streaming_handle_t handle);
int landscape_splat_streaming_get_info(landscape_splat_streaming_handle_t handle, landscape_splat_streaming_info_t* out_info);
void landscape_splat_streaming_mark_dirty(landscape_splat_streaming_handle_t handle);
int landscape_splat_streaming_process_pending(void);

/* Statistics */
uint32_t landscape_splat_streaming_get_count(void);
size_t landscape_splat_streaming_get_memory_usage(void);
void landscape_splat_streaming_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_SPLAT_STREAMING_H */
