/*
 * button.h
 * Button Widget Implementation
 * Standard button with states and click handling
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Button Button;

/* ============================================================================
 * TYPES
 * ============================================================================ */

struct Button {
    Widget widget;

    char* label;

    // Callbacks
    UIEventCallback on_click;
    void* on_click_user_data;

    // Visual properties specific to button
    Vec4 normal_color;
    Vec4 hover_color;
    Vec4 pressed_color;
    Vec4 disabled_color;

    Vec4 normal_text_color;
    Vec4 hover_text_color;
    Vec4 pressed_text_color;
    Vec4 disabled_text_color;
};

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * @brief Create a new button widget
 * @param name Unique name for the widget
 * @param label Text to display on the button
 * @return Pointer to the new button, or NULL on failure
 */
Button* button_create(const char* name, const char* label);

/**
 * @brief Set the button label text
 * @param button Pointer to the button
 * @param label New label text
 */
void button_set_label(Button* button, const char* label);

/**
 * @brief Set the click callback
 * @param button Pointer to the button
 * @param callback Function to call when clicked
 * @param user_data User data to pass to the callback
 */
void button_set_on_click(Button* button, UIEventCallback callback, void* user_data);

/**
 * @brief Set button colors for different states
 * @param button Pointer to the button
 * @param normal Normal background color
 * @param hover Hover background color
 * @param pressed Pressed background color
 * @param disabled Disabled background color
 */
void button_set_colors(Button* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled);

/**
 * @brief Set button text colors for different states
 * @param button Pointer to the button
 * @param normal Normal text color
 * @param hover Hover text color
 * @param pressed Pressed text color
 * @param disabled Disabled text color
 */
void button_set_text_colors(Button* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUTTON_H */
