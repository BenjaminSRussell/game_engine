/*
 * ui_font.h
 * Font rendering
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RENDERING_UI_FONT_H
#define UI_RENDERING_UI_FONT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_rendering_ui_font_handle {
    uint32_t id;
} ui_rendering_ui_font_handle_t;

typedef struct ui_rendering_ui_font_desc {
    uint32_t flags;
    void* user_data;
} ui_rendering_ui_font_desc_t;

typedef struct ui_rendering_ui_font_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} ui_rendering_ui_font_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int ui_rendering_ui_font_init(void);
void ui_rendering_ui_font_shutdown(void);

/* Lifecycle */
int ui_rendering_ui_font_create(ui_rendering_ui_font_handle_t* out_handle, const ui_rendering_ui_font_desc_t* desc);
void ui_rendering_ui_font_destroy(ui_rendering_ui_font_handle_t handle);

/* Operations */
int ui_rendering_ui_font_update(ui_rendering_ui_font_handle_t handle, const void* data, size_t size);
bool ui_rendering_ui_font_is_valid(ui_rendering_ui_font_handle_t handle);
int ui_rendering_ui_font_get_info(ui_rendering_ui_font_handle_t handle, ui_rendering_ui_font_info_t* out_info);
void ui_rendering_ui_font_mark_dirty(ui_rendering_ui_font_handle_t handle);
int ui_rendering_ui_font_process_pending(void);

/* Statistics */
uint32_t ui_rendering_ui_font_get_count(void);
size_t ui_rendering_ui_font_get_memory_usage(void);
void ui_rendering_ui_font_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_FONT_H */
