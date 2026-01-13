/*
 * dialog.c
 * Dialog/Modal Widget Implementation
 * Customizable dialog boxes with various configurations and callbacks
 */

#include "dialog.h"
#include "ui_button.h"
#include "ui_label.h"
#include "../modal_manager.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static void dialog_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;
    UIDialog* dialog = (UIDialog*)widget;

    // TODO: Implement actual rendering
    // Render dialog background, title bar, content, buttons, shadow
    LOG_DEBUG("Dialog render: %s (fade=%.2f)", dialog->title ? dialog->title : "Dialog",
             dialog->fade_progress);
}

static void dialog_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;
    UIDialog* dialog = (UIDialog*)widget;

    // Default dialog size
    Vec2 size = widget_get_size(widget);
    if (size.x <= 0) size.x = 400.0f;
    if (size.y <= 0) size.y = 300.0f;

    // Apply size constraints
    if (dialog->min_size.x > 0) size.x = fmaxf(size.x, dialog->min_size.x);
    if (dialog->min_size.y > 0) size.y = fmaxf(size.y, dialog->min_size.y);
    if (dialog->max_size.x > 0) size.x = fminf(size.x, dialog->max_size.x);
    if (dialog->max_size.y > 0) size.y = fminf(size.y, dialog->max_size.y);

    widget_set_size(widget, size);

    // Center dialog on screen
    Vec2 pos = (Vec2){
        (available_width - size.x) * 0.5f,
        (available_height - size.y) * 0.5f
    };
    widget_set_position(widget, pos);

    widget->needs_layout = false;
}

static Size dialog_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};
    return (Size){400.0f, 300.0f};
}

static bool dialog_handle_event_impl(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;
    UIDialog* dialog = (UIDialog*)widget;

    switch (event->type) {
        case UI_EVENT_KEY_DOWN: {
            // ESC to close
            if (dialog->close_on_escape && event->keyboard.key_code == 27) {
                dialog_close(dialog, DIALOG_RESULT_CANCEL);
                event->handled = true;
                return true;
            }
            break;
        }

        case UI_EVENT_MOUSE_DOWN: {
            // Check for title bar drag
            if (dialog->draggable && dialog->title_bar) {
                if (widget_contains_point(dialog->title_bar, event->mouse.position)) {
                    dialog->drag_offset = event->mouse.position;
                    dialog->drag_offset.x -= widget_get_position(widget).x;
                    dialog->drag_offset.y -= widget_get_position(widget).y;
                    event->handled = true;
                    return true;
                }
            }
            break;
        }

        case UI_EVENT_MOUSE_MOVE: {
            // Handle title bar drag
            if (dialog->draggable && event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
                Vec2 new_pos = (Vec2){
                    event->mouse.position.x - dialog->drag_offset.x,
                    event->mouse.position.y - dialog->drag_offset.y
                };
                widget_set_position(widget, new_pos);
                event->handled = true;
                return true;
            }
            break;
        }

        default:
            break;
    }

    return false;
}

static void dialog_destroy_impl(Widget* widget) {
    if (!widget) return;
    UIDialog* dialog = (UIDialog*)widget;

    if (dialog->title) {
        free(dialog->title);
        dialog->title = NULL;
    }
    if (dialog->message) {
        free(dialog->message);
        dialog->message = NULL;
    }
    if (dialog->prompt_result) {
        free(dialog->prompt_result);
        dialog->prompt_result = NULL;
    }
}

static void dialog_on_button_click(UIButton* button, void* user_data) {
    UIDialog* dialog = (UIDialog*)user_data;
    if (!dialog) return;

    // Determine which button was clicked
    DialogResult result = DIALOG_RESULT_NONE;

    if (button == (UIButton*)dialog->ok_button) {
        result = DIALOG_RESULT_OK;
    } else if (button == (UIButton*)dialog->cancel_button) {
        result = DIALOG_RESULT_CANCEL;
    } else if (button == (UIButton*)dialog->yes_button) {
        result = DIALOG_RESULT_YES;
    } else if (button == (UIButton*)dialog->no_button) {
        result = DIALOG_RESULT_NO;
    } else if (button == (UIButton*)dialog->apply_button) {
        result = DIALOG_RESULT_APPLY;
    } else if (button == (UIButton*)dialog->reset_button) {
        result = DIALOG_RESULT_RESET;
    }

    dialog_close(dialog, result);
}

