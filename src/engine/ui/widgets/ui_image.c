#include "ui_image.h"
#include "core/memory.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t texture_id;
    Rect uv_rect;
    Vec4 color;
} UIImageData;

static void ui_image_render(Widget* widget);
static void ui_image_destroy(Widget* widget);

Widget* ui_image_create(const char* name, uint32_t texture_id) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    UIImageData* data = memory_alloc(sizeof(UIImageData));
    if (!data) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI image data");
        widget_destroy(widget);
        return NULL;
    }

    data->texture_id = texture_id;
    data->uv_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f};
    data->color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};

    widget->user_data = data;
    widget->render = ui_image_render;
    widget->destroy = ui_image_destroy;

    // Default size
    widget_set_size(widget, (Vec2){100.0f, 100.0f});

    // Transparent background by default for images, usually we just want the texture
    widget_set_background_color(widget, (Vec4){0.0f, 0.0f, 0.0f, 0.0f});
    widget_set_border_width(widget, 0.0f);

    return widget;
}

static void ui_image_destroy(Widget* widget) {
    if (widget && widget->user_data) {
        memory_free(widget->user_data);
        widget->user_data = NULL;
    }
}

static void ui_image_render(Widget* widget) {
    UIImageData* data = (UIImageData*)widget->user_data;
    if (!data) return;

    if (!widget_is_visible(widget)) return;

    // Stub implementation - actual rendering would happen in UI renderer using this data
    // The renderer would look at widget type or user_data and draw the texture
}

void ui_image_set_texture_id(Widget* widget, uint32_t texture_id) {
    UIImageData* data = (UIImageData*)widget->user_data;
    if (data && data->texture_id != texture_id) {
        data->texture_id = texture_id;
        widget_invalidate_redraw(widget);
    }
}

void ui_image_set_uv_rect(Widget* widget, Rect uv_rect) {
    UIImageData* data = (UIImageData*)widget->user_data;
    if (data) {
        data->uv_rect = uv_rect;
        widget_invalidate_redraw(widget);
    }
}

void ui_image_set_color(Widget* widget, Vec4 color) {
    UIImageData* data = (UIImageData*)widget->user_data;
    if (data) {
        data->color = color;
        widget_invalidate_redraw(widget);
    }
}

uint32_t ui_image_get_texture_id(const Widget* widget) {
    UIImageData* data = (UIImageData*)widget->user_data;
    return data ? data->texture_id : 0;
}

Rect ui_image_get_uv_rect(const Widget* widget) {
    UIImageData* data = (UIImageData*)widget->user_data;
    return data ? data->uv_rect : (Rect){0.0f, 0.0f, 1.0f, 1.0f};
}

Vec4 ui_image_get_color(const Widget* widget) {
    UIImageData* data = (UIImageData*)widget->user_data;
    return data ? data->color : (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
}
