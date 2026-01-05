/*
 * debug_text.h
 * Debug text overlay
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_DEBUG_TEXT_H
#define EDITOR_DEBUG_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_debug_text_handle {
    uint32_t id;
} editor_debug_text_handle_t;

typedef struct editor_debug_text_desc {
    uint32_t flags;
    void* user_data;
} editor_debug_text_desc_t;

typedef struct editor_debug_text_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_debug_text_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_debug_text_init(void);
void editor_debug_text_shutdown(void);

/* Lifecycle */
int editor_debug_text_create(editor_debug_text_handle_t* out_handle, const editor_debug_text_desc_t* desc);
void editor_debug_text_destroy(editor_debug_text_handle_t handle);

/* Operations */
int editor_debug_text_update(editor_debug_text_handle_t handle, const void* data, size_t size);
bool editor_debug_text_is_valid(editor_debug_text_handle_t handle);
int editor_debug_text_get_info(editor_debug_text_handle_t handle, editor_debug_text_info_t* out_info);
void editor_debug_text_mark_dirty(editor_debug_text_handle_t handle);
int editor_debug_text_process_pending(void);

/* Statistics */
uint32_t editor_debug_text_get_count(void);
size_t editor_debug_text_get_memory_usage(void);
void editor_debug_text_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_DEBUG_TEXT_H */
