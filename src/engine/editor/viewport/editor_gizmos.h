/*
 * editor_gizmos.h
 * Transform gizmos
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_EDITOR_GIZMOS_H
#define EDITOR_EDITOR_GIZMOS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_editor_gizmos_handle {
    uint32_t id;
} editor_editor_gizmos_handle_t;

typedef struct editor_editor_gizmos_desc {
    uint32_t flags;
    void* user_data;
} editor_editor_gizmos_desc_t;

typedef struct editor_editor_gizmos_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_editor_gizmos_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_editor_gizmos_init(void);
void editor_editor_gizmos_shutdown(void);

/* Lifecycle */
int editor_editor_gizmos_create(editor_editor_gizmos_handle_t* out_handle, const editor_editor_gizmos_desc_t* desc);
void editor_editor_gizmos_destroy(editor_editor_gizmos_handle_t handle);

/* Operations */
int editor_editor_gizmos_update(editor_editor_gizmos_handle_t handle, const void* data, size_t size);
bool editor_editor_gizmos_is_valid(editor_editor_gizmos_handle_t handle);
int editor_editor_gizmos_get_info(editor_editor_gizmos_handle_t handle, editor_editor_gizmos_info_t* out_info);
void editor_editor_gizmos_mark_dirty(editor_editor_gizmos_handle_t handle);
int editor_editor_gizmos_process_pending(void);

/* Statistics */
uint32_t editor_editor_gizmos_get_count(void);
size_t editor_editor_gizmos_get_memory_usage(void);
void editor_editor_gizmos_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_EDITOR_GIZMOS_H */
