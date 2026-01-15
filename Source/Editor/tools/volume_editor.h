/*
 * volume_editor.h
 * Volume editing
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_VOLUME_EDITOR_H
#define EDITOR_VOLUME_EDITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_volume_editor_handle {
    uint32_t id;
} editor_volume_editor_handle_t;

typedef struct editor_volume_editor_desc {
    uint32_t flags;
    void* user_data;
} editor_volume_editor_desc_t;

typedef struct editor_volume_editor_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_volume_editor_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_volume_editor_init(void);
void editor_volume_editor_shutdown(void);

/* Lifecycle */
int editor_volume_editor_create(editor_volume_editor_handle_t* out_handle, const editor_volume_editor_desc_t* desc);
void editor_volume_editor_destroy(editor_volume_editor_handle_t handle);

/* Operations */
int editor_volume_editor_update(editor_volume_editor_handle_t handle, const void* data, size_t size);
bool editor_volume_editor_is_valid(editor_volume_editor_handle_t handle);
int editor_volume_editor_get_info(editor_volume_editor_handle_t handle, editor_volume_editor_info_t* out_info);
void editor_volume_editor_mark_dirty(editor_volume_editor_handle_t handle);
int editor_volume_editor_process_pending(void);

/* Statistics */
uint32_t editor_volume_editor_get_count(void);
size_t editor_volume_editor_get_memory_usage(void);
void editor_volume_editor_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_VOLUME_EDITOR_H */
