/*
 * ui_gradients.h
 * Gradient fills
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RENDERING_UI_GRADIENTS_H
#define UI_RENDERING_UI_GRADIENTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_rendering_ui_gradients_handle {
    uint32_t id;
} ui_rendering_ui_gradients_handle_t;

typedef struct ui_rendering_ui_gradients_desc {
    uint32_t flags;
    void* user_data;
} ui_rendering_ui_gradients_desc_t;

typedef struct ui_rendering_ui_gradients_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} ui_rendering_ui_gradients_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int ui_rendering_ui_gradients_init(void);
void ui_rendering_ui_gradients_shutdown(void);

/* Lifecycle */
int ui_rendering_ui_gradients_create(ui_rendering_ui_gradients_handle_t* out_handle, const ui_rendering_ui_gradients_desc_t* desc);
void ui_rendering_ui_gradients_destroy(ui_rendering_ui_gradients_handle_t handle);

/* Operations */
int ui_rendering_ui_gradients_update(ui_rendering_ui_gradients_handle_t handle, const void* data, size_t size);
bool ui_rendering_ui_gradients_is_valid(ui_rendering_ui_gradients_handle_t handle);
int ui_rendering_ui_gradients_get_info(ui_rendering_ui_gradients_handle_t handle, ui_rendering_ui_gradients_info_t* out_info);
void ui_rendering_ui_gradients_mark_dirty(ui_rendering_ui_gradients_handle_t handle);
int ui_rendering_ui_gradients_process_pending(void);

/* Statistics */
uint32_t ui_rendering_ui_gradients_get_count(void);
size_t ui_rendering_ui_gradients_get_memory_usage(void);
void ui_rendering_ui_gradients_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_GRADIENTS_H */
