/*
 * button.c
 * Button Widget Implementation
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "button.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static void button_destroy_impl(Widget* widget) {
    Button* button = (Button*)widget;

    if (button->label) {
        free(button->label);
        button->label = NULL;
    }
}

static void button_render_impl(Widget* widget) {
    Button* button = (Button*)widget;

    // Placeholder rendering logic
    // In a real implementation, this would issue draw commands to the renderer

    Vec4 current_color = button->normal_color;
    Vec4 current_text_color = button->normal_text_color;

    if (!widget->enabled) {
        current_color = button->disabled_color;
        current_text_color = button->disabled_text_color;
    } else if (widget->state == WIDGET_STATE_PRESSED) {
        current_color = button->pressed_color;
        current_text_color = button->pressed_text_color;
    } else if (widget->state == WIDGET_STATE_HOVER) {
        current_color = button->hover_color;
        current_text_color = button->hover_text_color;
    }

    // We log at verbose/debug level to avoid spam, but useful for verification
    // LOG_DEBUG("Render Button '%s' [State: %d] [Color: %.1f,%.1f,%.1f,%.1f]",
    //           widget->name, widget->state,
    //           current_color.x, current_color.y, current_color.z, current_color.w);
}

static bool button_handle_event_impl(Widget* widget, UIEvent* event) {
    Button* button = (Button*)widget;
    bool handled = false;

    if (!widget->enabled || !widget->visible) return false;

    switch (event->type) {
        case UI_EVENT_MOUSE_ENTER:
            if (widget->state != WIDGET_STATE_PRESSED && widget->state != WIDGET_STATE_DISABLED) {
                widget_set_state(widget, WIDGET_STATE_HOVER);
                widget_invalidate_redraw(widget);
                handled = true;
            }
            break;

        case UI_EVENT_MOUSE_LEAVE:
            if (widget->state != WIDGET_STATE_DISABLED) {
                // If we were pressed, we might want to stay pressed if drag-out behavior is supported
                // But for simple buttons, leaving usually resets state
                widget_set_state(widget, WIDGET_STATE_NORMAL);
                widget_invalidate_redraw(widget);
                handled = true;
            }
            break;

        case UI_EVENT_MOUSE_DOWN:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                widget_request_focus(widget);
                widget_set_state(widget, WIDGET_STATE_PRESSED);
                widget_invalidate_redraw(widget);
                handled = true;
            }
            break;

        case UI_EVENT_MOUSE_UP:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                if (widget->state == WIDGET_STATE_PRESSED) {
                    // Check if mouse is still inside
                    if (widget_contains_point(widget, event->mouse.position)) {
                        // Click!
                        if (button->on_click) {
                            button->on_click(widget, event, button->on_click_user_data);
                        }

                        // Emit CLICK event
                        UIEvent* click_event = ui_event_create_mouse(UI_EVENT_MOUSE_CLICK, event->mouse.position, UI_MOUSE_BUTTON_LEFT);
                        if (click_event) {
                            widget_emit_event(widget, click_event);
                            ui_event_destroy(click_event);
                        }

                        widget_set_state(widget, WIDGET_STATE_HOVER);
                    } else {
                        widget_set_state(widget, WIDGET_STATE_NORMAL);
                    }
                    widget_invalidate_redraw(widget);
                    handled = true;
                }
            }
            break;

        default:
            break;
    }

    return handled;
}

/* ============================================================================
 * API
 * ============================================================================ */

Button* button_create(const char* name, const char* label) {
    Button* button = memory_alloc(sizeof(Button));
    if (!button) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate button");
        return NULL;
    }

    if (!widget_init(&button->widget, name)) {
        memory_free(button);
        return NULL;
    }

    // Set virtual overrides
    button->widget.destroy = button_destroy_impl;
    button->widget.render = button_render_impl;
    button->widget.handle_event = button_handle_event_impl;

    // Initialize button properties
    button->label = label ? strdup(label) : NULL;
    button->on_click = NULL;
    button->on_click_user_data = NULL;

    // Set default colors
    button->normal_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    button->hover_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
    button->pressed_color = (Vec4){0.1f, 0.1f, 0.1f, 1.0f};
    button->disabled_color = (Vec4){0.1f, 0.1f, 0.1f, 0.5f};

    button->normal_text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    button->hover_text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    button->pressed_text_color = (Vec4){0.8f, 0.8f, 0.8f, 1.0f};
    button->disabled_text_color = (Vec4){0.5f, 0.5f, 0.5f, 0.5f};

    // Set default widget properties for a button
    button->widget.background_color = button->normal_color;
    button->widget.focusable = true;

    return button;
}

void button_set_label(Button* button, const char* label) {
    if (!button) return;

    if (button->label) {
        free(button->label);
    }

    button->label = label ? strdup(label) : NULL;
    button->widget.dirty = true;
    widget_invalidate_redraw(&button->widget);
}

void button_set_on_click(Button* button, UIEventCallback callback, void* user_data) {
    if (!button) return;

    button->on_click = callback;
    button->on_click_user_data = user_data;
}

void button_set_colors(Button* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled) {
    if (!button) return;

    button->normal_color = normal;
    button->hover_color = hover;
    button->pressed_color = pressed;
    button->disabled_color = disabled;

    // Update current color
    if (button->widget.state == WIDGET_STATE_NORMAL) {
        button->widget.background_color = normal;
    } // Other states will be updated on next event or render

    widget_invalidate_redraw(&button->widget);
}

void button_set_text_colors(Button* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled) {
    if (!button) return;

    button->normal_text_color = normal;
    button->hover_text_color = hover;
    button->pressed_text_color = pressed;
    button->disabled_text_color = disabled;

    widget_invalidate_redraw(&button->widget);
}
