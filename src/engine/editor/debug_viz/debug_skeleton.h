/*
 * debug_skeleton.h
 * Skeleton visualization
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_DEBUG_SKELETON_H
#define EDITOR_DEBUG_SKELETON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_debug_skeleton_handle {
    uint32_t id;
} editor_debug_skeleton_handle_t;

typedef struct editor_debug_skeleton_desc {
    uint32_t flags;
    void* user_data;
} editor_debug_skeleton_desc_t;

typedef struct editor_debug_skeleton_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_debug_skeleton_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_debug_skeleton_init(void);
void editor_debug_skeleton_shutdown(void);

/* Lifecycle */
int editor_debug_skeleton_create(editor_debug_skeleton_handle_t* out_handle, const editor_debug_skeleton_desc_t* desc);
void editor_debug_skeleton_destroy(editor_debug_skeleton_handle_t handle);

/* Operations */
int editor_debug_skeleton_update(editor_debug_skeleton_handle_t handle, const void* data, size_t size);
bool editor_debug_skeleton_is_valid(editor_debug_skeleton_handle_t handle);
int editor_debug_skeleton_get_info(editor_debug_skeleton_handle_t handle, editor_debug_skeleton_info_t* out_info);
void editor_debug_skeleton_mark_dirty(editor_debug_skeleton_handle_t handle);
int editor_debug_skeleton_process_pending(void);

/* Statistics */
uint32_t editor_debug_skeleton_get_count(void);
size_t editor_debug_skeleton_get_memory_usage(void);
void editor_debug_skeleton_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_DEBUG_SKELETON_H */
