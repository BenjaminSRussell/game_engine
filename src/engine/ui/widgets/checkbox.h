/*
 * checkbox.h
 * CheckBox Widget Implementation
 * Boolean toggle with label
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_CHECKBOX_H
#define UI_CHECKBOX_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UICheckBox UICheckBox;

// Callback for toggle events
typedef void (*CheckBoxToggledCallback)(UICheckBox* checkbox, bool checked, void* user_data);

// CheckBox widget structure
struct UICheckBox {
    Widget base;                    // Inherit from Widget

    // State
    bool checked;

    // Content
    char* label;

    // Visual properties
    float box_size;
    float corner_radius;
    float text_padding;

    Vec4 box_color;
    Vec4 check_color;
    Vec4 text_color;
    Vec4 hover_color;

    // Callbacks
    CheckBoxToggledCallback on_toggled;
    void* user_data;
};

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new checkbox widget
 *
 * @param name     Name identifier
 * @param label    Label text (optional)
 * @param checked  Initial checked state
 * @return         Pointer to created checkbox, or NULL on failure
 */
UICheckBox* ui_checkbox_create(const char* name, const char* label, bool checked);

/**
 * Destroy a checkbox widget
 *
 * @param checkbox Checkbox to destroy
 */
void ui_checkbox_destroy(UICheckBox* checkbox);

/* ============================================================================
 * API - State Management
 * ============================================================================ */

/**
 * Set checked state
 *
 * @param checkbox Target checkbox
 * @param checked  New state
 */
void ui_checkbox_set_checked(UICheckBox* checkbox, bool checked);

/**
 * Get checked state
 *
 * @param checkbox Target checkbox
 * @return         Current state
 */
bool ui_checkbox_is_checked(const UICheckBox* checkbox);

/**
 * Toggle checked state
 *
 * @param checkbox Target checkbox
 */
void ui_checkbox_toggle(UICheckBox* checkbox);

/* ============================================================================
 * API - Content
 * ============================================================================ */

/**
 * Set label text
 *
 * @param checkbox Target checkbox
 * @param label    New label text
 */
void ui_checkbox_set_label(UICheckBox* checkbox, const char* label);

/* ============================================================================
 * API - Appearance
 * ============================================================================ */

/**
 * Set colors
 *
 * @param checkbox Target checkbox
 * @param box      Box background color
 * @param check    Checkmark color
 * @param text     Text color
 */
void ui_checkbox_set_colors(UICheckBox* checkbox, Vec4 box, Vec4 check, Vec4 text);

/**
 * Set box size
 *
 * @param checkbox Target checkbox
 * @param size     Size in pixels
 */
void ui_checkbox_set_box_size(UICheckBox* checkbox, float size);

/* ============================================================================
 * API - Callbacks
 * ============================================================================ */

/**
 * Set toggle callback
 *
 * @param checkbox  Target checkbox
 * @param callback  Function to call when toggled
 * @param user_data User data passed to callback
 */
void ui_checkbox_set_on_toggled(UICheckBox* checkbox, CheckBoxToggledCallback callback, void* user_data);

/**
 * Get base widget
 *
 * @param checkbox Target checkbox
 * @return         Base Widget pointer
 */
Widget* ui_checkbox_get_widget(UICheckBox* checkbox);

#ifdef __cplusplus
}
#endif

#endif // UI_CHECKBOX_H
