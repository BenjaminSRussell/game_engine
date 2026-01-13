/*
 * ui_batch_text.h
 * UI text rendering for batched geometry
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Provides text rendering capabilities including SDF (Signed Distance Field)
 * text, glyph atlasing, and advanced text layout for UI batching.
 */

#ifndef UI_RENDERING_UI_BATCH_TEXT_H
#define UI_RENDERING_UI_BATCH_TEXT_H

#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Font handle */
typedef struct ui_batch_text_font_handle {
    uint32_t id;
} ui_batch_text_font_handle_t;

/* Glyph metrics */
typedef struct ui_batch_text_glyph_metrics {
    uint32_t glyph_id;
    float width;
    float height;
    float advance;
    float bearing_x;
    float bearing_y;
    float u0, v0, u1, v1;  /* Atlas UV coordinates */
} ui_batch_text_glyph_metrics_t;

/* Font descriptor */
typedef struct ui_batch_text_font_desc {
    const char* font_name;
    uint32_t font_size;
    uint32_t atlas_width;
    uint32_t atlas_height;
    void* font_data;
    size_t font_data_size;
} ui_batch_text_font_desc_t;

/* Text layout info */
typedef struct ui_batch_text_layout {
    float x, y;
    float width, height;
    uint32_t alignment;
    float line_height;
    bool word_wrap;
    bool kerning_enabled;
} ui_batch_text_layout_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int ui_batch_text_init(void);
void ui_batch_text_shutdown(void);

/* ============================================================================
 * API - FONT MANAGEMENT
 * ============================================================================ */

/* Create font from descriptor */
int ui_batch_text_create_font(ui_batch_text_font_handle_t* out_handle,
                               const ui_batch_text_font_desc_t* desc);

/* Destroy font */
void ui_batch_text_destroy_font(ui_batch_text_font_handle_t handle);

/* Get glyph metrics for a character */
int ui_batch_text_get_glyph(ui_batch_text_font_handle_t font,
                             uint32_t codepoint,
                             ui_batch_text_glyph_metrics_t* out_glyph);

/* ============================================================================
 * API - TEXT RENDERING
 * ============================================================================ */

/* Add text to batch with layout */
int ui_batch_text_add_text(ui_rendering_ui_batch_handle_t batch_handle,
                            ui_batch_text_font_handle_t font_handle,
                            const char* text,
                            const ui_batch_text_layout_t* layout,
                            uint32_t color);

/* Add single line of text */
int ui_batch_text_add_line(ui_rendering_ui_batch_handle_t batch_handle,
                            ui_batch_text_font_handle_t font_handle,
                            const char* text,
                            float x, float y,
                            uint32_t color);

/* Measure text dimensions */
int ui_batch_text_measure(ui_batch_text_font_handle_t font_handle,
                           const char* text,
                           float* out_width,
                           float* out_height);

/* ============================================================================
 * API - SDF TEXT
 * ============================================================================ */

/* Enable SDF rendering for font */
int ui_batch_text_enable_sdf(ui_batch_text_font_handle_t font_handle,
                              float sdf_scale);

/* Set SDF outline width */
int ui_batch_text_set_sdf_outline(ui_batch_text_font_handle_t font_handle,
                                   float outline_width,
                                   uint32_t outline_color);

/* ============================================================================
 * API - GLYPH ATLAS
 * ============================================================================ */

/* Get font atlas texture ID */
uint32_t ui_batch_text_get_atlas_texture(ui_batch_text_font_handle_t font_handle);

/* Rebuild glyph atlas */
int ui_batch_text_rebuild_atlas(ui_batch_text_font_handle_t font_handle);

/* Get atlas dimensions */
int ui_batch_text_get_atlas_size(ui_batch_text_font_handle_t font_handle,
                                  uint32_t* out_width,
                                  uint32_t* out_height);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_TEXT_H */
