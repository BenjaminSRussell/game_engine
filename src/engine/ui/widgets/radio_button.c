/*
 * radio_button.c
 * RadioButton Widget Implementation
 * Single selection from a group of options
 */

#include "radio_button.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

// Forward declaration for type checking
static void ui_radio_button_render_impl(Widget* widget);

static bool is_radio_button(Widget* widget) {
    return widget && widget->render == ui_radio_button_render_impl;
}

static void ui_radio_button_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UIRadioButton* radio = (UIRadioButton*)widget;

    // TODO: Implement actual rendering
    // 1. Draw outer circle
    // 2. Draw inner circle if selected
    // 3. Draw text label

    LOG_DEBUG(LOG_CAT_GENERAL, "RadioButton render: %s group=%d (%s)", radio->label ? radio->label : "unnamed",
              radio->group_id, radio->selected ? "selected" : "unselected");
}

static void ui_radio_button_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    UIRadioButton* radio = (UIRadioButton*)widget;
    Vec2 size = widget_get_size(widget);

    if (size.x <= 0 || size.y <= 0) {
        float width = radio->circle_size + (radio->label ? strlen(radio->label) * 8.0f : 0.0f) + radio->text_padding;
        float height = fmaxf(radio->circle_size, 20.0f);

        if (size.x <= 0) size.x = width;
        if (size.y <= 0) size.y = height;

        widget_set_size(widget, size);
    }

    widget->needs_layout = false;
}

static Size ui_radio_button_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};
    UIRadioButton* radio = (UIRadioButton*)widget;

    float width = radio->circle_size + (radio->label ? strlen(radio->label) * 8.0f : 0.0f) + radio->text_padding;
    float height = fmaxf(radio->circle_size, 20.0f);

    return (Size){width, height};
}

static void ui_radio_button_destroy_impl(Widget* widget) {
    if (!widget) return;
    UIRadioButton* radio = (UIRadioButton*)widget;

    if (radio->label) {
        free(radio->label);
        radio->label = NULL;
    }
}

static void deselect_siblings(UIRadioButton* radio) {
    if (!radio || !radio->base.parent) return;

    Widget* parent = radio->base.parent;
    for (uint32_t i = 0; i < parent->child_count; i++) {
        Widget* sibling = parent->children[i];

        // Skip self
        if (sibling == &radio->base) continue;

        // Check if sibling is a radio button
        if (is_radio_button(sibling)) {
            UIRadioButton* sibling_radio = (UIRadioButton*)sibling;

            // Check group ID
            if (sibling_radio->group_id == radio->group_id) {
                if (sibling_radio->selected) {
                    sibling_radio->selected = false;
                    widget_invalidate_redraw(sibling);

                    if (sibling_radio->on_selected) {
                        sibling_radio->on_selected(sibling_radio, false, sibling_radio->user_data);
                    }
                }
            }
        }
    }
}

static bool ui_radio_button_handle_event_impl(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;
    if (!widget_is_enabled(widget)) return false;

    UIRadioButton* radio = (UIRadioButton*)widget;

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
                        ui_radio_button_set_selected(radio, true);
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
                    ui_radio_button_set_selected(radio, true);
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

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

UIRadioButton* ui_radio_button_create(const char* name, const char* label, int group_id) {
    UIRadioButton* radio = memory_alloc(sizeof(UIRadioButton));
    if (!radio) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI radio button");
        return NULL;
    }

    Widget* base = widget_create(name ? name : "RadioButton");
    if (!base) {
        free(radio);
        return NULL;
    }

    memcpy(radio, base, sizeof(Widget));

    radio->selected = false;
    radio->group_id = group_id;
    radio->label = label ? strdup(label) : NULL;

    // Visual defaults
    radio->circle_size = 16.0f;
    radio->text_padding = 8.0f;

    radio->circle_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    radio->selected_color = (Vec4){0.2f, 0.6f, 1.0f, 1.0f};
    radio->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    radio->hover_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};

    radio->base.render = ui_radio_button_render_impl;
    radio->base.layout = ui_radio_button_layout_impl;
    radio->base.measure = ui_radio_button_measure_impl;
    radio->base.handle_event = ui_radio_button_handle_event_impl;
    radio->base.destroy = ui_radio_button_destroy_impl;

    radio->base.focusable = true;
    radio->base.hoverable = true;

    ui_radio_button_layout_impl(&radio->base, 0, 0);

    LOG_INFO(LOG_CAT_GENERAL, "Created UI radio button: %s (%s, group=%d)", name ? name : "unnamed", label ? label : "", group_id);
    return radio;
}

void ui_radio_button_destroy(UIRadioButton* radio) {
    if (!radio) return;

    if (radio->base.destroy) {
        radio->base.destroy(&radio->base);
    }

    if (radio->base.children) {
        free(radio->base.children);
    }
    if (radio->base.name) {
        free(radio->base.name);
    }

    free(radio);
}

void ui_radio_button_set_selected(UIRadioButton* radio, bool selected) {
    if (!radio) return;

    // Radio buttons can usually only be selected by user, not deselected (except by selecting another)
    // But API should allow setting false
    if (radio->selected != selected) {
        radio->selected = selected;
        widget_invalidate_redraw(&radio->base);

        if (selected) {
            deselect_siblings(radio);
        }

        if (radio->on_selected) {
            radio->on_selected(radio, radio->selected, radio->user_data);
        }
    }
}

bool ui_radio_button_is_selected(const UIRadioButton* radio) {
    if (!radio) return false;
    return radio->selected;
}

void ui_radio_button_set_group(UIRadioButton* radio, int group_id) {
    if (!radio) return;
    radio->group_id = group_id;
}

void ui_radio_button_set_label(UIRadioButton* radio, const char* label) {
    if (!radio) return;

    if (radio->label) {
        free(radio->label);
    }

    radio->label = label ? strdup(label) : NULL;
    widget_invalidate_layout(&radio->base);
    widget_invalidate_redraw(&radio->base);
}

void ui_radio_button_set_colors(UIRadioButton* radio, Vec4 circle, Vec4 selected, Vec4 text) {
    if (!radio) return;
    radio->circle_color = circle;
    radio->selected_color = selected;
    radio->text_color = text;
    widget_invalidate_redraw(&radio->base);
}

void ui_radio_button_set_circle_size(UIRadioButton* radio, float size) {
    if (!radio) return;
    radio->circle_size = size;
    widget_invalidate_layout(&radio->base);
    widget_invalidate_redraw(&radio->base);
}

void ui_radio_button_set_on_selected(UIRadioButton* radio, RadioButtonSelectedCallback callback, void* user_data) {
    if (!radio) return;
    radio->on_selected = callback;
    radio->user_data = user_data;
}

Widget* ui_radio_button_get_widget(UIRadioButton* radio) {
    if (!radio) return NULL;
    return &radio->base;
}