static void dialog_create_buttons(UIDialog* dialog) {
    if (!dialog || !dialog->button_container) return;

    // Create buttons based on flags
    if (dialog->button_flags & DIALOG_BUTTON_OK) {
        dialog->ok_button = (Widget*)ui_button_create("OK", "OK");
        if (dialog->ok_button) {
            ui_button_set_on_click((UIButton*)dialog->ok_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->ok_button);
        }
    }

    if (dialog->button_flags & DIALOG_BUTTON_CANCEL) {
        dialog->cancel_button = (Widget*)ui_button_create("Cancel", "Cancel");
        if (dialog->cancel_button) {
            ui_button_set_on_click((UIButton*)dialog->cancel_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->cancel_button);
        }
    }

    if (dialog->button_flags & DIALOG_BUTTON_YES) {
        dialog->yes_button = (Widget*)ui_button_create("Yes", "Yes");
        if (dialog->yes_button) {
            ui_button_set_on_click((UIButton*)dialog->yes_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->yes_button);
        }
    }

    if (dialog->button_flags & DIALOG_BUTTON_NO) {
        dialog->no_button = (Widget*)ui_button_create("No", "No");
        if (dialog->no_button) {
            ui_button_set_on_click((UIButton*)dialog->no_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->no_button);
        }
    }

    if (dialog->button_flags & DIALOG_BUTTON_APPLY) {
        dialog->apply_button = (Widget*)ui_button_create("Apply", "Apply");
        if (dialog->apply_button) {
            ui_button_set_on_click((UIButton*)dialog->apply_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->apply_button);
        }
    }

    if (dialog->button_flags & DIALOG_BUTTON_RESET) {
        dialog->reset_button = (Widget*)ui_button_create("Reset", "Reset");
        if (dialog->reset_button) {
            ui_button_set_on_click((UIButton*)dialog->reset_button, dialog_on_button_click, dialog);
            widget_add_child(dialog->button_container, dialog->reset_button);
        }
    }
}

/* ============================================================================
 * PUBLIC API - Creation and Destruction
 * ============================================================================ */

UIDialog* dialog_create(const char* name, const char* title, const char* message, DialogType type) {
    UIDialog* dialog = memory_alloc(sizeof(UIDialog));
    if (!dialog) {
        LOG_ERROR("Failed to allocate dialog");
        return NULL;
    }

    // Initialize base widget
    Widget* base = widget_create(name ? name : "Dialog");
    if (!base) {
        free(dialog);
        return NULL;
    }

    memcpy(dialog, base, sizeof(Widget));

    // Set dialog-specific defaults
    dialog->type = type;
    dialog->title = title ? strdup(title) : strdup("");
    dialog->message = message ? strdup(message) : strdup("");
    dialog->button_flags = DIALOG_BUTTON_OK;  // Default
    dialog->is_modal = true;
    dialog->close_on_backdrop_click = false;
    dialog->close_on_escape = true;
    dialog->draggable = true;
    dialog->resizable = false;
    dialog->min_size = (Vec2){200.0f, 100.0f};
    dialog->max_size = (Vec2){0.0f, 0.0f};  // No maximum
    dialog->fade_progress = 0.0f;
    dialog->target_fade = 1.0f;
    dialog->is_animating = true;
    dialog->is_closing = false;
    dialog->result = DIALOG_RESULT_NONE;
    dialog->prompt_result = NULL;

    // Visual properties
    dialog->title_bg_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    dialog->content_bg_color = (Vec4){0.25f, 0.25f, 0.25f, 1.0f};
    dialog->corner_radius = 8.0f;
    dialog->shadow_offset = 4.0f;
    dialog->shadow_color = (Vec4){0.0f, 0.0f, 0.0f, 0.3f};

    // Create child widgets
    dialog->title_bar = widget_create("TitleBar");
    if (dialog->title_bar) {
        widget_add_child(&dialog->base, dialog->title_bar);
        widget_set_size(dialog->title_bar, (Vec2){400.0f, 30.0f});
    }

    dialog->close_button = (Widget*)ui_button_create("CloseBtn", "X");
    if (dialog->close_button && dialog->title_bar) {
        ui_button_set_on_click((UIButton*)dialog->close_button, dialog_on_button_click, dialog);
        widget_add_child(dialog->title_bar, dialog->close_button);
    }

    dialog->content_container = widget_create("Content");
    if (dialog->content_container) {
        widget_add_child(&dialog->base, dialog->content_container);
    }

    // Add message label
    if (dialog->message && strlen(dialog->message) > 0) {
        Widget* message_label = (Widget*)ui_label_create("Message", dialog->message);
        if (message_label && dialog->content_container) {
            widget_add_child(dialog->content_container, message_label);
        }
    }

    dialog->button_container = widget_create("Buttons");
    if (dialog->button_container) {
        widget_add_child(&dialog->base, dialog->button_container);
    }

    // Set default buttons based on type
    switch (type) {
        case DIALOG_TYPE_ALERT:
            dialog->button_flags = DIALOG_BUTTON_OK;
            dialog->is_modal = false;
            break;
        case DIALOG_TYPE_CONFIRM:
            dialog->button_flags = DIALOG_BUTTON_OK | DIALOG_BUTTON_CANCEL;
            dialog->is_modal = true;
            break;
        case DIALOG_TYPE_PROMPT:
            dialog->button_flags = DIALOG_BUTTON_OK | DIALOG_BUTTON_CANCEL;
            dialog->is_modal = true;
            break;
        case DIALOG_TYPE_WARNING:
        case DIALOG_TYPE_ERROR:
            dialog->button_flags = DIALOG_BUTTON_OK;
            break;
        case DIALOG_TYPE_CUSTOM:
            dialog->button_flags = 0;  // User configures buttons
            break;
    }

    // Create buttons
    dialog_create_buttons(dialog);

    // Set virtual functions
    dialog->base.render = dialog_render_impl;
    dialog->base.layout = dialog_layout_impl;
    dialog->base.measure = dialog_measure_impl;
    dialog->base.handle_event = dialog_handle_event_impl;
    dialog->base.destroy = dialog_destroy_impl;

    // Dialog configuration
    dialog->base.focusable = true;
    dialog->base.z_index = 1000;  // High Z for visibility

    // Default size
    widget_set_size(&dialog->base, (Vec2){400.0f, 300.0f});

    LOG_INFO("Created dialog: %s (type=%d, modal=%d)", title ? title : "unnamed", type, dialog->is_modal);
    return dialog;
}

