/*
 * progressbar.h
 * Progress Bar Widget Implementation
 * Visual indicator of operation progress
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_PROGRESSBAR_H
#define UI_PROGRESSBAR_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIProgressBar UIProgressBar;

// Progress bar structure
struct UIProgressBar {
    Widget base;                    // Inherit from Widget

    // Value properties
    float value;
    float min_value;
    float max_value;
    bool indeterminate;             // Indeterminate state (loading animation)

    // Visual properties
    Vec4 background_color;
    Vec4 fill_color;
    float corner_radius;

    // Text overlay
    bool show_text;
    char* format_text;              // e.g., "%.0f%%"
    Vec4 text_color;

    // Animation state for indeterminate mode
    float animation_time;
};

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new progress bar widget
 *
 * @param name     Name identifier
 * @return         Pointer to created progress bar, or NULL on failure
 */
UIProgressBar* ui_progressbar_create(const char* name);

/**
 * Destroy a progress bar widget
 *
 * @param bar      Progress bar to destroy
 */
void ui_progressbar_destroy(UIProgressBar* bar);

/* ============================================================================
 * API - Value Management
 * ============================================================================ */

/**
 * Set current progress value
 *
 * @param bar      Target progress bar
 * @param value    Current value
 */
void ui_progressbar_set_value(UIProgressBar* bar, float value);

/**
 * Get current progress value
 *
 * @param bar      Target progress bar
 * @return         Current value
 */
float ui_progressbar_get_value(const UIProgressBar* bar);

/**
 * Set progress percentage (0.0 to 1.0)
 *
 * @param bar      Target progress bar
 * @param percent  Percentage value
 */
void ui_progressbar_set_fraction(UIProgressBar* bar, float fraction);

/**
 * Set value range
 *
 * @param bar      Target progress bar
 * @param min      Minimum value
 * @param max      Maximum value
 */
void ui_progressbar_set_range(UIProgressBar* bar, float min, float max);

/**
 * Set indeterminate mode (infinite loading)
 *
 * @param bar           Target progress bar
 * @param indeterminate Whether to use indeterminate mode
 */
void ui_progressbar_set_indeterminate(UIProgressBar* bar, bool indeterminate);

/* ============================================================================
 * API - Appearance
 * ============================================================================ */

/**
 * Set colors
 *
 * @param bar      Target progress bar
 * @param bg       Background color
 * @param fill     Fill color
 */
void ui_progressbar_set_colors(UIProgressBar* bar, Vec4 bg, Vec4 fill);

/**
 * Configure text overlay
 *
 * @param bar      Target progress bar
 * @param show     Whether to show text
 * @param format   Printf-style format string (e.g., "%.0f%%")
 */
void ui_progressbar_set_show_text(UIProgressBar* bar, bool show, const char* format);

/**
 * Set text color
 *
 * @param bar      Target progress bar
 * @param color    Text color
 */
void ui_progressbar_set_text_color(UIProgressBar* bar, Vec4 color);

/**
 * Get base widget
 *
 * @param bar      Target progress bar
 * @return         Base Widget pointer
 */
Widget* ui_progressbar_get_widget(UIProgressBar* bar);

#ifdef __cplusplus
}
#endif

#endif // UI_PROGRESSBAR_H
