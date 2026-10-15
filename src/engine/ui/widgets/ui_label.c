/*
 * ui_label.c
 * Label Widget Implementation
 * Text display widget for UI labels and messages
 */

#include "ui_label.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Text backend installed through ui_label_set_text_backend. Labels draw
 * nothing while it is NULL. */
static UILabelDrawTextFn g_label_draw_text = NULL;
static void* g_label_draw_context = NULL;

/* Rough glyph metrics: the 0.6em advance and +4 line spacing match the
 * estimates in ui_label_measure_impl. Real font metrics would replace both. */
static float ui_label_char_advance(const UILabel* label) {
    return label->font_size * 0.6f;
}

static float ui_label_line_height(const UILabel* label) {
    return label->font_size + 4.0f;
}

static uint32_t ui_label_max_visible(const UILabel* label) {
    return label->max_lines > 0 ? label->max_lines : UINT32_MAX;
}

/* Characters that fit across the content box, never fewer than one. */
static uint32_t ui_label_fit_chars(const UILabel* label) {
    Vec2 content = widget_get_content_size(&label->base);
    float chars = content.x / ui_label_char_advance(label);
    if (!(chars >= 1.0f)) return 1;
    if (chars >= (float)UINT32_MAX) return UINT32_MAX;
    return (uint32_t)chars;
}

/* Called for each visible line. index counts visible lines from 0, and more is
 * true when text continues after this line. */
typedef void (*UILabelLineFn)(void* user, uint32_t index, uint32_t start,
                              uint32_t length, bool more);

/* Splits the text into lines. '\n' always ends a line; with word wrap on, a
 * paragraph is also cut every wrap_chars characters. Calls fn for lines below
 * max_visible (fn may be NULL) and returns the total number of lines. Layout
 * and drawing both use this walk, so the counted lines are the drawn lines. */
static uint32_t ui_label_walk_lines(const UILabel* label, uint32_t wrap_chars,
                                    uint32_t max_visible, UILabelLineFn fn,
                                    void* user) {
    const char* text = label->text;
    uint32_t text_length = label->text_length;
    uint32_t wrap = wrap_chars > 0 ? wrap_chars : 1;
    uint32_t count = 0;
    uint32_t para = 0;

    for (;;) {
        uint32_t para_end = para;
        while (para_end < text_length && text[para_end] != '\n') para_end++;
        bool last_para = para_end >= text_length;

        uint32_t start = para;
        do {  /* an empty paragraph still takes one line */
            uint32_t length = para_end - start;
            if (length > wrap) length = wrap;
            uint32_t next = start + length;
            bool more = next < para_end || !last_para;
            if (fn && count < max_visible) fn(user, count, start, length, more);
            count++;
            start = next;
        } while (start < para_end);

        if (last_para) break;
        para = para_end + 1;
    }
    return count;
}

static void ui_label_recalculate_lines(UILabel* label) {
    if (!label || !label->text) {
        label->calculated_lines = 0;
        return;
    }

    uint32_t wrap = label->word_wrap ? ui_label_fit_chars(label) : UINT32_MAX;
    uint32_t max_visible = ui_label_max_visible(label);
    uint32_t lines = ui_label_walk_lines(label, wrap, max_visible, NULL, NULL);
    label->calculated_lines = lines < max_visible ? lines : max_visible;
}

/* State shared by the line callback during one render call. */
typedef struct {
    UILabel* label;
    float origin_x;       /* left edge of the content box */
    float top_y;          /* top edge of the first line */
    float content_w;
    float advance;
    float line_height;
    uint32_t fit_chars;
    uint32_t max_visible;
    bool shadow_pass;
} UILabelDrawState;

static void ui_label_emit_run(const UILabel* label, const char* text, uint32_t length,
                              Vec2 position, Vec4 color) {
    if (g_label_draw_text && length > 0) {
        g_label_draw_text(g_label_draw_context, text, length, position,
                          label->font_size, color, label->format);
    }
}

