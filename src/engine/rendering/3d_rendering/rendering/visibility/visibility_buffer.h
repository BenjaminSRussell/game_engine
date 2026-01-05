/*
 * visibility_buffer.h
 * Visibility buffer pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_VISIBILITY_BUFFER_H
#define RENDERING_VISIBILITY_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_visibility_buffer_handle {
    uint32_t id;
} rendering_visibility_buffer_handle_t;

typedef struct rendering_visibility_buffer_desc {
    uint32_t flags;
    void* user_data;
} rendering_visibility_buffer_desc_t;

typedef struct rendering_visibility_buffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_visibility_buffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_visibility_buffer_init(void);
void rendering_visibility_buffer_shutdown(void);

/* Lifecycle */
int rendering_visibility_buffer_create(rendering_visibility_buffer_handle_t* out_handle, const rendering_visibility_buffer_desc_t* desc);
void rendering_visibility_buffer_destroy(rendering_visibility_buffer_handle_t handle);

/* Operations */
int rendering_visibility_buffer_update(rendering_visibility_buffer_handle_t handle, const void* data, size_t size);
bool rendering_visibility_buffer_is_valid(rendering_visibility_buffer_handle_t handle);
int rendering_visibility_buffer_get_info(rendering_visibility_buffer_handle_t handle, rendering_visibility_buffer_info_t* out_info);
void rendering_visibility_buffer_mark_dirty(rendering_visibility_buffer_handle_t handle);
int rendering_visibility_buffer_process_pending(void);

/* Statistics */
uint32_t rendering_visibility_buffer_get_count(void);
size_t rendering_visibility_buffer_get_memory_usage(void);
void rendering_visibility_buffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_VISIBILITY_BUFFER_H */
