/*
 * checkbox.c
 * CheckBox Widget Implementation
 * Boolean toggle with label
 */

#include "checkbox.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static void ui_checkbox_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UICheckBox* box = (UICheckBox*)widget;

    // TODO: Implement rendering
    // 1. Draw box (with check if checked)
    // 2. Draw label text next to it

    LOG_DEBUG(LOG_CAT_GENERAL, "CheckBox render: %s (%s)", box->label ? box->label : "unnamed",
              box->checked ? "checked" : "unchecked");
}

static void ui_checkbox_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    UICheckBox* box = (UICheckBox*)widget;
    Vec2 size = widget_get_size(widget);

    if (size.x <= 0 || size.y <= 0) {
        float width = box->box_size + (box->label ? strlen(box->label) * 8.0f : 0.0f) + box->text_padding;
        float height = fmaxf(box->box_size, 20.0f); // Assuming line height

        if (size.x <= 0) size.x = width;
        if (size.y <= 0) size.y = height;

        widget_set_size(widget, size);
    }

    widget->needs_layout = false;
}

static Size ui_checkbox_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};
    UICheckBox* box = (UICheckBox*)widget;

    float width = box->box_size + (box->label ? strlen(box->label) * 8.0f : 0.0f) + box->text_padding;
    float height = fmaxf(box->box_size, 20.0f);

    return (Size){width, height};
}

static bool ui_checkbox_handle_event_impl(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;
    if (!widget_is_enabled(widget)) return false;

    UICheckBox* box = (UICheckBox*)widget;

    switch (event->type) {
        case UI_EVENT_MOUSE_ENTER:
            widget_set_state(widget, WIDGET_STATE_HOVER);
            widget_invalidate_redraw(widget);
            break;

        case UI_EVENT_MOUSE_LEAVE:
            widget_set_state(widget, WIDGET_STATE_NORMAL);
            widget_invalidate_redraw(widget);
            break;

        case UI_EVENT_MOUSE_DOWN:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                widget_set_state(widget, WIDGET_STATE_PRESSED);
                widget_set_focused(widget, true);
                event->handled = true;
                return true;
            }
            break;

        case UI_EVENT_MOUSE_UP:
            if (event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                if (widget_get_state(widget) == WIDGET_STATE_PRESSED) {
                    if (widget_contains_point(widget, event->mouse.position)) {
                        ui_checkbox_toggle(box);
                        widget_set_state(widget, WIDGET_STATE_HOVER);
                    } else {
                        widget_set_state(widget, WIDGET_STATE_NORMAL);
                    }
                    event->handled = true;
                    return true;
                }
            }
            break;

        case UI_EVENT_KEY_PRESS:
            if (widget_is_focused(widget)) {
                if (event->keyboard.key_code == 32 || event->keyboard.key_code == 13) { // Space or Enter
                    ui_checkbox_toggle(box);
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

static void ui_checkbox_destroy_impl(Widget* widget) {
    if (!widget) return;
    UICheckBox* box = (UICheckBox*)widget;

    if (box->label) {
        free(box->label);
        box->label = NULL;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

UICheckBox* ui_checkbox_create(const char* name, const char* label, bool checked) {
    UICheckBox* box = memory_alloc(sizeof(UICheckBox));
    if (!box) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI checkbox");
        return NULL;
    }

    Widget* base = widget_create(name ? name : "CheckBox");
    if (!base) {
        free(box);
        return NULL;
    }

    memcpy(box, base, sizeof(Widget));

    box->checked = checked;
    box->label = label ? strdup(label) : NULL;

    // Visual defaults
    box->box_size = 16.0f;
    box->corner_radius = 2.0f;
    box->text_padding = 8.0f;

    box->box_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    box->check_color = (Vec4){0.2f, 0.8f, 0.2f, 1.0f};
    box->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    box->hover_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};

    box->base.render = ui_checkbox_render_impl;
    box->base.layout = ui_checkbox_layout_impl;
    box->base.measure = ui_checkbox_measure_impl;
    box->base.handle_event = ui_checkbox_handle_event_impl;
    box->base.destroy = ui_checkbox_destroy_impl;

    box->base.focusable = true;
    box->base.hoverable = true;

    ui_checkbox_layout_impl(&box->base, 0, 0);

    LOG_INFO(LOG_CAT_GENERAL, "Created UI checkbox: %s (%s)", name ? name : "unnamed", label ? label : "");
    return box;
}

void ui_checkbox_destroy(UICheckBox* checkbox) {
    if (!checkbox) return;

    if (checkbox->base.destroy) {
        checkbox->base.destroy(&checkbox->base);
    }

    if (checkbox->base.children) {
        free(checkbox->base.children);
    }
    if (checkbox->base.name) {
        free(checkbox->base.name);
    }

    free(checkbox);
}

void ui_checkbox_set_checked(UICheckBox* checkbox, bool checked) {
    if (!checkbox) return;

    if (checkbox->checked != checked) {
        checkbox->checked = checked;
        widget_invalidate_redraw(&checkbox->base);

        if (checkbox->on_toggled) {
            checkbox->on_toggled(checkbox, checkbox->checked, checkbox->user_data);
        }
    }
}

bool ui_checkbox_is_checked(const UICheckBox* checkbox) {
    if (!checkbox) return false;
    return checkbox->checked;
}

void ui_checkbox_toggle(UICheckBox* checkbox) {
    if (!checkbox) return;
    ui_checkbox_set_checked(checkbox, !checkbox->checked);
}

void ui_checkbox_set_label(UICheckBox* checkbox, const char* label) {
    if (!checkbox) return;

    if (checkbox->label) {
        free(checkbox->label);
    }

    checkbox->label = label ? strdup(label) : NULL;
    widget_invalidate_layout(&checkbox->base);
    widget_invalidate_redraw(&checkbox->base);
}

void ui_checkbox_set_colors(UICheckBox* checkbox, Vec4 box, Vec4 check, Vec4 text) {
    if (!checkbox) return;
    checkbox->box_color = box;
    checkbox->check_color = check;
    checkbox->text_color = text;
    widget_invalidate_redraw(&checkbox->base);
}

void ui_checkbox_set_box_size(UICheckBox* checkbox, float size) {
    if (!checkbox) return;
    checkbox->box_size = size;
    widget_invalidate_layout(&checkbox->base);
    widget_invalidate_redraw(&checkbox->base);
}

void ui_checkbox_set_on_toggled(UICheckBox* checkbox, CheckBoxToggledCallback callback, void* user_data) {
    if (!checkbox) return;
    checkbox->on_toggled = callback;
    checkbox->user_data = user_data;
}

Widget* ui_checkbox_get_widget(UICheckBox* checkbox) {
    if (!checkbox) return NULL;
    return &checkbox->base;
}
