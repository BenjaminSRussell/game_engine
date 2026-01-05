/*
 * spline_editor.h
 * Spline visualization
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_SPLINE_EDITOR_H
#define EDITOR_SPLINE_EDITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_spline_editor_handle {
    uint32_t id;
} editor_spline_editor_handle_t;

typedef struct editor_spline_editor_desc {
    uint32_t flags;
    void* user_data;
} editor_spline_editor_desc_t;

typedef struct editor_spline_editor_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_spline_editor_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_spline_editor_init(void);
void editor_spline_editor_shutdown(void);

/* Lifecycle */
int editor_spline_editor_create(editor_spline_editor_handle_t* out_handle, const editor_spline_editor_desc_t* desc);
void editor_spline_editor_destroy(editor_spline_editor_handle_t handle);

/* Operations */
int editor_spline_editor_update(editor_spline_editor_handle_t handle, const void* data, size_t size);
bool editor_spline_editor_is_valid(editor_spline_editor_handle_t handle);
int editor_spline_editor_get_info(editor_spline_editor_handle_t handle, editor_spline_editor_info_t* out_info);
void editor_spline_editor_mark_dirty(editor_spline_editor_handle_t handle);
int editor_spline_editor_process_pending(void);

/* Statistics */
uint32_t editor_spline_editor_get_count(void);
size_t editor_spline_editor_get_memory_usage(void);
void editor_spline_editor_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_SPLINE_EDITOR_H */