void dialog_destroy(UIDialog* dialog) {
    if (!dialog) return;

    if (dialog->base.destroy) {
        dialog->base.destroy(&dialog->base);
    }

    // Destroy child widgets
    for (uint32_t i = 0; i < dialog->base.child_count; i++) {
        Widget* child = dialog->base.children[i];
        if (child) {
            if (child->destroy) child->destroy(child);
            free(child);
        }
    }

    if (dialog->base.children) {
        free(dialog->base.children);
    }
    if (dialog->base.name) {
        free(dialog->base.name);
    }

    free(dialog);
}

/* ============================================================================
 * PUBLIC API - Configuration
 * ============================================================================ */

void dialog_set_modal(UIDialog* dialog, bool modal) {
    if (!dialog) return;
    dialog->is_modal = modal;
}

void dialog_set_draggable(UIDialog* dialog, bool draggable) {
    if (!dialog) return;
    dialog->draggable = draggable;
}

void dialog_set_resizable(UIDialog* dialog, bool resizable) {
    if (!dialog) return;
    dialog->resizable = resizable;
}

void dialog_set_size_constraints(UIDialog* dialog, Vec2 min_size, Vec2 max_size) {
    if (!dialog) return;
    dialog->min_size = min_size;
    dialog->max_size = max_size;
}

void dialog_set_buttons(UIDialog* dialog, uint32_t flags) {
    if (!dialog) return;
    dialog->button_flags = flags;

    // Recreate buttons
    if (dialog->button_container) {
        // Clear existing buttons
        for (uint32_t i = 0; i < dialog->button_container->child_count; i++) {
            dialog->button_container->children[i] = NULL;
        }
        dialog->button_container->child_count = 0;

        // Create new buttons
        dialog_create_buttons(dialog);
    }
}

void dialog_set_close_behavior(UIDialog* dialog, bool close_on_backdrop, bool close_on_escape) {
    if (!dialog) return;
    dialog->close_on_backdrop_click = close_on_backdrop;
    dialog->close_on_escape = close_on_escape;
}

void dialog_add_custom_widget(UIDialog* dialog, Widget* widget) {
    if (!dialog || !widget || !dialog->content_container) return;
    widget_add_child(dialog->content_container, widget);
}

