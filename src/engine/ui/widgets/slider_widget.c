/*
 * slider_widget.c
 * Slider Widget Implementation
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "slider_widget.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static void slider_render(Widget* widget) {
    // Stub render function
    // In a real implementation, this would call renderer functions to draw the track and thumb
}

static float calculate_value_from_pos(SliderWidget* slider, Vec2 local_pos) {
    float percent;
    Vec2 size = slider->widget.size;

    // Safety check for zero size
    if (size.x <= 0.001f && size.y <= 0.001f) return slider->min_value;

    if (slider->vertical) {
        if (size.y <= 0.001f) return slider->min_value;
        // Vertical: Bottom (Height) is Min, Top (0) is Max?
        // Or usually GUI coords: 0 is top.
        // Let's assume standard behavior: Up increases value.
        // So Y=Height is Min, Y=0 is Max.
        percent = 1.0f - (local_pos.y / size.y);
    } else {
        if (size.x <= 0.001f) return slider->min_value;
        // Horizontal: Left (0) is Min, Right (Width) is Max.
        percent = local_pos.x / size.x;
    }

    percent = fmaxf(0.0f, fminf(1.0f, percent));
    return slider->min_value + percent * (slider->max_value - slider->min_value);
}

static bool slider_handle_event(Widget* widget, UIEvent* event) {
    SliderWidget* slider = (SliderWidget*)widget;

    switch (event->type) {
        case UI_EVENT_MOUSE_DOWN:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                slider->dragging = true;
                widget_request_focus(widget);
                widget->state = WIDGET_STATE_PRESSED;

                float val = calculate_value_from_pos(slider, event->mouse.position);
                slider_widget_set_value(slider, val);

                return true; // Handled
            }
            break;

        case UI_EVENT_MOUSE_UP:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                if (slider->dragging) {
                    slider->dragging = false;
                    widget->state = widget_contains_point(widget, event->mouse.position) ?
                                   WIDGET_STATE_HOVER : WIDGET_STATE_NORMAL;
                    return true;
                }
            }
            break;

        case UI_EVENT_MOUSE_MOVE:
            if (slider->dragging) {
                float val = calculate_value_from_pos(slider, event->mouse.position);
                slider_widget_set_value(slider, val);
                return true;
            } else {
                 if (widget_contains_point(widget, event->mouse.position)) {
                     if (widget->state != WIDGET_STATE_FOCUSED) {
                         widget->state = WIDGET_STATE_HOVER;
                     }
                 } else {
                     if (widget->state != WIDGET_STATE_FOCUSED) {
                         widget->state = WIDGET_STATE_NORMAL;
                     }
                 }
            }
            break;

        case UI_EVENT_MOUSE_LEAVE:
            if (!slider->dragging && widget->state != WIDGET_STATE_FOCUSED) {
                widget->state = WIDGET_STATE_NORMAL;
            }
            break;

        default:
            break;
    }

    return false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

SliderWidget* slider_widget_create(const char* name) {
    SliderWidget* slider = memory_alloc(sizeof(SliderWidget));
    if (!slider) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate slider widget");
        return NULL;
    }

    // Initialize base widget
    if (!widget_init(&slider->widget, name)) {
        memory_free(slider);
        return NULL;
    }

    // Override virtual functions
    slider->widget.render = slider_render;
    slider->widget.handle_event = slider_handle_event;

    // Defaults
    slider->value = 0.0f;
    slider->min_value = 0.0f;
    slider->max_value = 1.0f;
    slider->step = 0.0f;
    slider->vertical = false;

    slider->track_thickness = 4.0f;
    slider->thumb_size = 16.0f;

    slider->track_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
    slider->thumb_color = (Vec4){0.8f, 0.8f, 0.8f, 1.0f};
    slider->active_track_color = (Vec4){0.2f, 0.6f, 1.0f, 1.0f};

    // Set default size
    slider->widget.size = (Vec2){200.0f, 24.0f};
    slider->widget.min_size = (Vec2){50.0f, 24.0f};

    return slider;
}

void slider_widget_set_value(SliderWidget* slider, float value) {
    if (!slider) return;

    // Clamp
    float new_value = fmaxf(slider->min_value, fminf(slider->max_value, value));

    // Apply step
    if (slider->step > 0.0f) {
        float steps = roundf((new_value - slider->min_value) / slider->step);
        new_value = slider->min_value + (steps * slider->step);
        new_value = fmaxf(slider->min_value, fminf(slider->max_value, new_value));
    }

    // Check if value changed significantly
    if (fabsf(slider->value - new_value) > 0.000001f) {
        slider->value = new_value;
        slider->widget.needs_redraw = true;

        if (slider->on_value_changed) {
            slider->on_value_changed(slider, slider->value, slider->callback_user_data);
        }
    }
}

void slider_widget_set_range(SliderWidget* slider, float min, float max) {
    if (!slider) return;

    slider->min_value = min;
    slider->max_value = max;

    // Re-clamp current value
    slider_widget_set_value(slider, slider->value);
}

void slider_widget_set_step(SliderWidget* slider, float step) {
    if (!slider) return;
    slider->step = step;
}

void slider_widget_set_vertical(SliderWidget* slider, bool vertical) {
    if (!slider) return;

    if (slider->vertical != vertical) {
        slider->vertical = vertical;
        // Swap default dimensions
        float temp_w = slider->widget.size.x;
        float temp_h = slider->widget.size.y;

        // Simple heuristic: if it looks like a default horizontal bar, flip it
        if (temp_w > temp_h) {
             slider->widget.size.x = temp_h;
             slider->widget.size.y = temp_w;
        }

        slider->widget.needs_layout = true;
    }
}

void slider_widget_set_callback(SliderWidget* slider, SliderValueChangedCallback callback, void* user_data) {
    if (!slider) return;
    slider->on_value_changed = callback;
    slider->callback_user_data = user_data;
}

float slider_widget_get_value(const SliderWidget* slider) {
    if (!slider) return 0.0f;
    return slider->value;
}
