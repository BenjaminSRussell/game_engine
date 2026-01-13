/*
 * ui_batch_effects.c
 * Visual effects for UI batching
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "editor/ui/canvas/ui_batch_effects.h"
#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_BATCH_EFFECT_MAX_EFFECTS 1024

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_batch_effect_context {
    bool initialized;
} ui_batch_effect_context_t;

static ui_batch_effect_context_t g_effect_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void* ui_batch_effect_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_effect_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* Create quad vertices for rectangle */
static void ui_batch_effect_create_quad(float x, float y, float w, float h,
                                         uint32_t color,
                                         ui_rendering_vertex_t* out_vertices) {
    out_vertices[0] = (ui_rendering_vertex_t){ x, y, 0, 0, 0, color, 0, 0 };
    out_vertices[1] = (ui_rendering_vertex_t){ x + w, y, 0, 1, 0, color, 0, 0 };
    out_vertices[2] = (ui_rendering_vertex_t){ x + w, y + h, 0, 1, 1, color, 0, 0 };
    out_vertices[3] = (ui_rendering_vertex_t){ x, y + h, 0, 0, 1, color, 0, 0 };
}

/* Create rounded rect with corner radius */
static int ui_batch_effect_create_rounded_rect_geometry(float x, float y,
                                                         float w, float h,
                                                         float radius,
                                                         uint32_t color,
                                                         ui_rendering_vertex_t** out_vertices,
                                                         uint32_t* out_vertex_count,
                                                         uint32_t** out_indices,
                                                         uint32_t* out_index_count) {
    if (!out_vertices || !out_vertex_count || !out_indices || !out_index_count) {
        return -1;
    }

    /* Clamp radius to half of smaller dimension */
    float max_radius = (w < h ? w : h) / 2.0f;
    if (radius > max_radius) {
        radius = max_radius;
    }

    /* Create vertices for rounded rectangle */
    /* For simplicity, create as quad with corner adjustments */
    uint32_t vertex_count = 4;
    ui_rendering_vertex_t* vertices = ui_batch_effect_malloc(vertex_count * sizeof(ui_rendering_vertex_t));
    if (!vertices) {
        return -2;
    }

    ui_batch_effect_create_quad(x, y, w, h, color, vertices);

    uint32_t index_count = 6;
    uint32_t* indices = ui_batch_effect_malloc(index_count * sizeof(uint32_t));
    if (!indices) {
        ui_batch_effect_free(vertices);
        return -3;
    }

    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    *out_vertices = vertices;
    *out_vertex_count = vertex_count;
    *out_indices = indices;
    *out_index_count = index_count;

    return 0;
}

/* ============================================================================
 * PUBLIC API - EFFECT SETUP
 * ============================================================================ */

int ui_batch_effect_init(void) {
    if (g_effect_ctx.initialized) {
        return 0;
    }

    g_effect_ctx.initialized = true;
    return 0;
}

void ui_batch_effect_shutdown(void) {
    if (!g_effect_ctx.initialized) {
        return;
    }

    g_effect_ctx.initialized = false;
}

/* ============================================================================
 * PUBLIC API - GRADIENT EFFECTS
 * ============================================================================ */

