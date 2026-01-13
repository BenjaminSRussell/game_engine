/*
 * slider.h
 * Slider Widget Implementation
 * Input widget for selecting a value from a continuous or discrete range
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum UIOrientation {
    UI_ORIENTATION_HORIZONTAL,
    UI_ORIENTATION_VERTICAL
} UIOrientation;

typedef struct UISlider UISlider;

// Callback for value changes
typedef void (*SliderValueChangedCallback)(UISlider* slider, float value, void* user_data);

// Slider widget structure
struct UISlider {
    Widget base;                    // Inherit from Widget

    // Value properties
    float value;
    float min_value;
    float max_value;
    float step;                     // 0 for continuous
    UIOrientation orientation;

    // Visual properties
    Vec4 track_color;
    Vec4 fill_color;
    Vec4 handle_color;
    Vec4 handle_hover_color;
    Vec4 handle_pressed_color;

    float track_thickness;
    float handle_size;              // Diameter or side length
    float handle_radius;            // For rounded handle

    // Animation
    float hover_progress;

    // Interaction state
    bool is_dragging;

    // Callbacks
    SliderValueChangedCallback on_value_changed;
    void* user_data;
};

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new slider widget
 *
 * @param name         Name identifier for the slider
 * @param value        Initial value
 * @param min          Minimum value
 * @param max          Maximum value
 * @param orientation  Slider orientation (horizontal/vertical)
 * @return             Pointer to created slider, or NULL on failure
 */
UISlider* ui_slider_create(const char* name, float value, float min, float max, UIOrientation orientation);

/**
 * Destroy a slider widget
 *
 * @param slider   Slider to destroy
 */
void ui_slider_destroy(UISlider* slider);

/* ============================================================================
 * API - Value Management
 * ============================================================================ */

/**
 * Set the slider value (clamped to range and snapped to step)
 *
 * @param slider   Target slider
 * @param value    New value
 */
void ui_slider_set_value(UISlider* slider, float value);

/**
 * Get the current slider value
 *
 * @param slider   Target slider
 * @return         Current value
 */
float ui_slider_get_value(const UISlider* slider);

/**
 * Set the value range
 *
 * @param slider   Target slider
 * @param min      Minimum value
 * @param max      Maximum value
 */
void ui_slider_set_range(UISlider* slider, float min, float max);

/**
 * Set the value step increment
 *
 * @param slider   Target slider
 * @param step     Step size (0.0f for continuous)
 */
void ui_slider_set_step(UISlider* slider, float step);

/* ============================================================================
 * API - Appearance
 * ============================================================================ */

/**
 * Set the slider orientation
 *
 * @param slider       Target slider
 * @param orientation  New orientation
 */
void ui_slider_set_orientation(UISlider* slider, UIOrientation orientation);

/**
 * Set slider colors
 *
 * @param slider   Target slider
 * @param track    Color of the empty track
 * @param fill     Color of the filled portion
 * @param handle   Color of the handle knob
 */
void ui_slider_set_colors(UISlider* slider, Vec4 track, Vec4 fill, Vec4 handle);

/**
 * Set handle size
 *
 * @param slider   Target slider
 * @param size     Diameter/size of the handle
 */
void ui_slider_set_handle_size(UISlider* slider, float size);

/* ============================================================================
 * API - Callbacks
 * ============================================================================ */

/**
 * Set value changed callback
 *
 * @param slider    Target slider
 * @param callback  Function to call when value changes
 * @param user_data User data passed to callback
 */
void ui_slider_set_on_value_changed(UISlider* slider, SliderValueChangedCallback callback, void* user_data);

/**
 * Get the base widget
 *
 * @param slider   Target slider
 * @return         Base Widget pointer
 */
Widget* ui_slider_get_widget(UISlider* slider);

#ifdef __cplusplus
}
#endif

#endif // UI_SLIDER_H
