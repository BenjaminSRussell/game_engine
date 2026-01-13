/*
 * ui_button.h
 * Button Widget Implementation
 * Clickable button with text and visual state feedback
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum ButtonStyle {
    BUTTON_STYLE_DEFAULT,       // Standard button
    BUTTON_STYLE_PRIMARY,       // Highlighted/primary action
    BUTTON_STYLE_DANGER,        // Warning/destructive action
    BUTTON_STYLE_OUTLINE,       // Outlined button
    BUTTON_STYLE_FLAT,          // No background
} ButtonStyle;

typedef enum ButtonSize {
    BUTTON_SIZE_SMALL,
    BUTTON_SIZE_MEDIUM,
    BUTTON_SIZE_LARGE,
} ButtonSize;

// Button state structure
typedef struct UIButton {
    Widget base;                // Inherit from Widget

    // Content
    char* text;
    uint32_t text_length;

    // Visual properties
    ButtonStyle style;
    ButtonSize size;
    float corner_radius;
    bool show_icon;
    uint32_t icon_id;

    // Color overrides
    Vec4 normal_color;
    Vec4 hover_color;
    Vec4 pressed_color;
    Vec4 disabled_color;
    Vec4 text_color;

    // Animation
    float hover_progress;       // 0.0 to 1.0 for smooth hover animation

    // Callbacks
    void (*on_click)(struct UIButton* button, void* user_data);
    void (*on_press)(struct UIButton* button, void* user_data);
    void (*on_release)(struct UIButton* button, void* user_data);
    void* user_data;
} UIButton;

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new button widget
 *
 * @param name     Name identifier for the button
 * @param text     Button label text
 * @return         Pointer to created button, or NULL on failure
 */
UIButton* ui_button_create(const char* name, const char* text);

/**
 * Destroy a button widget and free all resources
 *
 * @param button   Button to destroy
 */
void ui_button_destroy(UIButton* button);

/* ============================================================================
 * API - Content Management
 * ============================================================================ */

/**
 * Set the button's text label
 *
 * @param button   Target button
 * @param text     New text to display
 */
void ui_button_set_text(UIButton* button, const char* text);

/**
 * Get the button's text label
 *
 * @param button   Target button
 * @return         Pointer to button text
 */
const char* ui_button_get_text(const UIButton* button);

/**
 * Set the button's icon
 *
 * @param button   Target button
 * @param icon_id  Texture/icon identifier
 */
void ui_button_set_icon(UIButton* button, uint32_t icon_id);

/**
 * Show or hide the icon
 *
 * @param button   Target button
 * @param show     Whether to show the icon
 */
void ui_button_set_show_icon(UIButton* button, bool show);

/* ============================================================================
 * API - Styling
 * ============================================================================ */

/**
 * Set the button's visual style
 *
 * @param button   Target button
 * @param style    Style to apply
 */
void ui_button_set_style(UIButton* button, ButtonStyle style);

/**
 * Set the button's size preset
 *
 * @param button   Target button
 * @param size     Size preset to apply
 */
void ui_button_set_size(UIButton* button, ButtonSize size);

/**
 * Set corner radius for button background
 *
 * @param button   Target button
 * @param radius   Radius in pixels
 */
void ui_button_set_corner_radius(UIButton* button, float radius);

/**
 * Set custom colors for different button states
 *
 * @param button   Target button
 * @param normal   Color for normal state
 * @param hover    Color for hover state
 * @param pressed  Color for pressed state
 * @param disabled Color for disabled state
 */
void ui_button_set_colors(UIButton* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled);

/**
 * Set text color
 *
 * @param button   Target button
 * @param color    Text color (RGBA)
 */
void ui_button_set_text_color(UIButton* button, Vec4 color);

/* ============================================================================
 * API - State Management
 * ============================================================================ */

/**
 * Check if button is currently pressed
 *
 * @param button   Target button
 * @return         True if pressed
 */
bool ui_button_is_pressed(const UIButton* button);

/**
 * Check if button is currently hovered
 *
 * @param button   Target button
 * @return         True if hovered
 */
bool ui_button_is_hovered(const UIButton* button);

/* ============================================================================
 * API - Callbacks
 * ============================================================================ */

/**
 * Set click callback - called when button is clicked
 *
 * @param button      Target button
 * @param callback    Callback function, or NULL to unset
 * @param user_data   Data to pass to callback
 */
void ui_button_set_on_click(UIButton* button,
                            void (*callback)(UIButton*, void*),
                            void* user_data);

/**
 * Set press callback - called when button is pressed down
 *
 * @param button      Target button
 * @param callback    Callback function, or NULL to unset
 * @param user_data   Data to pass to callback
 */
void ui_button_set_on_press(UIButton* button,
                            void (*callback)(UIButton*, void*),
                            void* user_data);

/**
 * Set release callback - called when button is released
 *
 * @param button      Target button
 * @param callback    Callback function, or NULL to unset
 * @param user_data   Data to pass to callback
 */
void ui_button_set_on_release(UIButton* button,
                              void (*callback)(UIButton*, void*),
                              void* user_data);

/* ============================================================================
 * API - Utility
 * ============================================================================ */

/**
 * Simulate a button click programmatically
 *
 * @param button   Target button
 */
void ui_button_click(UIButton* button);

/**
 * Get the base widget from a button (for hierarchy operations)
 *
 * @param button   Target button
 * @return         Pointer to base Widget structure
 */
Widget* ui_button_get_widget(UIButton* button);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUTTON_H */
