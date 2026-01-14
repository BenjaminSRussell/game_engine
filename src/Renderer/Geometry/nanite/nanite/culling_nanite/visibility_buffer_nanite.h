/*
 * visibility_buffer_nanite.h
 * Visibility buffer integration
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_VISIBILITY_BUFFER_NANITE_H
#define NANITE_VISIBILITY_BUFFER_NANITE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_visibility_buffer_nanite_handle {
    uint32_t id;
} nanite_visibility_buffer_nanite_handle_t;

typedef struct nanite_visibility_buffer_nanite_desc {
    uint32_t flags;
    void* user_data;
} nanite_visibility_buffer_nanite_desc_t;

typedef struct nanite_visibility_buffer_nanite_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_visibility_buffer_nanite_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_visibility_buffer_nanite_init(void);
void nanite_visibility_buffer_nanite_shutdown(void);

/* Lifecycle */
int nanite_visibility_buffer_nanite_create(nanite_visibility_buffer_nanite_handle_t* out_handle, const nanite_visibility_buffer_nanite_desc_t* desc);
void nanite_visibility_buffer_nanite_destroy(nanite_visibility_buffer_nanite_handle_t handle);

/* Operations */
int nanite_visibility_buffer_nanite_update(nanite_visibility_buffer_nanite_handle_t handle, const void* data, size_t size);
bool nanite_visibility_buffer_nanite_is_valid(nanite_visibility_buffer_nanite_handle_t handle);
int nanite_visibility_buffer_nanite_get_info(nanite_visibility_buffer_nanite_handle_t handle, nanite_visibility_buffer_nanite_info_t* out_info);
void nanite_visibility_buffer_nanite_mark_dirty(nanite_visibility_buffer_nanite_handle_t handle);
int nanite_visibility_buffer_nanite_process_pending(void);

/* Statistics */
uint32_t nanite_visibility_buffer_nanite_get_count(void);
size_t nanite_visibility_buffer_nanite_get_memory_usage(void);
void nanite_visibility_buffer_nanite_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_VISIBILITY_BUFFER_NANITE_H */
