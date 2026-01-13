/*
 * ui_button.c
 * Button Widget Implementation
 * Clickable button with text and visual state feedback
 */

#include "ui_button.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL RENDERING HELPERS
 * ============================================================================ */

static Vec4 ui_button_get_current_color(UIButton* button) {
    if (!widget_is_enabled(&button->base)) {
        return button->disabled_color;
    }

    float hover = button->hover_progress;

    if (widget_get_state(&button->base) == WIDGET_STATE_PRESSED) {
        return button->pressed_color;
    } else if (widget_get_state(&button->base) == WIDGET_STATE_HOVER) {
        // Interpolate between normal and hover color based on animation
        Vec4 result;
        result.x = button->normal_color.x + (button->hover_color.x - button->normal_color.x) * hover;
        result.y = button->normal_color.y + (button->hover_color.y - button->normal_color.y) * hover;
        result.z = button->normal_color.z + (button->hover_color.z - button->normal_color.z) * hover;
        result.w = button->normal_color.w + (button->hover_color.w - button->normal_color.w) * hover;
        return result;
    }

    return button->normal_color;
}

static void ui_button_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UIButton* button = (UIButton*)widget;
    Vec2 pos = widget_get_position(widget);
    Vec2 size = widget_get_size(widget);
    Vec4 bg_color = ui_button_get_current_color(button);

    // TODO: Implement actual rendering with graphics backend
    // For now, this is a placeholder
    // In a real implementation, this would:
    // 1. Render background rectangle with corner_radius
    // 2. Render border
    // 3. Render text centered
    // 4. Optionally render icon

    LOG_DEBUG("Button render: pos=(%.1f, %.1f) size=(%.1f, %.1f) text=%s",
             pos.x, pos.y, size.x, size.y, button->text ? button->text : "");
}

/* ============================================================================
 * INTERNAL EVENT HANDLING
 * ============================================================================ */

static bool ui_button_handle_event_impl(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;

    UIButton* button = (UIButton*)widget;

    switch (event->type) {
        case UI_EVENT_MOUSE_ENTER: {
            if (widget_is_enabled(widget)) {
                widget_set_state(widget, WIDGET_STATE_HOVER);
                button->hover_progress = 0.0f;
                widget_invalidate_redraw(widget);
            }
            break;
        }

        case UI_EVENT_MOUSE_LEAVE: {
            if (widget_get_state(widget) == WIDGET_STATE_HOVER ||
                widget_get_state(widget) == WIDGET_STATE_PRESSED) {
                widget_set_state(widget, WIDGET_STATE_NORMAL);
                button->hover_progress = 0.0f;
                widget_invalidate_redraw(widget);
            }
            break;
        }

        case UI_EVENT_MOUSE_DOWN: {
            if (!widget_is_enabled(widget)) break;

            // Check if click is on this button
            if (widget_contains_point(widget, event->mouse.position)) {
                widget_set_state(widget, WIDGET_STATE_PRESSED);
                widget_set_focused(widget, true);
                widget_invalidate_redraw(widget);

                if (button->on_press) {
                    button->on_press(button, button->user_data);
                }

                event->handled = true;
                return true;
            }
            break;
        }

        case UI_EVENT_MOUSE_UP: {
            if (!widget_is_enabled(widget)) break;

            // Check if we were pressed and click is still on us
            if (widget_get_state(widget) == WIDGET_STATE_PRESSED) {
                if (widget_contains_point(widget, event->mouse.position)) {
                    // This is a click
                    if (button->on_click) {
                        button->on_click(button, button->user_data);
                    }
                } else {
                    // Released outside button
                    widget_set_state(widget, WIDGET_STATE_NORMAL);
                    button->hover_progress = 0.0f;
                }

                if (button->on_release) {
                    button->on_release(button, button->user_data);
                }

                widget_invalidate_redraw(widget);
                event->handled = true;
                return true;
            }
            break;
        }

        case UI_EVENT_MOUSE_CLICK: {
            if (!widget_is_enabled(widget)) break;

            if (widget_contains_point(widget, event->mouse.position)) {
                if (button->on_click) {
                    button->on_click(button, button->user_data);
                }
                event->handled = true;
                return true;
            }
            break;
        }

        case UI_EVENT_KEY_PRESS: {
            // Space or Enter to activate button when focused
            if (widget_is_focused(widget) && widget_is_enabled(widget)) {
                if (event->keyboard.key_code == 32 ||  // Space
                    event->keyboard.key_code == 13) {   // Enter
                    ui_button_click(button);
                    event->handled = true;
                    return true;
                }
            }
            break;
        }

        default:
            break;
    }

    return false;
}

static void ui_button_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    // Default layout: button takes requested size or fills available space
    Vec2 size = widget_get_size(widget);

    if (size.x <= 0) {
        size.x = available_width > 0 ? available_width : 100.0f;
    }
    if (size.y <= 0) {
        size.y = available_height > 0 ? available_height : 32.0f;
    }

    widget_set_size(widget, size);
    widget->needs_layout = false;
}

