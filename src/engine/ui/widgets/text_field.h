#ifndef UI_TEXT_FIELD_H
#define UI_TEXT_FIELD_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TextField Widget
 *
 * A widget that allows the user to view and edit a single line of text.
 */

/**
 * Creates a new TextField widget.
 *
 * @param name The name of the widget.
 * @param initial_text The initial text content (can be NULL).
 * @return A pointer to the created widget.
 */
Widget* text_field_create(const char* name, const char* initial_text);

/**
 * Sets the text content of the TextField.
 *
 * @param widget The TextField widget.
 * @param text The new text content.
 */
void text_field_set_text(Widget* widget, const char* text);

/**
 * Gets the current text content of the TextField.
 *
 * @param widget The TextField widget.
 * @return The current text content.
 */
const char* text_field_get_text(const Widget* widget);

/**
 * Sets the placeholder text displayed when the TextField is empty.
 *
 * @param widget The TextField widget.
 * @param placeholder The placeholder text.
 */
void text_field_set_placeholder(Widget* widget, const char* placeholder);

/**
 * Sets the cursor position in the TextField.
 *
 * @param widget The TextField widget.
 * @param position The new cursor position (index).
 */
void text_field_set_cursor_position(Widget* widget, uint32_t position);

/**
 * Gets the current cursor position.
 *
 * @param widget The TextField widget.
 * @return The current cursor position.
 */
uint32_t text_field_get_cursor_position(const Widget* widget);

/**
 * Selects a range of text.
 *
 * @param widget The TextField widget.
 * @param start The start index of the selection.
 * @param end The end index of the selection.
 */
void text_field_select_range(Widget* widget, uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif // UI_TEXT_FIELD_H
