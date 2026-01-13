/*
 * label.h
 * Label Widget
 * Display text with configurable properties
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
} TextAlign;

/**
 * @brief Creates a new label widget
 * @param name Widget name for debugging
 * @param text Initial text content
 * @return Pointer to the new widget
 */
Widget* label_create(const char* name, const char* text);

/**
 * @brief Sets the text content of the label
 * @param widget Pointer to the label widget
 * @param text New text content
 */
void label_set_text(Widget* widget, const char* text);

/**
 * @brief Gets the current text content
 * @param widget Pointer to the label widget
 * @return Current text string (read-only)
 */
const char* label_get_text(const Widget* widget);

/**
 * @brief Sets the font size
 * @param widget Pointer to the label widget
 * @param size Font size in pixels
 */
void label_set_font_size(Widget* widget, float size);

/**
 * @brief Sets the text alignment
 * @param widget Pointer to the label widget
 * @param align Text alignment (left, center, right)
 */
void label_set_alignment(Widget* widget, TextAlign align);

#ifdef __cplusplus
}
#endif

#endif /* UI_LABEL_H */