static Size ui_button_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};

    UIButton* button = (UIButton*)widget;

    // Estimate text size (very rough - would need actual font metrics)
    float text_width = button->text_length * 8.0f + 20.0f; // Rough estimate
    float text_height = 20.0f;

    // Add padding
    text_width += 16.0f;
    text_height += 8.0f;

    return (Size){
        .width = fminf(text_width, available_width > 0 ? available_width : 200.0f),
        .height = fminf(text_height, available_height > 0 ? available_height : 40.0f)
    };
}

static void ui_button_destroy_impl(Widget* widget) {
    if (!widget) return;

    UIButton* button = (UIButton*)widget;

    if (button->text) {
        free(button->text);
        button->text = NULL;
    }

    // Don't call widget_destroy on base - caller should handle
}

/* ============================================================================
 * PUBLIC API - Creation and Destruction
 * ============================================================================ */

UIButton* ui_button_create(const char* name, const char* text) {
    UIButton* button = memory_alloc(sizeof(UIButton));
    if (!button) {
        LOG_ERROR("Failed to allocate UI button");
        return NULL;
    }

    // Initialize base widget
    Widget* base = widget_create(name ? name : "Button");
    if (!base) {
        free(button);
        return NULL;
    }

    // Copy base widget into button
    memcpy(button, base, sizeof(Widget));

    // Set button-specific defaults
    button->style = BUTTON_STYLE_DEFAULT;
    button->size = BUTTON_SIZE_MEDIUM;
    button->corner_radius = 4.0f;
    button->show_icon = false;
    button->icon_id = 0;
    button->hover_progress = 0.0f;

    // Default colors (light theme)
    button->normal_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};    // Dark gray
    button->hover_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};      // Slightly lighter
    button->pressed_color = (Vec4){0.15f, 0.15f, 0.15f, 1.0f}; // Darker
    button->disabled_color = (Vec4){0.5f, 0.5f, 0.5f, 0.5f};   // Semi-transparent gray
    button->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};       // White text

    // Set text
    if (text) {
        button->text = strdup(text);
        button->text_length = strlen(text);
    } else {
        button->text = strdup("");
        button->text_length = 0;
    }

    // Set virtual functions
    button->base.render = ui_button_render_impl;
    button->base.layout = ui_button_layout_impl;
    button->base.measure = ui_button_measure_impl;
    button->base.handle_event = ui_button_handle_event_impl;
    button->base.destroy = ui_button_destroy_impl;

    // Button defaults
    button->base.focusable = true;
    button->base.hoverable = true;

    // Default size
    widget_set_size(&button->base, (Vec2){100.0f, 32.0f});
    widget_set_min_size(&button->base, (Vec2){60.0f, 24.0f});

    LOG_INFO("Created UI button: %s with text '%s'", name ? name : "unnamed", text ? text : "");
    return button;
}

void ui_button_destroy(UIButton* button) {
    if (!button) return;

    if (button->base.destroy) {
        button->base.destroy(&button->base);
    }

    // Destroy all child widgets
    for (uint32_t i = 0; i < button->base.child_count; i++) {
        Widget* child = button->base.children[i];
        if (child && child->destroy) {
            child->destroy(child);
        }
    }

    if (button->base.children) {
        free(button->base.children);
    }

    if (button->base.name) {
        free(button->base.name);
    }

    free(button);
}

/* ============================================================================
 * PUBLIC API - Content Management
 * ============================================================================ */

void ui_button_set_text(UIButton* button, const char* text) {
    if (!button) return;

    if (button->text) {
        free(button->text);
    }

    if (text) {
        button->text = strdup(text);
        button->text_length = strlen(text);
    } else {
        button->text = strdup("");
        button->text_length = 0;
    }

    widget_invalidate_redraw(&button->base);
    widget_invalidate_layout(&button->base);
}

const char* ui_button_get_text(const UIButton* button) {
    if (!button || !button->text) return "";
    return button->text;
}

void ui_button_set_icon(UIButton* button, uint32_t icon_id) {
    if (!button) return;
    button->icon_id = icon_id;
    widget_invalidate_redraw(&button->base);
}

void ui_button_set_show_icon(UIButton* button, bool show) {
    if (!button) return;
    if (button->show_icon != show) {
        button->show_icon = show;
        widget_invalidate_redraw(&button->base);
        widget_invalidate_layout(&button->base);
    }
}

/* ============================================================================
 * PUBLIC API - Styling
 * ============================================================================ */

