/*
 * slider.c
 * Slider Widget Implementation
 * Input widget for selecting a value from a continuous or discrete range
 */

#include "slider.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static float ui_clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float snap_value(float value, float step, float min) {
    if (step <= 0.0001f) return value;
    float relative = value - min;
    float steps = roundf(relative / step);
    return min + (steps * step);
}

static void update_value_from_position(UISlider* slider, Vec2 mouse_pos) {
    Vec2 pos = widget_get_position(&slider->base);
    Vec2 size = widget_get_size(&slider->base);

    // Calculate usable track length
    float track_len;
    float relative_pos;
    float padding = slider->handle_size * 0.5f;

    if (slider->orientation == UI_ORIENTATION_HORIZONTAL) {
        track_len = size.x - 2.0f * padding;
        if (track_len <= 0) track_len = 1.0f;

        relative_pos = mouse_pos.x - (pos.x + padding);
    } else {
        track_len = size.y - 2.0f * padding;
        if (track_len <= 0) track_len = 1.0f;

        // Vertical sliders usually go bottom-to-top or top-to-bottom
        // Let's assume top-to-bottom for standard GUI coordinates where y increases downwards
        relative_pos = mouse_pos.y - (pos.y + padding);
    }

    float t = ui_clamp(relative_pos / track_len, 0.0f, 1.0f);
    float new_value = slider->min_value + t * (slider->max_value - slider->min_value);

    // Apply stepping
    new_value = snap_value(new_value, slider->step, slider->min_value);
    new_value = ui_clamp(new_value, slider->min_value, slider->max_value);

    if (new_value != slider->value) {
        ui_slider_set_value(slider, new_value);
    }
}

static void ui_slider_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UISlider* slider = (UISlider*)widget;

    // TODO: Implement actual rendering
    // Render track, filled portion, and handle

    LOG_DEBUG(LOG_CAT_GENERAL, "Slider render: val=%.2f [%.2f, %.2f]", slider->value, slider->min_value, slider->max_value);
}

static void ui_slider_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    UISlider* slider = (UISlider*)widget;
    Vec2 size = widget_get_size(widget);

    // Set default size if not set
    if (size.x <= 0 || size.y <= 0) {
        if (slider->orientation == UI_ORIENTATION_HORIZONTAL) {
            if (size.x <= 0) size.x = available_width > 0 ? fminf(available_width, 200.0f) : 200.0f;
            if (size.y <= 0) size.y = 20.0f;
        } else {
            if (size.x <= 0) size.x = 20.0f;
            if (size.y <= 0) size.y = available_height > 0 ? fminf(available_height, 200.0f) : 200.0f;
        }
        widget_set_size(widget, size);
    }

    widget->needs_layout = false;
}

static Size ui_slider_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};
    UISlider* slider = (UISlider*)widget;

    if (slider->orientation == UI_ORIENTATION_HORIZONTAL) {
        return (Size){200.0f, 20.0f};
    } else {
        return (Size){20.0f, 200.0f};
    }
}

static bool ui_slider_handle_event_impl(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;
    if (!widget_is_enabled(widget)) return false;

    UISlider* slider = (UISlider*)widget;

    switch (event->type) {
        case UI_EVENT_MOUSE_ENTER:
            widget_set_state(widget, WIDGET_STATE_HOVER);
            widget_invalidate_redraw(widget);
            break;

        case UI_EVENT_MOUSE_LEAVE:
            if (!slider->is_dragging) {
                widget_set_state(widget, WIDGET_STATE_NORMAL);
            }
            widget_invalidate_redraw(widget);
            break;

        case UI_EVENT_MOUSE_DOWN:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                if (widget_contains_point(widget, event->mouse.position)) {
                    slider->is_dragging = true;
                    widget_set_state(widget, WIDGET_STATE_PRESSED);
                    widget_set_focused(widget, true);

                    update_value_from_position(slider, event->mouse.position);

                    event->handled = true;
                    return true;
                }
            }
            break;

        case UI_EVENT_MOUSE_UP:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                if (slider->is_dragging) {
                    slider->is_dragging = false;
                    if (widget_contains_point(widget, event->mouse.position)) {
                        widget_set_state(widget, WIDGET_STATE_HOVER);
                    } else {
                        widget_set_state(widget, WIDGET_STATE_NORMAL);
                    }
                    widget_invalidate_redraw(widget);
                    event->handled = true;
                    return true;
                }
            }
            break;

        case UI_EVENT_MOUSE_MOVE:
            if (slider->is_dragging) {
                update_value_from_position(slider, event->mouse.position);
                event->handled = true;
                return true;
            }
            break;

        case UI_EVENT_MOUSE_WHEEL:
            if (widget_is_hover(widget) || widget_is_focused(widget)) {
                float delta = event->mouse.wheel_delta;
                float change = (slider->step > 0) ? slider->step : (slider->max_value - slider->min_value) * 0.05f;

                ui_slider_set_value(slider, slider->value + (delta * change));
                event->handled = true;
                return true;
            }
            break;

        case UI_EVENT_KEY_DOWN:
            if (widget_is_focused(widget)) {
                float change = (slider->step > 0) ? slider->step : (slider->max_value - slider->min_value) * 0.05f;
                bool handled = false;

                if (event->keyboard.key_code == 37 || event->keyboard.key_code == 40) { // Left or Down
                    ui_slider_set_value(slider, slider->value - change);
                    handled = true;
                } else if (event->keyboard.key_code == 39 || event->keyboard.key_code == 38) { // Right or Up
                    ui_slider_set_value(slider, slider->value + change);
                    handled = true;
                }

                if (handled) {
                    event->handled = true;
                    return true;
                }
            }
            break;

        default:
            break;
    }

    return false;
}

