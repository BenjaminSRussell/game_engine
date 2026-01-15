/*
 * debug_bounds.h
 * Bounds visualization
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_DEBUG_BOUNDS_H
#define EDITOR_DEBUG_BOUNDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_debug_bounds_handle {
    uint32_t id;
} editor_debug_bounds_handle_t;

typedef struct editor_debug_bounds_desc {
    uint32_t flags;
    void* user_data;
} editor_debug_bounds_desc_t;

typedef struct editor_debug_bounds_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_debug_bounds_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_debug_bounds_init(void);
void editor_debug_bounds_shutdown(void);

/* Lifecycle */
int editor_debug_bounds_create(editor_debug_bounds_handle_t* out_handle, const editor_debug_bounds_desc_t* desc);
void editor_debug_bounds_destroy(editor_debug_bounds_handle_t handle);

/* Operations */
int editor_debug_bounds_update(editor_debug_bounds_handle_t handle, const void* data, size_t size);
bool editor_debug_bounds_is_valid(editor_debug_bounds_handle_t handle);
int editor_debug_bounds_get_info(editor_debug_bounds_handle_t handle, editor_debug_bounds_info_t* out_info);
void editor_debug_bounds_mark_dirty(editor_debug_bounds_handle_t handle);
int editor_debug_bounds_process_pending(void);

/* Statistics */
uint32_t editor_debug_bounds_get_count(void);
size_t editor_debug_bounds_get_memory_usage(void);
void editor_debug_bounds_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_DEBUG_BOUNDS_H */
