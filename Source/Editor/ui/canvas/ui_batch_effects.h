/*
 * ui_batch_effects.h
 * Visual effects for UI batching
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Provides visual effects support including gradients, shadows,
 * blur, glow, distortion, and advanced compositing effects.
 */

#ifndef UI_RENDERING_UI_BATCH_EFFECTS_H
#define UI_RENDERING_UI_BATCH_EFFECTS_H

#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Effect types */
enum ui_batch_effect_type {
    UI_EFFECT_NONE = 0,
    UI_EFFECT_GRADIENT,
    UI_EFFECT_SHADOW,
    UI_EFFECT_BLUR,
    UI_EFFECT_GLOW,
    UI_EFFECT_DISTORTION,
    UI_EFFECT_OUTLINE,
    UI_EFFECT_CORNER_RADIUS,
    UI_EFFECT_MASK,
    UI_EFFECT_NINEPATCH,
};

/* Gradient descriptor */
typedef struct ui_batch_gradient {
    enum ui_batch_effect_type type;
    uint32_t start_color;
    uint32_t end_color;
    uint32_t gradient_type;  /* Linear, radial, etc. */
    float angle;              /* For linear gradient */
} ui_batch_gradient_t;

/* Shadow descriptor */
typedef struct ui_batch_shadow {
    enum ui_batch_effect_type type;
    float blur_radius;
    float offset_x;
    float offset_y;
    uint32_t color;
    float opacity;
} ui_batch_shadow_t;

/* Blur effect descriptor */
typedef struct ui_batch_blur {
    enum ui_batch_effect_type type;
    float blur_radius;
    uint32_t blur_quality;  /* 0=low, 1=medium, 2=high */
} ui_batch_blur_t;

/* Glow effect descriptor */
typedef struct ui_batch_glow {
    enum ui_batch_effect_type type;
    float glow_radius;
    float glow_intensity;
    uint32_t glow_color;
} ui_batch_glow_t;

/* Outline descriptor */
typedef struct ui_batch_outline {
    enum ui_batch_effect_type type;
    float width;
    uint32_t color;
    float softness;
} ui_batch_outline_t;

/* Corner radius descriptor */
typedef struct ui_batch_corner_radius {
    enum ui_batch_effect_type type;
    float radius;
    uint32_t corner_flags;  /* Which corners to apply to */
} ui_batch_corner_radius_t;

/* ============================================================================
 * API - EFFECT SETUP
 * ============================================================================ */

int ui_batch_effect_init(void);
void ui_batch_effect_shutdown(void);

/* ============================================================================
 * API - GRADIENT EFFECTS
 * ============================================================================ */

/* Add gradient to batch */
int ui_batch_effect_add_gradient(ui_rendering_ui_batch_handle_t batch_handle,
                                  float x, float y, float width, float height,
                                  const ui_batch_gradient_t* gradient);

/* Create linear gradient fill */
int ui_batch_effect_create_linear_gradient(float angle,
                                            uint32_t start_color,
                                            uint32_t end_color,
                                            ui_batch_gradient_t* out_gradient);

/* Create radial gradient fill */
int ui_batch_effect_create_radial_gradient(uint32_t center_color,
                                            uint32_t edge_color,
                                            ui_batch_gradient_t* out_gradient);

/* ============================================================================
 * API - SHADOW & GLOW
 * ============================================================================ */

/* Add shadow effect */
int ui_batch_effect_add_shadow(ui_rendering_ui_batch_handle_t batch_handle,
                                float x, float y, float width, float height,
                                const ui_batch_shadow_t* shadow);

/* Add glow effect */
int ui_batch_effect_add_glow(ui_rendering_ui_batch_handle_t batch_handle,
                              float x, float y, float width, float height,
                              const ui_batch_glow_t* glow);

/* ============================================================================
 * API - BLUR & DISTORTION
 * ============================================================================ */

/* Add blur effect */
int ui_batch_effect_add_blur(ui_rendering_ui_batch_handle_t batch_handle,
                              float x, float y, float width, float height,
                              const ui_batch_blur_t* blur);

/* ============================================================================
 * API - OUTLINES & SHAPES
 * ============================================================================ */

/* Add outline effect */
int ui_batch_effect_add_outline(ui_rendering_ui_batch_handle_t batch_handle,
                                 float x, float y, float width, float height,
                                 const ui_batch_outline_t* outline);

/* Add rounded rectangle with corner radius */
int ui_batch_effect_add_rounded_rect(ui_rendering_ui_batch_handle_t batch_handle,
                                      float x, float y, float width, float height,
                                      const ui_batch_corner_radius_t* radius);

/* Add 9-patch scaled widget */
int ui_batch_effect_add_ninepatch(ui_rendering_ui_batch_handle_t batch_handle,
                                   float x, float y, float width, float height,
                                   uint32_t texture_id,
                                   const float* border_sizes);

/* ============================================================================
 * API - EFFECT PROPERTIES
 * ============================================================================ */

/* Set effect blend mode */
int ui_batch_effect_set_blend_mode(ui_rendering_ui_batch_handle_t batch_handle,
                                    uint32_t blend_mode);

/* Set effect opacity */
int ui_batch_effect_set_opacity(ui_rendering_ui_batch_handle_t batch_handle,
                                 float opacity);

/* Set effect color */
int ui_batch_effect_set_color(ui_rendering_ui_batch_handle_t batch_handle,
                               uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_EFFECTS_H */
