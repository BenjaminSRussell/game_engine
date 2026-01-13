/*
 * ui_batch_text.c
 * UI text rendering for batched geometry
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "editor/ui/canvas/ui_batch_text.h"
#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_BATCH_TEXT_MAX_FONTS 256
#define UI_BATCH_TEXT_MAX_GLYPHS 4096
#define UI_BATCH_TEXT_DEFAULT_ATLAS_SIZE 2048

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_batch_text_glyph {
    uint32_t codepoint;
    ui_batch_text_glyph_metrics_t metrics;
} ui_batch_text_glyph_t;

typedef struct ui_batch_text_font {
    uint32_t id;
    char font_name[256];
    uint32_t font_size;

    ui_batch_text_glyph_t* glyphs;
    uint32_t glyph_count;
    uint32_t glyph_capacity;

    uint32_t atlas_texture_id;
    uint32_t atlas_width;
    uint32_t atlas_height;

    bool sdf_enabled;
    float sdf_scale;
    float sdf_outline_width;
    uint32_t sdf_outline_color;

    bool valid;
} ui_batch_text_font_t;

typedef struct ui_batch_text_context {
    ui_batch_text_font_t* fonts;
    uint32_t font_count;
    uint32_t font_capacity;

    bool initialized;
} ui_batch_text_context_t;

static ui_batch_text_context_t g_text_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void* ui_batch_text_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_text_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

static int ui_batch_text_validate_font(const ui_batch_text_font_t* font) {
    if (!font) return 0;
    if (!font->valid) return 0;
    if (font->font_size == 0) return 0;
    return 1;
}

static uint32_t ui_batch_text_utf8_to_codepoint(const uint8_t* src, uint32_t* out_advance) {
    if (!src) {
        *out_advance = 0;
        return 0xFFFD; /* Replacement character */
    }

    uint8_t byte = src[0];

    if ((byte & 0x80) == 0) {
        /* ASCII */
        *out_advance = 1;
        return byte;
    }

    if ((byte & 0xE0) == 0xC0) {
        /* 2-byte sequence */
        *out_advance = 2;
        uint32_t cp = ((byte & 0x1F) << 6) | (src[1] & 0x3F);
        return cp;
    }

    if ((byte & 0xF0) == 0xE0) {
        /* 3-byte sequence */
        *out_advance = 3;
        uint32_t cp = ((byte & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
        return cp;
    }

    if ((byte & 0xF8) == 0xF0) {
        /* 4-byte sequence */
        *out_advance = 4;
        uint32_t cp = ((byte & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
        return cp;
    }

    *out_advance = 1;
    return 0xFFFD;
}

static int ui_batch_text_find_glyph(const ui_batch_text_font_t* font, uint32_t codepoint,
                                     ui_batch_text_glyph_t** out_glyph) {
    if (!ui_batch_text_validate_font(font)) {
        return -1;
    }

    for (uint32_t i = 0; i < font->glyph_count; i++) {
        if (font->glyphs[i].codepoint == codepoint) {
            *out_glyph = &font->glyphs[i];
            return 0;
        }
    }

    return -2; /* Not found */
}

static int ui_batch_text_add_glyph(ui_batch_text_font_t* font, uint32_t codepoint) {
    if (!ui_batch_text_validate_font(font)) {
        return -1;
    }

    /* Check if already exists */
    ui_batch_text_glyph_t* existing = NULL;
    if (ui_batch_text_find_glyph(font, codepoint, &existing) == 0) {
        return 0; /* Already have this glyph */
    }

    if (font->glyph_count >= font->glyph_capacity) {
        uint32_t new_capacity = font->glyph_capacity * 2;
        if (new_capacity == 0) new_capacity = 256;

        ui_batch_text_glyph_t* new_glyphs = ui_batch_text_malloc(new_capacity * sizeof(ui_batch_text_glyph_t));
        if (!new_glyphs) {
            return -2;
        }

        if (font->glyph_count > 0) {
            memcpy(new_glyphs, font->glyphs, font->glyph_count * sizeof(ui_batch_text_glyph_t));
        }

        ui_batch_text_free(font->glyphs);
        font->glyphs = new_glyphs;
        font->glyph_capacity = new_capacity;
    }

    ui_batch_text_glyph_t* glyph = &font->glyphs[font->glyph_count++];
    glyph->codepoint = codepoint;

    /* Initialize glyph metrics with placeholder values */
    glyph->metrics.glyph_id = codepoint;
    glyph->metrics.width = font->font_size * 0.5f;
    glyph->metrics.height = font->font_size;
    glyph->metrics.advance = font->font_size * 0.5f;
    glyph->metrics.bearing_x = 0;
    glyph->metrics.bearing_y = font->font_size;
    glyph->metrics.u0 = 0;
    glyph->metrics.v0 = 0;
    glyph->metrics.u1 = 1;
    glyph->metrics.v1 = 1;

    return 0;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int ui_batch_text_init(void) {
    if (g_text_ctx.initialized) {
        return 0;
    }

    g_text_ctx.font_capacity = UI_BATCH_TEXT_MAX_FONTS;
    g_text_ctx.fonts = ui_batch_text_malloc(g_text_ctx.font_capacity * sizeof(ui_batch_text_font_t));
    if (!g_text_ctx.fonts) {
        return -1;
    }

    g_text_ctx.font_count = 0;
    g_text_ctx.initialized = true;

    return 0;
}

void ui_batch_text_shutdown(void) {
    if (!g_text_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_text_ctx.font_count; i++) {
        ui_batch_text_font_t* font = &g_text_ctx.fonts[i];
        if (font->valid) {
            ui_batch_text_free(font->glyphs);
            font->valid = false;
        }
    }

    ui_batch_text_free(g_text_ctx.fonts);
    g_text_ctx.fonts = NULL;
    g_text_ctx.font_count = 0;
    g_text_ctx.font_capacity = 0;
    g_text_ctx.initialized = false;
}

/* ============================================================================
 * PUBLIC API - FONT MANAGEMENT
 * ============================================================================ */

int ui_batch_text_create_font(ui_batch_text_font_handle_t* out_handle,
                               const ui_batch_text_font_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_text_ctx.initialized) {
        return -2;
    }

    if (g_text_ctx.font_count >= g_text_ctx.font_capacity) {
        return -3;
    }

    uint32_t index = g_text_ctx.font_count++;
    ui_batch_text_font_t* font = &g_text_ctx.fonts[index];

    font->id = index;
    font->font_size = desc->font_size;
    font->atlas_width = desc->atlas_width > 0 ? desc->atlas_width : UI_BATCH_TEXT_DEFAULT_ATLAS_SIZE;
    font->atlas_height = desc->atlas_height > 0 ? desc->atlas_height : UI_BATCH_TEXT_DEFAULT_ATLAS_SIZE;

    if (desc->font_name) {
        strncpy(font->font_name, desc->font_name, sizeof(font->font_name) - 1);
    }

    font->glyph_capacity = 256;
    font->glyphs = ui_batch_text_malloc(font->glyph_capacity * sizeof(ui_batch_text_glyph_t));
    if (!font->glyphs) {
        g_text_ctx.font_count--;
        return -4;
    }

    font->glyph_count = 0;
    font->atlas_texture_id = 0;
    font->sdf_enabled = false;
    font->sdf_scale = 1.0f;
    font->sdf_outline_width = 0;
    font->sdf_outline_color = 0;
    font->valid = true;

    out_handle->id = index;
    return 0;
}

void ui_batch_text_destroy_font(ui_batch_text_font_handle_t handle) {
    if (handle.id >= g_text_ctx.font_count) {
        return;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[handle.id];
    if (!font->valid) {
        return;
    }

    ui_batch_text_free(font->glyphs);
    font->valid = false;
}

int ui_batch_text_get_glyph(ui_batch_text_font_handle_t font_handle,
                             uint32_t codepoint,
                             ui_batch_text_glyph_metrics_t* out_glyph) {
    if (!out_glyph) {
        return -1;
    }

    if (font_handle.id >= g_text_ctx.font_count) {
        return -2;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -3;
    }

    ui_batch_text_glyph_t* glyph = NULL;
    if (ui_batch_text_find_glyph(font, codepoint, &glyph) != 0) {
        if (ui_batch_text_add_glyph(font, codepoint) != 0) {
            return -4;
        }
        glyph = &font->glyphs[font->glyph_count - 1];
    }

    memcpy(out_glyph, &glyph->metrics, sizeof(ui_batch_text_glyph_metrics_t));
    return 0;
}

/* ============================================================================
 * PUBLIC API - TEXT RENDERING
 * ============================================================================ */

int ui_batch_text_add_text(ui_rendering_ui_batch_handle_t batch_handle,
                            ui_batch_text_font_handle_t font_handle,
                            const char* text,
                            const ui_batch_text_layout_t* layout,
                            uint32_t color) {
    if (!text || !layout) {
        return -1;
    }

    if (font_handle.id >= g_text_ctx.font_count) {
        return -2;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -3;
    }

    float x = layout->x;
    float y = layout->y;
    const uint8_t* src = (const uint8_t*)text;

    while (*src) {
        uint32_t advance;
        uint32_t codepoint = ui_batch_text_utf8_to_codepoint(src, &advance);
        src += advance;

        ui_batch_text_glyph_metrics_t glyph_metrics;
        if (ui_batch_text_get_glyph(font_handle, codepoint, &glyph_metrics) != 0) {
            continue; /* Skip unmappable characters */
        }

        /* Create quad vertices for glyph */
        float w = glyph_metrics.width;
        float h = glyph_metrics.height;

        ui_rendering_vertex_t vertices[4] = {
            { x, y, 0, glyph_metrics.u0, glyph_metrics.v0, color, 0, 0 },
            { x + w, y, 0, glyph_metrics.u1, glyph_metrics.v0, color, 0, 0 },
            { x + w, y + h, 0, glyph_metrics.u1, glyph_metrics.v1, color, 0, 0 },
            { x, y + h, 0, glyph_metrics.u0, glyph_metrics.v1, color, 0, 0 },
        };

        uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

        /* Add to batch */
        ui_rendering_ui_batch_add_geometry(batch_handle, vertices, 4, indices, 6);

        x += glyph_metrics.advance;
    }

    return 0;
}

int ui_batch_text_add_line(ui_rendering_ui_batch_handle_t batch_handle,
                            ui_batch_text_font_handle_t font_handle,
                            const char* text,
                            float x, float y,
                            uint32_t color) {
    ui_batch_text_layout_t layout = {
        .x = x,
        .y = y,
        .width = 10000,
        .height = 0,
        .alignment = 0,
        .line_height = 0,
        .word_wrap = false,
        .kerning_enabled = false,
    };

    return ui_batch_text_add_text(batch_handle, font_handle, text, &layout, color);
}

int ui_batch_text_measure(ui_batch_text_font_handle_t font_handle,
                           const char* text,
                           float* out_width,
                           float* out_height) {
    if (!text || !out_width || !out_height) {
        return -1;
    }

    if (font_handle.id >= g_text_ctx.font_count) {
        return -2;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -3;
    }

    *out_width = 0;
    *out_height = (float)font->font_size;

    const uint8_t* src = (const uint8_t*)text;
    while (*src) {
        uint32_t advance;
        uint32_t codepoint = ui_batch_text_utf8_to_codepoint(src, &advance);
        src += advance;

        ui_batch_text_glyph_metrics_t glyph_metrics;
        if (ui_batch_text_get_glyph(font_handle, codepoint, &glyph_metrics) != 0) {
            continue;
        }

        *out_width += glyph_metrics.advance;
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API - SDF TEXT
 * ============================================================================ */

int ui_batch_text_enable_sdf(ui_batch_text_font_handle_t font_handle,
                              float sdf_scale) {
    if (font_handle.id >= g_text_ctx.font_count) {
        return -1;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -2;
    }

    font->sdf_enabled = true;
    font->sdf_scale = sdf_scale > 0 ? sdf_scale : 1.0f;

    return 0;
}

int ui_batch_text_set_sdf_outline(ui_batch_text_font_handle_t font_handle,
                                   float outline_width,
                                   uint32_t outline_color) {
    if (font_handle.id >= g_text_ctx.font_count) {
        return -1;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -2;
    }

    font->sdf_outline_width = outline_width;
    font->sdf_outline_color = outline_color;

    return 0;
}

/* ============================================================================
 * PUBLIC API - GLYPH ATLAS
 * ============================================================================ */

uint32_t ui_batch_text_get_atlas_texture(ui_batch_text_font_handle_t font_handle) {
    if (font_handle.id >= g_text_ctx.font_count) {
        return 0;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return 0;
    }

    return font->atlas_texture_id;
}

int ui_batch_text_rebuild_atlas(ui_batch_text_font_handle_t font_handle) {
    if (font_handle.id >= g_text_ctx.font_count) {
        return -1;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -2;
    }

    /* Atlas rebuild logic would go here */

    return 0;
}

int ui_batch_text_get_atlas_size(ui_batch_text_font_handle_t font_handle,
                                  uint32_t* out_width,
                                  uint32_t* out_height) {
    if (!out_width || !out_height) {
        return -1;
    }

    if (font_handle.id >= g_text_ctx.font_count) {
        return -2;
    }

    ui_batch_text_font_t* font = &g_text_ctx.fonts[font_handle.id];
    if (!ui_batch_text_validate_font(font)) {
        return -3;
    }

    *out_width = font->atlas_width;
    *out_height = font->atlas_height;

    return 0;
}

/* End of ui_batch_text.c */
