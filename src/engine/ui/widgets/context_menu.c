/*
 * context_menu.c
 * Context Menu Implementation
 */

#include "context_menu.h"
#include "ui_button.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    UIContextMenu* menu;
    int id;
} MenuItemData;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static void ui_context_menu_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    // UIContextMenu* menu = (UIContextMenu*)widget;

    // TODO: Render menu background and border
    // Children (items) will render themselves
}

static void ui_context_menu_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    UIContextMenu* menu = (UIContextMenu*)widget;

    float current_y = 5.0f; // Padding
    float max_width = menu->width;

    // Layout items vertically
    for (uint32_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];

        widget_set_position(child, (Vec2){5.0f, current_y});
        widget_set_size(child, (Vec2){max_width - 10.0f, child->size.y});

        current_y += child->size.y + 2.0f; // Spacing
    }

    widget_set_size(widget, (Vec2){max_width, current_y + 5.0f});
    widget->needs_layout = false;
}

static Size ui_context_menu_measure_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return (Size){0, 0};
    UIContextMenu* menu = (UIContextMenu*)widget;

    // Calculate height based on children
    float total_height = 10.0f; // Padding
    for (uint32_t i = 0; i < widget->child_count; i++) {
        total_height += widget->children[i]->size.y + 2.0f;
    }

    return (Size){menu->width, total_height};
}

static void ui_context_menu_destroy_impl(Widget* widget) {
    if (!widget) return;

    // We need to free the MenuItemData attached to buttons
    for (uint32_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child->user_data) {
            free(child->user_data);
            child->user_data = NULL;
        }
    }
}

static void on_item_click(UIButton* btn, void* user_data) {
    if (!user_data) return;
    MenuItemData* data = (MenuItemData*)user_data;

    if (data->menu && data->menu->on_select) {
        data->menu->on_select(data->menu, data->id, data->menu->user_data);
    }

    if (data->menu) {
        ui_context_menu_hide(data->menu);
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

UIContextMenu* ui_context_menu_create(const char* name) {
    UIContextMenu* menu = memory_alloc(sizeof(UIContextMenu));
    if (!menu) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI context menu");
        return NULL;
    }

    Widget* base = widget_create(name ? name : "ContextMenu");
    if (!base) {
        free(menu);
        return NULL;
    }

    memcpy(menu, base, sizeof(Widget));

    menu->active = false;
    menu->width = 150.0f;
    menu->item_height = 24.0f;

    menu->bg_color = (Vec4){0.15f, 0.15f, 0.15f, 1.0f};
    menu->border_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
    menu->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};

    menu->base.render = ui_context_menu_render_impl;
    menu->base.layout = ui_context_menu_layout_impl;
    menu->base.measure = ui_context_menu_measure_impl;
    menu->base.destroy = ui_context_menu_destroy_impl;

    menu->base.visible = false;
    menu->base.z_index = 2000; // Top layer

    return menu;
}

void ui_context_menu_destroy(UIContextMenu* menu) {
    if (!menu) return;

    if (menu->base.destroy) {
        menu->base.destroy(&menu->base);
    }

    if (menu->base.children) {
        free(menu->base.children);
    }
    if (menu->base.name) {
        free(menu->base.name);
    }

    free(menu);
}

void ui_context_menu_add_item(UIContextMenu* menu, int id, const char* label) {
    if (!menu) return;

    UIButton* btn = ui_button_create(label, label);
    if (!btn) return;

    ui_button_set_style(btn, BUTTON_STYLE_FLAT);
    widget_set_size(&btn->base, (Vec2){menu->width - 10.0f, menu->item_height});
    ui_button_set_text_color(btn, menu->text_color);

    // Store ID and menu ref
    MenuItemData* data = memory_alloc(sizeof(MenuItemData));
    data->menu = menu;
    data->id = id;

    // We attach this data to the button widget's user_data
    // Note: This overrides whatever ui_button_set_on_click usually uses for user_data
    // So we must ensure on_item_click expects it.
    ui_button_set_on_click(btn, on_item_click, data);

    // Also store it in base widget user_data so we can free it later
    btn->base.user_data = data;

    widget_add_child(&menu->base, &btn->base);
}

void ui_context_menu_add_separator(UIContextMenu* menu) {
    if (!menu) return;

    Widget* sep = widget_create("Separator");
    widget_set_size(sep, (Vec2){menu->width - 10.0f, 2.0f});
    sep->background_color = menu->border_color;

    widget_add_child(&menu->base, sep);
}

void ui_context_menu_set_callback(UIContextMenu* menu, ContextMenuCallback callback, void* user_data) {
    if (!menu) return;
    menu->on_select = callback;
    menu->user_data = user_data;
}

void ui_context_menu_show(UIContextMenu* menu, Vec2 position) {
    if (!menu) return;

    widget_set_position(&menu->base, position);
    widget_set_visible(&menu->base, true);
    menu->active = true;

    // Bring to front
    // In a real system, we might move it to a specific overlay layer
    // For now we assume z_index handles drawing order

    LOG_INFO(LOG_CAT_GENERAL, "Show context menu at (%.1f, %.1f)", position.x, position.y);
}

void ui_context_menu_hide(UIContextMenu* menu) {
    if (!menu) return;

    widget_set_visible(&menu->base, false);
    menu->active = false;
}

bool ui_context_menu_is_visible(const UIContextMenu* menu) {
    if (!menu) return false;
    return menu->base.visible;
}
