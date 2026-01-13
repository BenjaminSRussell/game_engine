/*
 * tooltip.h
 * Tooltip System
 * Manages tooltip text and display logic
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_TOOLTIP_H
#define UI_TOOLTIP_H

#include "widget.h"
#include "include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API - Widget Integration
 * ============================================================================ */

/**
 * Set tooltip text for a widget
 *
 * @param widget   Target widget
 * @param text     Tooltip text
 */
void ui_tooltip_set(Widget* widget, const char* text);

/**
 * Get tooltip text from a widget
 *
 * @param widget   Target widget
 * @return         Tooltip text, or NULL
 */
const char* ui_tooltip_get(const Widget* widget);

/* ============================================================================
 * API - System (Singleton/Global)
 * ============================================================================ */

/**
 * Initialize tooltip system
 */
void ui_tooltip_system_init(void);

/**
 * Update tooltip system (call per frame)
 * Handles hover timers and showing/hiding
 *
 * @param delta_time      Time elapsed since last frame
 * @param hovered_widget  Currently hovered widget (can be NULL)
 */
void ui_tooltip_system_update(float delta_time, Widget* hovered_widget);

/**
 * Configure tooltip appearance
 *
 * @param bg_color    Background color
 * @param text_color  Text color
 * @param delay       Delay in seconds before showing
 */
void ui_tooltip_configure(Vec4 bg_color, Vec4 text_color, float delay);

#ifdef __cplusplus
}
#endif

#endif // UI_TOOLTIP_H
