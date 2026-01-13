/*
 * label.c
 * Label Widget Implementation
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "label.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char* text;
    float font_size;
    TextAlign alignment;
} LabelData;

static void label_destroy(Widget* widget) {
    if (!widget || !widget->user_data) return;

    LabelData* data = (LabelData*)widget->user_data;

    if (data->text) {
        free(data->text); // text is strdup'ed
    }

    memory_free(data);
    widget->user_data = NULL;
}

static void label_render(Widget* widget) {
    if (!widget || !widget->visible || !widget->user_data) return;

    // Stub renderer - actual text rendering to be implemented
    // This would typically issue draw commands to the UI renderer

    // LabelData* data = (LabelData*)widget->user_data;
    // LOG_DEBUG(LOG_CAT_UI, "Rendering label '%s' at (%.1f, %.1f)",
    //           data->text, widget->position.x, widget->position.y);
}

static Size label_measure(Widget* widget, float available_width, float available_height) {
    if (!widget || !widget->user_data) return (Size){0, 0};

    LabelData* data = (LabelData*)widget->user_data;

    // Simple estimation for now since we don't have font metrics
    // Assume monospaced font with aspect ratio 0.6 per character
    size_t len = data->text ? strlen(data->text) : 0;
    float char_width = data->font_size * 0.6f;
    float width = len * char_width;
    float height = data->font_size;

    // Add padding
    width += widget->padding.left + widget->padding.right;
    height += widget->padding.top + widget->padding.bottom;

    return (Size){width, height};
}

static void label_layout(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    // Labels typically don't have children to layout,
    // but if we supported rich text with embedded widgets, we would do it here.

    // For now, just measure and set size if not fixed
    Size measured = label_measure(widget, available_width, available_height);

    if (widget->size.x == 0) widget->size.x = measured.width;
    if (widget->size.y == 0) widget->size.y = measured.height;
}

Widget* label_create(const char* name, const char* text) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    LabelData* data = memory_alloc(sizeof(LabelData));
    if (!data) {
        widget_destroy(widget);
        return NULL;
    }

    data->text = text ? strdup(text) : strdup("");
    data->font_size = 14.0f; // Default font size
    data->alignment = TEXT_ALIGN_LEFT;

    widget->user_data = data;

    // Override virtual functions
    widget->destroy = label_destroy;
    widget->render = label_render;
    widget->measure = label_measure;
    widget->layout = label_layout;

    return widget;
}

void label_set_text(Widget* widget, const char* text) {
    if (!widget || !widget->user_data) return;

    LabelData* data = (LabelData*)widget->user_data;

    if (data->text) {
        free(data->text);
    }

    data->text = text ? strdup(text) : strdup("");

    widget_invalidate_layout(widget);
    widget_invalidate_redraw(widget);
}

const char* label_get_text(const Widget* widget) {
    if (!widget || !widget->user_data) return NULL;

    LabelData* data = (LabelData*)widget->user_data;
    return data->text;
}

void label_set_font_size(Widget* widget, float size) {
    if (!widget || !widget->user_data) return;

    LabelData* data = (LabelData*)widget->user_data;
    if (data->font_size != size) {
        data->font_size = size;
        widget_invalidate_layout(widget);
        widget_invalidate_redraw(widget);
    }
}

void label_set_alignment(Widget* widget, TextAlign align) {
    if (!widget || !widget->user_data) return;

    LabelData* data = (LabelData*)widget->user_data;
    if (data->alignment != align) {
        data->alignment = align;
        widget_invalidate_redraw(widget);
    }
}