static void ui_label_draw_line(void* user, uint32_t index, uint32_t start,
                               uint32_t length, bool more) {
    UILabelDrawState* state = user;
    const UILabel* label = state->label;

    /* With ellipsis on, clip the last visible line when text follows it, and
     * clip any line wider than the content box. The suffix counts toward width. */
    bool clip = label->ellipsis &&
                ((index + 1 == state->max_visible && more) || length > state->fit_chars);
    uint32_t kept = length;
    if (clip) {
        uint32_t budget = state->fit_chars > 3 ? state->fit_chars - 3 : 0;
        if (kept > budget) kept = budget;
    }
    float drawn_width = (float)(kept + (clip ? 3u : 0u)) * state->advance;

    float x = state->origin_x;
    if (label->alignment == LABEL_ALIGN_CENTER) {
        x += (state->content_w - drawn_width) * 0.5f;
    } else if (label->alignment == LABEL_ALIGN_RIGHT) {
        x += state->content_w - drawn_width;
    }
    if (x < state->origin_x) x = state->origin_x;

    Vec2 position = {x, state->top_y + (float)index * state->line_height};
    Vec4 color = label->text_color;
    if (state->shadow_pass) {
        position.x += label->shadow_offset_x;
        position.y += label->shadow_offset_y;
        color = label->shadow_color;
    }

    ui_label_emit_run(label, label->text + start, kept, position, color);
    if (clip) {
        Vec2 dots = {position.x + (float)kept * state->advance, position.y};
        ui_label_emit_run(label, "...", 3, dots, color);
    }
}

static void ui_label_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UILabel* label = (UILabel*)widget;
    Vec2 pos = widget_get_position(widget);
    Vec2 size = widget_get_size(widget);

    LOG_DEBUG_CAT(LOG_CAT_GENERAL, "Label render: pos=(%.1f, %.1f) size=(%.1f, %.1f) text=%s",
             pos.x, pos.y, size.x, size.y, label->text ? label->text : "");

    if (!g_label_draw_text || !label->text || label->text_length == 0) return;

    Vec2 content_pos = widget_get_content_position(widget);
    Vec2 content = widget_get_content_size(widget);
    float line_height = ui_label_line_height(label);
    uint32_t fit = ui_label_fit_chars(label);
    uint32_t max_visible = ui_label_max_visible(label);
    uint32_t wrap = label->word_wrap ? fit : UINT32_MAX;

    /* Vertical placement needs the visible line count before any line is drawn. */
    uint32_t lines = ui_label_walk_lines(label, wrap, 0, NULL, NULL);
    uint32_t visible = lines < max_visible ? lines : max_visible;
    float block_height = (float)visible * line_height;
    float top_offset = 0.0f;
    if (label->v_alignment == LABEL_VALIGN_CENTER) {
        top_offset = (content.y - block_height) * 0.5f;
    } else if (label->v_alignment == LABEL_VALIGN_BOTTOM) {
        top_offset = content.y - block_height;
    }
    if (top_offset < 0.0f) top_offset = 0.0f;

    UILabelDrawState state = {
        .label = label,
        .origin_x = pos.x + content_pos.x,
        .top_y = pos.y + content_pos.y + top_offset,
        .content_w = content.x,
        .advance = ui_label_char_advance(label),
        .line_height = line_height,
        .fit_chars = fit,
        .max_visible = max_visible,
        .shadow_pass = false,
    };

    if (label->show_shadow) {
        state.shadow_pass = true;
        ui_label_walk_lines(label, wrap, max_visible, ui_label_draw_line, &state);
        state.shadow_pass = false;
    }
    ui_label_walk_lines(label, wrap, max_visible, ui_label_draw_line, &state);
}

static void ui_label_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    UILabel* label = (UILabel*)widget;

    // Recalculate line count based on new dimensions
    ui_label_recalculate_lines(label);

    // Default layout: label takes requested size or fills available space
    Vec2 size = widget_get_size(widget);

    if (size.x <= 0) {
        size.x = available_width > 0 ? available_width : 200.0f;
    }
    if (size.y <= 0) {
        size.y = available_height > 0 ? available_height : 20.0f;
    }

    widget_set_size(widget, size);
    widget->needs_layout = false;
}