void dialog_set_messages(UIDialog* dialog, const char* title, const char* message) {
    if (!dialog) return;
    if (title) {
        if (dialog->title) free(dialog->title);
        dialog->title = strdup(title);
    }
    if (message) {
        if (dialog->message) free(dialog->message);
        dialog->message = strdup(message);
    }
}

/* ============================================================================
 * PUBLIC API - Callbacks
 * ============================================================================ */

void dialog_set_result_callback(UIDialog* dialog, DialogResultCallback callback, void* user_data) {
    if (!dialog) return;
    dialog->on_result = callback;
    dialog->user_data = user_data;
}

void dialog_set_close_callback(UIDialog* dialog, DialogCloseCallback callback, void* user_data) {
    if (!dialog) return;
    dialog->on_close = callback;
}

/* ============================================================================
 * PUBLIC API - Display Control
 * ============================================================================ */

void dialog_show(UIDialog* dialog) {
    if (!dialog) return;
    widget_set_visible(&dialog->base, true);
    dialog->fade_progress = 0.0f;
    dialog->target_fade = 1.0f;
    dialog->is_animating = true;

    if (dialog->is_modal) {
        ModalManager* manager = modal_manager_get_global();
        if (manager) {
            modal_manager_push(manager, dialog);
        }
    }

    LOG_INFO("Showed dialog: %s", dialog->title ? dialog->title : "unnamed");
}

void dialog_hide(UIDialog* dialog) {
    if (!dialog) return;
    dialog->is_closing = true;
    dialog->target_fade = 0.0f;
    dialog->is_animating = true;
}

void dialog_close(UIDialog* dialog, DialogResult result) {
    if (!dialog) return;
    dialog->result = result;

    // Call result callback
    if (dialog->on_result) {
        dialog->on_result(dialog, result, dialog->user_data);
    }

    // Call close callback
    if (dialog->on_close) {
        dialog->on_close(dialog, dialog->user_data);
    }

    // Remove from modal manager
    if (dialog->is_modal) {
        ModalManager* manager = modal_manager_get_global();
        if (manager) {
            modal_manager_remove(manager, dialog);
        }
    }

    // Hide dialog
    dialog_hide(dialog);
}

/* ============================================================================
 * PUBLIC API - State Query
 * ============================================================================ */

bool dialog_is_visible(const UIDialog* dialog) {
    if (!dialog) return false;
    return widget_is_visible(&dialog->base);
}

bool dialog_is_modal(const UIDialog* dialog) {
    if (!dialog) return false;
    return dialog->is_modal;
}

DialogResult dialog_get_result(const UIDialog* dialog) {
    if (!dialog) return DIALOG_RESULT_NONE;
    return dialog->result;
}

const char* dialog_get_prompt_text(const UIDialog* dialog) {
    if (!dialog || !dialog->prompt_result) return "";
    return dialog->prompt_result;
}

Widget* dialog_get_widget(UIDialog* dialog) {
    if (!dialog) return NULL;
    return &dialog->base;
}

/* ============================================================================
 * PUBLIC API - Visual Customization
 * ============================================================================ */

void dialog_set_title_bg_color(UIDialog* dialog, Vec4 color) {
    if (!dialog) return;
    dialog->title_bg_color = color;
    widget_invalidate_redraw(&dialog->base);
}

void dialog_set_content_bg_color(UIDialog* dialog, Vec4 color) {
    if (!dialog) return;
    dialog->content_bg_color = color;
    widget_invalidate_redraw(&dialog->base);
}

void dialog_set_corner_radius(UIDialog* dialog, float radius) {
    if (!dialog) return;
    dialog->corner_radius = radius;
    widget_invalidate_redraw(&dialog->base);
}

void dialog_set_shadow(UIDialog* dialog, float offset, Vec4 color) {
    if (!dialog) return;
    dialog->shadow_offset = offset;
    dialog->shadow_color = color;
    widget_invalidate_redraw(&dialog->base);
}

/* ============================================================================
 * PUBLIC API - Helper Functions
 * ============================================================================ */

UIDialog* dialog_alert(const char* title, const char* message) {
    return dialog_create("Alert", title, message, DIALOG_TYPE_ALERT);
}

UIDialog* dialog_confirm(const char* title, const char* message) {
    return dialog_create("Confirm", title, message, DIALOG_TYPE_CONFIRM);
}