int ui_batch_effect_add_gradient(ui_rendering_ui_batch_handle_t batch_handle,
                                  float x, float y, float width, float height,
                                  const ui_batch_gradient_t* gradient) {
    if (!gradient) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create gradient quad */
    ui_rendering_vertex_t vertices[4];
    ui_batch_effect_create_quad(x, y, width, height, gradient->start_color, vertices);

    /* Blend end color into second half for gradient effect */
    vertices[1].color = gradient->end_color;
    vertices[2].color = gradient->end_color;

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

int ui_batch_effect_create_linear_gradient(float angle,
                                            uint32_t start_color,
                                            uint32_t end_color,
                                            ui_batch_gradient_t* out_gradient) {
    if (!out_gradient) {
        return -1;
    }

    out_gradient->type = UI_EFFECT_GRADIENT;
    out_gradient->start_color = start_color;
    out_gradient->end_color = end_color;
    out_gradient->gradient_type = 0; /* Linear */
    out_gradient->angle = angle;

    return 0;
}

int ui_batch_effect_create_radial_gradient(uint32_t center_color,
                                            uint32_t edge_color,
                                            ui_batch_gradient_t* out_gradient) {
    if (!out_gradient) {
        return -1;
    }

    out_gradient->type = UI_EFFECT_GRADIENT;
    out_gradient->start_color = center_color;
    out_gradient->end_color = edge_color;
    out_gradient->gradient_type = 1; /* Radial */
    out_gradient->angle = 0;

    return 0;
}

/* ============================================================================
 * PUBLIC API - SHADOW & GLOW
 * ============================================================================ */

int ui_batch_effect_add_shadow(ui_rendering_ui_batch_handle_t batch_handle,
                                float x, float y, float width, float height,
                                const ui_batch_shadow_t* shadow) {
    if (!shadow) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create shadow quad offset from main quad */
    float shadow_x = x + shadow->offset_x;
    float shadow_y = y + shadow->offset_y;

    /* Blend opacity into color */
    uint8_t alpha = (uint8_t)((shadow->opacity * 255.0f) * ((shadow->color >> 24) & 0xFF) / 255.0f);
    uint32_t shadow_color = (shadow->color & 0xFFFFFF) | (alpha << 24);

    ui_rendering_vertex_t vertices[4];
    ui_batch_effect_create_quad(shadow_x, shadow_y, width, height, shadow_color, vertices);

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

int ui_batch_effect_add_glow(ui_rendering_ui_batch_handle_t batch_handle,
                              float x, float y, float width, float height,
                              const ui_batch_glow_t* glow) {
    if (!glow) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create glow quad with intensity factored into color */
    ui_rendering_vertex_t vertices[4];

    uint8_t r = (glow->glow_color >> 16) & 0xFF;
    uint8_t g = (glow->glow_color >> 8) & 0xFF;
    uint8_t b = glow->glow_color & 0xFF;

    uint8_t intensity = (uint8_t)(glow->glow_intensity * 255.0f);
    uint32_t glow_color = (r << 16) | (g << 8) | b | (intensity << 24);

    ui_batch_effect_create_quad(x, y, width, height, glow_color, vertices);

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

/* ============================================================================
 * PUBLIC API - BLUR & DISTORTION
 * ============================================================================ */

int ui_batch_effect_add_blur(ui_rendering_ui_batch_handle_t batch_handle,
                              float x, float y, float width, float height,
                              const ui_batch_blur_t* blur) {
    if (!blur) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create blurred quad - actual blur would be done in shader */
    ui_rendering_vertex_t vertices[4];
    ui_batch_effect_create_quad(x, y, width, height, 0xFFFFFFFF, vertices);

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

/* ============================================================================
 * PUBLIC API - OUTLINES & SHAPES
 * ============================================================================ */

int ui_batch_effect_add_outline(ui_rendering_ui_batch_handle_t batch_handle,
                                 float x, float y, float width, float height,
                                 const ui_batch_outline_t* outline) {
    if (!outline) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create outline quad */
    ui_rendering_vertex_t vertices[4];
    ui_batch_effect_create_quad(x - outline->width / 2.0f, y - outline->width / 2.0f,
                                 width + outline->width, height + outline->width,
                                 outline->color, vertices);

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

int ui_batch_effect_add_rounded_rect(ui_rendering_ui_batch_handle_t batch_handle,
                                      float x, float y, float width, float height,
                                      const ui_batch_corner_radius_t* radius) {
    if (!radius) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    ui_rendering_vertex_t* vertices = NULL;
    uint32_t vertex_count = 0;
    uint32_t* indices = NULL;
    uint32_t index_count = 0;

    if (ui_batch_effect_create_rounded_rect_geometry(x, y, width, height,
                                                      radius->radius, 0xFFFFFFFF,
                                                      &vertices, &vertex_count,
                                                      &indices, &index_count) != 0) {
        return -3;
    }

    int result = ui_rendering_ui_batch_add_geometry(batch_handle, vertices, vertex_count,
                                                     indices, index_count);

    ui_batch_effect_free(vertices);
    ui_batch_effect_free(indices);

    return result;
}

int ui_batch_effect_add_ninepatch(ui_rendering_ui_batch_handle_t batch_handle,
                                   float x, float y, float width, float height,
                                   uint32_t texture_id,
                                   const float* border_sizes) {
    if (!border_sizes) {
        return -1;
    }

    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -2;
    }

    /* Create 9-patch geometry - simplified as single quad for now */
    ui_rendering_vertex_t vertices[4];
    ui_batch_effect_create_quad(x, y, width, height, 0xFFFFFFFF, vertices);

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    return ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);
}

/* ============================================================================
 * PUBLIC API - EFFECT PROPERTIES
 * ============================================================================ */

int ui_batch_effect_set_blend_mode(ui_rendering_ui_batch_handle_t batch_handle,
                                    uint32_t blend_mode) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    ui_rendering_ui_batch_mark_dirty(batch_handle);
    return 0;
}

int ui_batch_effect_set_opacity(ui_rendering_ui_batch_handle_t batch_handle,
                                 float opacity) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    if (opacity < 0.0f || opacity > 1.0f) {
        return -2;
    }

    ui_rendering_ui_batch_mark_dirty(batch_handle);
    return 0;
}

int ui_batch_effect_set_color(ui_rendering_ui_batch_handle_t batch_handle,
                               uint32_t color) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    ui_rendering_ui_batch_mark_dirty(batch_handle);
    return 0;
}

/* End of ui_batch_effects.c */