static void ui_slider_destroy_impl(Widget* widget) {
    if (!widget) return;
    // Nothing specific to free for slider struct itself, as it doesn't own dynamic strings besides what Widget owns
}


/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

UISlider* ui_slider_create(const char* name, float value, float min, float max, UIOrientation orientation) {
    UISlider* slider = memory_alloc(sizeof(UISlider));
    if (!slider) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI slider");
        return NULL;
    }

    Widget* base = widget_create(name ? name : "Slider");
    if (!base) {
        free(slider);
        return NULL;
    }

    memcpy(slider, base, sizeof(Widget));

    slider->value = ui_clamp(value, min, max);
    slider->min_value = min;
    slider->max_value = max;
    slider->step = 0.0f;
    slider->orientation = orientation;

    // Defaults
    slider->track_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
    slider->fill_color = (Vec4){0.2f, 0.6f, 1.0f, 1.0f};
    slider->handle_color = (Vec4){0.9f, 0.9f, 0.9f, 1.0f};
    slider->handle_hover_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    slider->handle_pressed_color = (Vec4){0.8f, 0.8f, 0.8f, 1.0f};

    slider->track_thickness = 4.0f;
    slider->handle_size = 16.0f;
    slider->handle_radius = 8.0f;

    slider->hover_progress = 0.0f;
    slider->is_dragging = false;

    slider->base.render = ui_slider_render_impl;
    slider->base.layout = ui_slider_layout_impl;
    slider->base.measure = ui_slider_measure_impl;
    slider->base.handle_event = ui_slider_handle_event_impl;
    slider->base.destroy = ui_slider_destroy_impl;

    slider->base.focusable = true;
    slider->base.hoverable = true;

    // Initial layout
    ui_slider_layout_impl(&slider->base, 0, 0);

    LOG_INFO(LOG_CAT_GENERAL, "Created UI slider: %s (%.2f - %.2f)", name ? name : "unnamed", min, max);
    return slider;
}

void ui_slider_destroy(UISlider* slider) {
    if (!slider) return;

    if (slider->base.destroy) {
        slider->base.destroy(&slider->base);
    }

    if (slider->base.children) {
        free(slider->base.children);
    }
    if (slider->base.name) {
        free(slider->base.name);
    }

    free(slider);
}

void ui_slider_set_value(UISlider* slider, float value) {
    if (!slider) return;

    float new_value = ui_clamp(value, slider->min_value, slider->max_value);
    new_value = snap_value(new_value, slider->step, slider->min_value);

    if (new_value != slider->value) {
        slider->value = new_value;
        widget_invalidate_redraw(&slider->base);

        if (slider->on_value_changed) {
            slider->on_value_changed(slider, slider->value, slider->user_data);
        }
    }
}

float ui_slider_get_value(const UISlider* slider) {
    if (!slider) return 0.0f;
    return slider->value;
}

void ui_slider_set_range(UISlider* slider, float min, float max) {
    if (!slider) return;
    slider->min_value = min;
    slider->max_value = max;
    // Re-clamp value
    ui_slider_set_value(slider, slider->value);
    widget_invalidate_redraw(&slider->base);
}

void ui_slider_set_step(UISlider* slider, float step) {
    if (!slider) return;
    slider->step = step;
}

void ui_slider_set_orientation(UISlider* slider, UIOrientation orientation) {
    if (!slider) return;
    slider->orientation = orientation;
    widget_invalidate_layout(&slider->base);
    widget_invalidate_redraw(&slider->base);
}

void ui_slider_set_colors(UISlider* slider, Vec4 track, Vec4 fill, Vec4 handle) {
    if (!slider) return;
    slider->track_color = track;
    slider->fill_color = fill;
    slider->handle_color = handle;
    widget_invalidate_redraw(&slider->base);
}

void ui_slider_set_handle_size(UISlider* slider, float size) {
    if (!slider) return;
    slider->handle_size = size;
    slider->handle_radius = size * 0.5f;
    widget_invalidate_layout(&slider->base);
    widget_invalidate_redraw(&slider->base);
}

void ui_slider_set_on_value_changed(UISlider* slider, SliderValueChangedCallback callback, void* user_data) {
    if (!slider) return;
    slider->on_value_changed = callback;
    slider->user_data = user_data;
}

Widget* ui_slider_get_widget(UISlider* slider) {
    if (!slider) return NULL;
    return &slider->base;
}