UIDialog* dialog_yes_no(const char* title, const char* message) {
    UIDialog* dialog = dialog_create("YesNo", title, message, DIALOG_TYPE_CONFIRM);
    if (dialog) {
        dialog_set_buttons(dialog, DIALOG_BUTTON_YES | DIALOG_BUTTON_NO);
    }
    return dialog;
}

UIDialog* dialog_warning(const char* title, const char* message) {
    UIDialog* dialog = dialog_create("Warning", title, message, DIALOG_TYPE_WARNING);
    if (dialog) {
        // Apply warning style (orange-ish)
        dialog_set_title_bg_color(dialog, (Vec4){0.8f, 0.6f, 0.2f, 1.0f});
    }
    return dialog;
}

UIDialog* dialog_error(const char* title, const char* message) {
    UIDialog* dialog = dialog_create("Error", title, message, DIALOG_TYPE_ERROR);
    if (dialog) {
        // Apply error style (red-ish)
        dialog_set_title_bg_color(dialog, (Vec4){0.8f, 0.2f, 0.2f, 1.0f});
    }
    return dialog;
}

UIDialog* dialog_prompt(const char* title, const char* message, const char* default_text) {
    UIDialog* dialog = dialog_create("Prompt", title, message, DIALOG_TYPE_PROMPT);
    if (dialog) {
        // TODO: Add text input widget to content container
        if (default_text) {
            dialog->prompt_result = strdup(default_text);
        } else {
            dialog->prompt_result = strdup("");
        }
    }
    return dialog;
}

UIDialog* dialog_custom(const char* title) {
    return dialog_create("Custom", title, NULL, DIALOG_TYPE_CUSTOM);
}

/* ============================================================================
 * PUBLIC API - Builder Pattern
 * ============================================================================ */

typedef struct DialogBuilder {
    const char* title;
    const char* message;
    DialogType type;
    uint32_t buttons;
    bool modal;
    Vec2 size;
    bool draggable;
    bool resizable;
} DialogBuilder;

DialogBuilder* dialog_builder_create(void) {
    DialogBuilder* builder = memory_alloc(sizeof(DialogBuilder));
    if (!builder) return NULL;

    builder->title = "";
    builder->message = "";
    builder->type = DIALOG_TYPE_CUSTOM;
    builder->buttons = DIALOG_BUTTON_OK;
    builder->modal = true;
    builder->size = (Vec2){400.0f, 300.0f};
    builder->draggable = true;
    builder->resizable = false;

    return builder;
}

void dialog_builder_destroy(DialogBuilder* builder) {
    if (builder) free(builder);
}

DialogBuilder* dialog_builder_title(DialogBuilder* builder, const char* title) {
    if (builder) builder->title = title ? title : "";
    return builder;
}

DialogBuilder* dialog_builder_message(DialogBuilder* builder, const char* message) {
    if (builder) builder->message = message ? message : "";
    return builder;
}

DialogBuilder* dialog_builder_type(DialogBuilder* builder, DialogType type) {
    if (builder) builder->type = type;
    return builder;
}

DialogBuilder* dialog_builder_buttons(DialogBuilder* builder, uint32_t flags) {
    if (builder) builder->buttons = flags;
    return builder;
}

DialogBuilder* dialog_builder_modal(DialogBuilder* builder, bool modal) {
    if (builder) builder->modal = modal;
    return builder;
}

DialogBuilder* dialog_builder_size(DialogBuilder* builder, Vec2 size) {
    if (builder) builder->size = size;
    return builder;
}

DialogBuilder* dialog_builder_draggable(DialogBuilder* builder, bool draggable) {
    if (builder) builder->draggable = draggable;
    return builder;
}

DialogBuilder* dialog_builder_resizable(DialogBuilder* builder, bool resizable) {
    if (builder) builder->resizable = resizable;
    return builder;
}

UIDialog* dialog_builder_build(DialogBuilder* builder) {
    if (!builder) return NULL;

    UIDialog* dialog = dialog_create("BuiltDialog", builder->title, builder->message, builder->type);
    if (!dialog) return NULL;

    dialog_set_modal(dialog, builder->modal);
    dialog_set_buttons(dialog, builder->buttons);
    dialog_set_draggable(dialog, builder->draggable);
    dialog_set_resizable(dialog, builder->resizable);
    widget_set_size(&dialog->base, builder->size);

    return dialog;
}
