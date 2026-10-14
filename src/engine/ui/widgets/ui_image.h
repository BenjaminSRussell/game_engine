#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Image Widget
 * Displays a texture with optional UV mapping and color tinting.
 */

/**
 * Creates a new Image widget.
 *
 * @param name The name of the widget.
 * @param texture_id The ID of the texture to display (from GPU texture system).
 * @return A pointer to the created widget.
 */
Widget* ui_image_create(const char* name, uint32_t texture_id);

/**
 * Sets the texture ID to display.
 *
 * @param widget The Image widget.
 * @param texture_id The new texture ID.
 */
void ui_image_set_texture_id(Widget* widget, uint32_t texture_id);

/**
 * Sets the UV rectangle for texture mapping.
 * Default is {0, 0, 1, 1} (full texture).
 *
 * @param widget The Image widget.
 * @param uv_rect The UV coordinates (x, y, width, height).
 */
void ui_image_set_uv_rect(Widget* widget, Rect uv_rect);

/**
 * Sets the tint color for the image.
 * Default is white (no tint).
 *
 * @param widget The Image widget.
 * @param color The RGBA color to multiply with the texture.
 */
void ui_image_set_color(Widget* widget, Vec4 color);

/**
 * Gets the current texture ID.
 *
 * @param widget The Image widget.
 * @return The texture ID.
 */
uint32_t ui_image_get_texture_id(const Widget* widget);

/**
 * Gets the current UV rectangle.
 *
 * @param widget The Image widget.
 * @return The UV rectangle.
 */
Rect ui_image_get_uv_rect(const Widget* widget);

/**
 * Gets the current tint color.
 *
 * @param widget The Image widget.
 * @return The tint color.
 */
Vec4 ui_image_get_color(const Widget* widget);

#ifdef __cplusplus
}
#endif

#endif // UI_IMAGE_H
