/*
 * brush_preview.h
 * Terrain brush preview
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_BRUSH_PREVIEW_H
#define EDITOR_BRUSH_PREVIEW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_brush_preview_handle {
    uint32_t id;
} editor_brush_preview_handle_t;

typedef struct editor_brush_preview_desc {
    uint32_t flags;
    void* user_data;
} editor_brush_preview_desc_t;

typedef struct editor_brush_preview_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_brush_preview_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_brush_preview_init(void);
void editor_brush_preview_shutdown(void);

/* Lifecycle */
int editor_brush_preview_create(editor_brush_preview_handle_t* out_handle, const editor_brush_preview_desc_t* desc);
void editor_brush_preview_destroy(editor_brush_preview_handle_t handle);

/* Operations */
int editor_brush_preview_update(editor_brush_preview_handle_t handle, const void* data, size_t size);
bool editor_brush_preview_is_valid(editor_brush_preview_handle_t handle);
int editor_brush_preview_get_info(editor_brush_preview_handle_t handle, editor_brush_preview_info_t* out_info);
void editor_brush_preview_mark_dirty(editor_brush_preview_handle_t handle);
int editor_brush_preview_process_pending(void);

/* Statistics */
uint32_t editor_brush_preview_get_count(void);
size_t editor_brush_preview_get_memory_usage(void);
void editor_brush_preview_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_BRUSH_PREVIEW_H */
