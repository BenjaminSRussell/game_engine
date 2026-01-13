/*
 * ui_label.c
 * Label Widget Implementation
 * Text display widget for UI labels and messages
 */

#include "ui_label.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static void ui_label_recalculate_lines(UILabel* label) {
    if (!label || !label->text) {
        label->calculated_lines = 0;
        return;
    }

    if (!label->word_wrap) {
        label->calculated_lines = 1;
        return;
    }

    // Rough calculation - would need actual font metrics for accuracy
    Vec2 size = widget_get_size(&label->base);
    uint32_t chars_per_line = (uint32_t)(size.x / (label->font_size * 0.6f));
    if (chars_per_line <= 0) chars_per_line = 1;

    uint32_t lines = 1;
    uint32_t current_line_chars = 0;
    for (uint32_t i = 0; i < label->text_length; i++) {
        if (label->text[i] == '\n') {
            lines++;
            current_line_chars = 0;
        } else {
            current_line_chars++;
            if (current_line_chars >= chars_per_line) {
                lines++;
                current_line_chars = 0;
            }
        }
    }

    if (label->max_lines > 0 && lines > label->max_lines) {
        lines = label->max_lines;
    }

    label->calculated_lines = lines;
}

static void ui_label_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UILabel* label = (UILabel*)widget;
    Vec2 pos = widget_get_position(widget);
    Vec2 size = widget_get_size(widget);

    // TODO: Implement actual text rendering with graphics backend
    // For now, this is a placeholder
    // In a real implementation, this would:
    // 1. Render shadow if enabled
    // 2. Render text with proper alignment
    // 3. Handle word wrapping and ellipsis
    // 4. Apply text color and format

    LOG_DEBUG("Label render: pos=(%.1f, %.1f) size=(%.1f, %.1f) text=%s",
             pos.x, pos.y, size.x, size.y, label->text ? label->text : "");
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
        .width = fminf(text_width, available_width > 0 ? available_width : 500.0f),
        .height = fminf(text_height, available_height > 0 ? available_height : 100.0f)
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
        LOG_ERROR("Failed to allocate UI label");
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

    LOG_INFO("Created UI label: %s with text '%s'", name ? name : "unnamed", text ? text : "");
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
    return (Vec2){measured.width, measured.height};
}
