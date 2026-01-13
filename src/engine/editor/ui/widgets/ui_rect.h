/*
 * ui_rect.h
 * Rectangle rendering
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RENDERING_UI_RECT_H
#define UI_RENDERING_UI_RECT_H

#include "engine/include/math/math.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct ui_rendering_ui_rect_handle {
  uint32_t id;
} ui_rendering_ui_rect_handle_t;

typedef struct ui_rendering_ui_rect_desc {
  float x, y;
  float width, height;
  float color[4];
  uint32_t flags;
  void *user_data;
} ui_rendering_ui_rect_desc_t;

typedef struct ui_rendering_ui_rect_info {
  uint32_t id;
  uint32_t flags;
  bool initialized;
} ui_rendering_ui_rect_info_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int ui_rendering_ui_rect_init(void);
void ui_rendering_ui_rect_shutdown(void);

/* Lifecycle */
int ui_rendering_ui_rect_create(ui_rendering_ui_rect_handle_t *out_handle,
                                const ui_rendering_ui_rect_desc_t *desc);
void ui_rendering_ui_rect_destroy(ui_rendering_ui_rect_handle_t handle);

/* Operations */
int ui_rendering_ui_rect_update(ui_rendering_ui_rect_handle_t handle,
                                const ui_rendering_ui_rect_desc_t *desc);
void ui_rendering_ui_rect_get_render_data(ui_rendering_ui_rect_handle_t handle,
                                          Vec2 *pos, Vec2 *size, Vec4 *color);

/* Statistics */
uint32_t ui_rendering_ui_rect_get_count(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_RECT_H */