static Size ui_label_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};

    UILabel* label = (UILabel*)widget;

    // Rough text measurement - would need actual font metrics
    float text_width = 0;
    float text_height = label->font_size + 4.0f;

    if (label->text && label->text_length > 0) {
        text_width = label->text_length * (label->font_size * 0.6f);

        // Account for word wrapping
        if (label->word_wrap && available_width > 0) {
            uint32_t chars_per_line = (uint32_t)(available_width / (label->font_size * 0.6f));
            if (chars_per_line > 0) {
                uint32_t lines = (label->text_length + chars_per_line - 1) / chars_per_line;
                if (label->max_lines > 0 && lines > label->max_lines) {
                    lines = label->max_lines;
                }
                text_height = lines * (label->font_size + 4.0f);
                text_width = fminf(text_width, available_width);
            }
        }
    }

    // Add padding
    text_width += 4.0f;
    text_height += 2.0f;

    return (Size){
        .x = fminf(text_width, available_width > 0 ? available_width : 500.0f),
        .y = fminf(text_height, available_height > 0 ? available_height : 100.0f)
    };
}

static void ui_label_destroy_impl(Widget* widget) {
    if (!widget) return;

    UILabel* label = (UILabel*)widget;

    if (label->text) {
        free(label->text);
        label->text = NULL;
    }

    // Don't call widget_destroy on base - caller should handle
}

/* ============================================================================
 * PUBLIC API - Creation and Destruction
 * ============================================================================ */

UILabel* ui_label_create(const char* name, const char* text) {
    UILabel* label = memory_alloc(sizeof(UILabel));
    if (!label) {
        LOG_ERROR_CAT(LOG_CAT_GENERAL, "Failed to allocate UI label");
        return NULL;
    }

    // Initialize base widget
    Widget* base = widget_create(name ? name : "Label");
    if (!base) {
        free(label);
        return NULL;
    }

    // Copy base widget into label
    memcpy(label, base, sizeof(Widget));

    // Set label-specific defaults
    label->font_size = 12.0f;
    label->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};  // White
    label->alignment = LABEL_ALIGN_LEFT;
    label->v_alignment = LABEL_VALIGN_CENTER;
    label->format = LABEL_FORMAT_PLAIN;
    label->word_wrap = false;
    label->ellipsis = false;
    label->max_lines = 0;
    label->calculated_lines = 1;
    label->shadow_offset_x = 0.0f;
    label->shadow_offset_y = 0.0f;
    label->shadow_color = (Vec4){0.0f, 0.0f, 0.0f, 0.5f};
    label->show_shadow = false;

    // Set text
    if (text) {
        label->text = strdup(text);
        label->text_length = strlen(text);
    } else {
        label->text = strdup("");
        label->text_length = 0;
    }

    // Set virtual functions
    label->base.render = ui_label_render_impl;
    label->base.layout = ui_label_layout_impl;
    label->base.measure = ui_label_measure_impl;
    label->base.handle_event = NULL;  // Labels don't handle events
    label->base.destroy = ui_label_destroy_impl;

    // Label defaults
    label->base.focusable = false;
    label->base.hoverable = false;

    // Default size
    widget_set_size(&label->base, (Vec2){200.0f, 20.0f});

    LOG_INFO_CAT(LOG_CAT_GENERAL, "Created UI label: %s with text '%s'", name ? name : "unnamed", text ? text : "");
    return label;
}

void ui_label_destroy(UILabel* label) {
    if (!label) return;

    if (label->base.destroy) {
        label->base.destroy(&label->base);
    }

    // Destroy all child widgets
    for (uint32_t i = 0; i < label->base.child_count; i++) {
        Widget* child = label->base.children[i];
        if (child && child->destroy) {
            child->destroy(child);
        }
    }

    if (label->base.children) {
        free(label->base.children);
    }

    if (label->base.name) {
        free(label->base.name);
    }

    free(label);
}

/* ============================================================================
 * PUBLIC API - Content Management
 * ============================================================================ */

void ui_label_set_text(UILabel* label, const char* text) {
    if (!label) return;

    if (label->text) {
        free(label->text);
    }

    if (text) {
        label->text = strdup(text);
        label->text_length = strlen(text);
    } else {
        label->text = strdup("");
        label->text_length = 0;
    }

    ui_label_recalculate_lines(label);
    widget_invalidate_redraw(&label->base);
    widget_invalidate_layout(&label->base);
}