void ui_button_set_style(UIButton* button, ButtonStyle style) {
    if (!button) return;

    button->style = style;

    // Apply style-specific defaults
    switch (style) {
        case BUTTON_STYLE_PRIMARY:
            button->normal_color = (Vec4){0.2f, 0.6f, 1.0f, 1.0f};      // Blue
            button->hover_color = (Vec4){0.3f, 0.7f, 1.0f, 1.0f};       // Light blue
            button->pressed_color = (Vec4){0.1f, 0.5f, 0.9f, 1.0f};     // Dark blue
            break;

        case BUTTON_STYLE_DANGER:
            button->normal_color = (Vec4){0.8f, 0.2f, 0.2f, 1.0f};      // Red
            button->hover_color = (Vec4){0.9f, 0.3f, 0.3f, 1.0f};       // Light red
            button->pressed_color = (Vec4){0.7f, 0.1f, 0.1f, 1.0f};     // Dark red
            break;

        case BUTTON_STYLE_OUTLINE:
            button->base.border_width = 2.0f;
            button->normal_color = (Vec4){0.1f, 0.1f, 0.1f, 0.0f};      // Transparent
            button->hover_color = (Vec4){0.2f, 0.2f, 0.2f, 0.1f};       // Slight fill
            button->pressed_color = (Vec4){0.3f, 0.3f, 0.3f, 0.2f};     // More fill
            break;

        case BUTTON_STYLE_FLAT:
            button->normal_color = (Vec4){0.0f, 0.0f, 0.0f, 0.0f};      // Transparent
            button->hover_color = (Vec4){0.1f, 0.1f, 0.1f, 0.1f};       // Slight fill
            button->pressed_color = (Vec4){0.2f, 0.2f, 0.2f, 0.2f};     // More fill
            break;

        case BUTTON_STYLE_DEFAULT:
        default:
            button->normal_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
            button->hover_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
            button->pressed_color = (Vec4){0.15f, 0.15f, 0.15f, 1.0f};
            break;
    }

    widget_invalidate_redraw(&button->base);
}

void ui_button_set_size(UIButton* button, ButtonSize size) {
    if (!button) return;

    button->size = size;

    Vec2 new_size;
    switch (size) {
        case BUTTON_SIZE_SMALL:
            new_size = (Vec2){80.0f, 24.0f};
            break;
        case BUTTON_SIZE_LARGE:
            new_size = (Vec2){150.0f, 48.0f};
            break;
        case BUTTON_SIZE_MEDIUM:
        default:
            new_size = (Vec2){100.0f, 32.0f};
            break;
    }

    widget_set_size(&button->base, new_size);
}

void ui_button_set_corner_radius(UIButton* button, float radius) {
    if (!button) return;
    button->corner_radius = radius;
    widget_invalidate_redraw(&button->base);
}

void ui_button_set_colors(UIButton* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled) {
    if (!button) return;
    button->normal_color = normal;
    button->hover_color = hover;
    button->pressed_color = pressed;
    button->disabled_color = disabled;
    widget_invalidate_redraw(&button->base);
}

void ui_button_set_text_color(UIButton* button, Vec4 color) {
    if (!button) return;
    button->text_color = color;
    widget_invalidate_redraw(&button->base);
}

/* ============================================================================
 * PUBLIC API - State Management
 * ============================================================================ */

bool ui_button_is_pressed(const UIButton* button) {
    if (!button) return false;
    return widget_get_state(&button->base) == WIDGET_STATE_PRESSED;
}

bool ui_button_is_hovered(const UIButton* button) {
    if (!button) return false;
    return widget_get_state(&button->base) == WIDGET_STATE_HOVER;
}

/* ============================================================================
 * PUBLIC API - Callbacks
 * ============================================================================ */

void ui_button_set_on_click(UIButton* button,
                            void (*callback)(UIButton*, void*),
                            void* user_data) {
    if (!button) return;
    button->on_click = callback;
    button->user_data = user_data;
}

void ui_button_set_on_press(UIButton* button,
                            void (*callback)(UIButton*, void*),
                            void* user_data) {
    if (!button) return;
    button->on_press = callback;
    button->user_data = user_data;
}

void ui_button_set_on_release(UIButton* button,
                              void (*callback)(UIButton*, void*),
                              void* user_data) {
    if (!button) return;
    button->on_release = callback;
    button->user_data = user_data;
}

/* ============================================================================
 * PUBLIC API - Utility
 * ============================================================================ */

void ui_button_click(UIButton* button) {
    if (!button || !widget_is_enabled(&button->base)) return;

    widget_set_state(&button->base, WIDGET_STATE_PRESSED);
    widget_invalidate_redraw(&button->base);

    if (button->on_press) {
        button->on_press(button, button->user_data);
    }

    if (button->on_click) {
        button->on_click(button, button->user_data);
    }

    if (button->on_release) {
        button->on_release(button, button->user_data);
    }

    widget_set_state(&button->base, WIDGET_STATE_NORMAL);
    widget_invalidate_redraw(&button->base);
}

Widget* ui_button_get_widget(UIButton* button) {
    if (!button) return NULL;
    return &button->base;
}
