/*
 * image_widget.c
 * Image Widget Implementation
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "image_widget.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static void image_widget_destroy(Widget* widget) {
    if (!widget || !widget->user_data) return;

    // Free the image data
    memory_free(widget->user_data);
    widget->user_data = NULL;
}

static void image_widget_render(Widget* widget) {
    if (!widget || !widget->visible || !widget->user_data) return;

    ImageWidgetData* data = (ImageWidgetData*)widget->user_data;
    Rect bounds = widget_get_bounds(widget);

    // Placeholder for actual rendering call
    LOG_DEBUG(LOG_CAT_GENERAL, "Rendering ImageWidget '%s': TextureID=%u, Rect=(%.1f, %.1f, %.1f, %.1f), UV=(%.2f, %.2f, %.2f, %.2f)",
              widget->name,
              data->texture_id,
              bounds.x, bounds.y, bounds.width, bounds.height,
              data->uv_rect.x, data->uv_rect.y, data->uv_rect.width, data->uv_rect.height);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

Widget* image_widget_create(const char* name, TextureID texture_id) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    ImageWidgetData* data = memory_alloc(sizeof(ImageWidgetData));
    if (!data) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate image widget data");
        widget_destroy(widget);
        return NULL;
    }

    data->texture_id = texture_id;
    data->uv_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f};
    data->tint_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    data->keep_aspect_ratio = false;

    widget->user_data = data;
    // Set virtual function overrides
    // Note: widget->destroy is called by widget_destroy() for custom cleanup
    widget->destroy = image_widget_destroy;
    widget->render = image_widget_render;

    return widget;
}

void image_widget_set_texture(Widget* widget, TextureID texture_id) {
    if (!widget || !widget->user_data) return;
    ImageWidgetData* data = (ImageWidgetData*)widget->user_data;

    if (data->texture_id != texture_id) {
        data->texture_id = texture_id;
        widget_invalidate_redraw(widget);
    }
}

void image_widget_set_uvs(Widget* widget, Rect uv_rect) {
    if (!widget || !widget->user_data) return;
    ImageWidgetData* data = (ImageWidgetData*)widget->user_data;

    data->uv_rect = uv_rect;
    widget_invalidate_redraw(widget);
}

void image_widget_set_tint(Widget* widget, Vec4 color) {
    if (!widget || !widget->user_data) return;
    ImageWidgetData* data = (ImageWidgetData*)widget->user_data;

    data->tint_color = color;
    widget_invalidate_redraw(widget);
}

void image_widget_set_keep_aspect_ratio(Widget* widget, bool enable) {
    if (!widget || !widget->user_data) return;
    ImageWidgetData* data = (ImageWidgetData*)widget->user_data;

    if (data->keep_aspect_ratio != enable) {
        data->keep_aspect_ratio = enable;
        widget_invalidate_layout(widget);
        widget_invalidate_redraw(widget);
    }
}
