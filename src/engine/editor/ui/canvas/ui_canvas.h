/*
 * ui_canvas.h
 * UI canvas system
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RENDERING_UI_CANVAS_H
#define UI_RENDERING_UI_CANVAS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int ui_rendering_ui_canvas_init(void);
void ui_rendering_ui_canvas_shutdown(void);

/* Rendering - Immediate Mode Layer */
void ui_canvas_begin(void);
void ui_canvas_end(void);

/* Primitives */
void ui_canvas_draw_rect(float x, float y, float w, float h, float r, float g,
                         float b, float a);

/* Statistics */
uint32_t ui_rendering_ui_canvas_get_count(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_CANVAS_H */
