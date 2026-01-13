/*
 * radio_button.h
 * RadioButton Widget Implementation
 * Single selection from a group of options
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RADIO_BUTTON_H
#define UI_RADIO_BUTTON_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIRadioButton UIRadioButton;

// Callback for selection events
typedef void (*RadioButtonSelectedCallback)(UIRadioButton* radio, bool selected, void* user_data);

// Radio button structure
struct UIRadioButton {
    Widget base;                    // Inherit from Widget

    // State
    bool selected;
    int group_id;

    // Content
    char* label;

    // Visual properties
    float circle_size;              // Diameter
    float text_padding;

    Vec4 circle_color;
    Vec4 selected_color;            // Color of the inner dot
    Vec4 text_color;
    Vec4 hover_color;

    // Callbacks
    RadioButtonSelectedCallback on_selected;
    void* user_data;
};

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new radio button widget
 *
 * @param name     Name identifier
 * @param label    Label text (optional)
 * @param group_id ID of the group this button belongs to
 * @return         Pointer to created radio button, or NULL on failure
 */
UIRadioButton* ui_radio_button_create(const char* name, const char* label, int group_id);

/**
 * Destroy a radio button widget
 *
 * @param radio    Radio button to destroy
 */
void ui_radio_button_destroy(UIRadioButton* radio);

/* ============================================================================
 * API - State Management
 * ============================================================================ */

/**
 * Set selected state
 * Note: Setting this to true will deselect other radio buttons in the same group
 * (siblings sharing the same parent and group_id)
 *
 * @param radio    Target radio button
 * @param selected New state
 */
void ui_radio_button_set_selected(UIRadioButton* radio, bool selected);

/**
 * Get selected state
 *
 * @param radio    Target radio button
 * @return         Current state
 */
bool ui_radio_button_is_selected(const UIRadioButton* radio);

/**
 * Set group ID
 *
 * @param radio    Target radio button
 * @param group_id New group ID
 */
void ui_radio_button_set_group(UIRadioButton* radio, int group_id);

/* ============================================================================
 * API - Content
 * ============================================================================ */

/**
 * Set label text
 *
 * @param radio    Target radio button
 * @param label    New label text
 */
void ui_radio_button_set_label(UIRadioButton* radio, const char* label);

/* ============================================================================
 * API - Appearance
 * ============================================================================ */

/**
 * Set colors
 *
 * @param radio    Target radio button
 * @param circle   Outer circle color
 * @param selected Inner dot color
 * @param text     Text color
 */
void ui_radio_button_set_colors(UIRadioButton* radio, Vec4 circle, Vec4 selected, Vec4 text);

/**
 * Set circle size
 *
 * @param radio    Target radio button
 * @param size     Diameter in pixels
 */
void ui_radio_button_set_circle_size(UIRadioButton* radio, float size);

/* ============================================================================
 * API - Callbacks
 * ============================================================================ */

/**
 * Set selection callback
 *
 * @param radio     Target radio button
 * @param callback  Function to call when selection state changes
 * @param user_data User data passed to callback
 */
void ui_radio_button_set_on_selected(UIRadioButton* radio, RadioButtonSelectedCallback callback, void* user_data);

/**
 * Get base widget
 *
 * @param radio    Target radio button
 * @return         Base Widget pointer
 */
Widget* ui_radio_button_get_widget(UIRadioButton* radio);

#ifdef __cplusplus
}
#endif

#endif // UI_RADIO_BUTTON_H