const char* ui_label_get_text(const UILabel* label) {
    if (!label || !label->text) return "";
    return label->text;
}

void ui_label_set_text_format(UILabel* label, const char* fmt, ...) {
    if (!label || !fmt) return;

    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    ui_label_set_text(label, buffer);
}

/* ============================================================================
 * PUBLIC API - Text Properties
 * ============================================================================ */

void ui_label_set_font_size(UILabel* label, float font_size) {
    if (!label) return;
    if (label->font_size != font_size) {
        label->font_size = font_size;
        ui_label_recalculate_lines(label);
        widget_invalidate_redraw(&label->base);
        widget_invalidate_layout(&label->base);
    }
}

void ui_label_set_text_color(UILabel* label, Vec4 color) {
    if (!label) return;
    label->text_color = color;
    widget_invalidate_redraw(&label->base);
}

void ui_label_set_alignment(UILabel* label, LabelAlignment alignment) {
    if (!label) return;
    if (label->alignment != alignment) {
        label->alignment = alignment;
        widget_invalidate_redraw(&label->base);
    }
}

void ui_label_set_vertical_alignment(UILabel* label, LabelVerticalAlignment v_alignment) {
    if (!label) return;
    if (label->v_alignment != v_alignment) {
        label->v_alignment = v_alignment;
        widget_invalidate_redraw(&label->base);
    }
}

void ui_label_set_format(UILabel* label, LabelTextFormat format) {
    if (!label) return;
    if (label->format != format) {
        label->format = format;
        widget_invalidate_redraw(&label->base);
    }
}

/* ============================================================================
 * PUBLIC API - Layout Properties
 * ============================================================================ */

void ui_label_set_word_wrap(UILabel* label, bool wrap) {
    if (!label) return;
    if (label->word_wrap != wrap) {
        label->word_wrap = wrap;
        ui_label_recalculate_lines(label);
        widget_invalidate_redraw(&label->base);
        widget_invalidate_layout(&label->base);
    }
}

void ui_label_set_ellipsis(UILabel* label, bool ellipsis) {
    if (!label) return;
    if (label->ellipsis != ellipsis) {
        label->ellipsis = ellipsis;
        widget_invalidate_redraw(&label->base);
    }
}

void ui_label_set_max_lines(UILabel* label, uint32_t max_lines) {
    if (!label) return;
    if (label->max_lines != max_lines) {
        label->max_lines = max_lines;
        ui_label_recalculate_lines(label);
        widget_invalidate_redraw(&label->base);
        widget_invalidate_layout(&label->base);
    }
}

uint32_t ui_label_get_line_count(const UILabel* label) {
    if (!label) return 0;
    return label->calculated_lines;
}

/* ============================================================================
 * PUBLIC API - Visual Effects
 * ============================================================================ */

void ui_label_set_shadow(UILabel* label, float offset_x, float offset_y, Vec4 color, bool show_shadow) {
    if (!label) return;
    label->shadow_offset_x = offset_x;
    label->shadow_offset_y = offset_y;
    label->shadow_color = color;
    label->show_shadow = show_shadow;
    widget_invalidate_redraw(&label->base);
}

void ui_label_disable_shadow(UILabel* label) {
    if (!label) return;
    label->show_shadow = false;
    widget_invalidate_redraw(&label->base);
}

/* ============================================================================
 * PUBLIC API - Utility
 * ============================================================================ */

Widget* ui_label_get_widget(UILabel* label) {
    if (!label) return NULL;
    return &label->base;
}

Vec2 ui_label_get_content_size(UILabel* label, float max_width) {
    if (!label) return (Vec2){0, 0};

    Size measured = ui_label_measure_impl(&label->base, max_width, 0);
    return (Vec2){measured.x, measured.y};
}

/* ============================================================================
 * PUBLIC API - Text Rendering Backend
 * ============================================================================ */

void ui_label_set_text_backend(UILabelDrawTextFn draw, void* context) {
    g_label_draw_text = draw;
    g_label_draw_context = context;
}
