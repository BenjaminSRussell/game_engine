/*
 * image_widget.h
 * Image Widget Implementation
 * Displays a texture with optional UV mapping and tinting
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_IMAGE_WIDGET_H
#define UI_IMAGE_WIDGET_H

#include "widget.h"
#include "include/math/rect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    TextureID texture_id;
    Rect uv_rect;
    Vec4 tint_color;
    bool keep_aspect_ratio;
} ImageWidgetData;

/**
 * @brief Creates a new image widget
 * @param name Name of the widget
 * @param texture_id ID of the texture to display
 * @return Pointer to the new widget
 */
Widget* image_widget_create(const char* name, TextureID texture_id);

/**
 * @brief Sets the texture of the image widget
 * @param widget Pointer to the widget
 * @param texture_id New texture ID
 */
void image_widget_set_texture(Widget* widget, TextureID texture_id);

/**
 * @brief Sets the UV coordinates of the image widget
 * @param widget Pointer to the widget
 * @param uv_rect UV rectangle (x, y, width, height)
 */
void image_widget_set_uvs(Widget* widget, Rect uv_rect);

/**
 * @brief Sets the tint color of the image widget
 * @param widget Pointer to the widget
 * @param color Tint color
 */
void image_widget_set_tint(Widget* widget, Vec4 color);

/**
 * @brief Sets whether the image should maintain its aspect ratio
 * @param widget Pointer to the widget
 * @param enable True to maintain aspect ratio, false to stretch
 */
void image_widget_set_keep_aspect_ratio(Widget* widget, bool enable);

#ifdef __cplusplus
}
#endif

#endif // UI_IMAGE_WIDGET_H
