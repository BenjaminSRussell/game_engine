#include "combobox.h"
#include "core/memory.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static ComboBoxData* get_data(const Widget* widget) {
    if (!widget || !widget->user_data) return NULL;
    // In a real system, we'd verify type safety here
    return (ComboBoxData*)widget->user_data;
}

static void combobox_destroy_internal(Widget* widget) {
    ComboBoxData* data = get_data(widget);
    if (!data) return;

    // Free items
    if (data->items) {
        for (uint32_t i = 0; i < data->item_count; i++) {
            if (data->items[i]) {
                free(data->items[i]);
            }
        }
        memory_free(data->items);
    }

    // Free data structure
    memory_free(data);
    widget->user_data = NULL;
}

static void on_item_clicked(Widget* item_widget, UIEvent* event, void* user_data) {
    if (event->type != UI_EVENT_MOUSE_CLICK) return;

    Widget* list = item_widget->parent;
    if (!list) return;

    Widget* combo = list->parent;
    if (!combo) return;

    // Find index of clicked item
    // The user_data of the item widget stores the index
    intptr_t index = (intptr_t)user_data;

    combobox_select_index(combo, (int)index);
    combobox_toggle(combo); // Close

    event->handled = true;
    event->stop_propagation = true;
}

static bool on_item_event_proxy(Widget* widget, UIEvent* event, void* user_data) {
    if (event->type == UI_EVENT_MOUSE_CLICK) {
        on_item_clicked(widget, event, user_data);
        return true;
    }
    return false;
}

static void refresh_list_ui(Widget* combobox) {
    ComboBoxData* data = get_data(combobox);
    if (!data || !data->list_container) return;

    // Clear existing items in list container
    // We iterate backwards to safely remove
    while (data->list_container->child_count > 0) {
        Widget* child = data->list_container->children[data->list_container->child_count - 1];
        widget_remove_child(data->list_container, child);
        widget_destroy(child);
    }

    // Rebuild items
    float y_offset = 0.0f;
    float item_height = 25.0f; // Fixed height for now

    for (uint32_t i = 0; i < data->item_count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "item_%u", i);

        Widget* item = widget_create(name);
        widget_set_size(item, (Vec2){data->list_container->size.x, item_height});
        widget_set_position(item, (Vec2){0.0f, y_offset});

        // Item styling
        item->background_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
        item->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
        item->hoverable = true;

        widget_set_text(item, data->items[i]);

        widget_add_event_handler(item, UI_EVENT_MOUSE_CLICK, on_item_event_proxy, (void*)(intptr_t)i);

        widget_add_child(data->list_container, item);
        y_offset += item_height;
    }

    // Adjust list container size
    float list_height = y_offset;
    if (data->max_dropdown_height > 0 && list_height > data->max_dropdown_height) {
        list_height = data->max_dropdown_height;
        // Should enable scrolling here
    }
    widget_set_size(data->list_container, (Vec2){data->list_container->size.x, list_height});
}

static bool on_combobox_click(Widget* widget, UIEvent* event, void* user_data) {
    if (event->type == UI_EVENT_MOUSE_CLICK) {
        combobox_toggle(widget);
        return true;
    }
    return false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

Widget* combobox_create(const char* name) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    ComboBoxData* data = memory_alloc(sizeof(ComboBoxData));
    if (!data) {
        widget_destroy(widget);
        return NULL;
    }

    memset(data, 0, sizeof(ComboBoxData));
    data->max_dropdown_height = 200.0f;
    data->selected_index = -1;
    data->item_capacity = 8;
    data->items = memory_alloc(sizeof(char*) * data->item_capacity);

    widget->user_data = data;
    widget->destroy = combobox_destroy_internal;

    // Set default size
    widget_set_size(widget, (Vec2){150.0f, 30.0f});
    widget->background_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    widget->border_color = (Vec4){0.5f, 0.5f, 0.5f, 1.0f};
    widget->border_width = 1.0f;

    // Create Label (for selected text)
    data->label_widget = widget_create("combo_label");
    widget_set_position(data->label_widget, (Vec2){5.0f, 5.0f});
    widget_set_size(data->label_widget, (Vec2){120.0f, 20.0f});
    widget_set_text(data->label_widget, "Select...");
    widget_add_child(widget, data->label_widget);

    // Create Button (arrow)
    data->button_widget = widget_create("combo_arrow");
    widget_set_position(data->button_widget, (Vec2){130.0f, 5.0f});
    widget_set_size(data->button_widget, (Vec2){20.0f, 20.0f});
    data->button_widget->background_color = (Vec4){0.4f, 0.4f, 0.4f, 1.0f};
    widget_add_child(widget, data->button_widget);

    // Create List Container (Hidden)
    data->list_container = widget_create("combo_list");
    widget_set_visible(data->list_container, false);
    widget_set_position(data->list_container, (Vec2){0.0f, 30.0f}); // Below the main widget
    widget_set_size(data->list_container, (Vec2){150.0f, 0.0f});
    data->list_container->background_color = (Vec4){0.25f, 0.25f, 0.25f, 1.0f};
    data->list_container->z_index = 100; // On top
    widget_add_child(widget, data->list_container);

    // Add event handler to toggle
    widget_add_event_handler(widget, UI_EVENT_MOUSE_CLICK, on_combobox_click, NULL);
    widget_add_event_handler(data->button_widget, UI_EVENT_MOUSE_CLICK, on_combobox_click, NULL);

    return widget;
}

void combobox_add_item(Widget* combobox, const char* text) {
    ComboBoxData* data = get_data(combobox);
    if (!data || !text) return;

    if (data->item_count >= data->item_capacity) {
        uint32_t new_capacity = data->item_capacity * 2;
        char** new_items = memory_realloc(data->items, sizeof(char*) * new_capacity);
        if (!new_items) return;
        data->items = new_items;
        data->item_capacity = new_capacity;
    }

    data->items[data->item_count] = strdup(text);
    data->item_count++;

    // If this is the first item, select it by default?
    if (data->selected_index == -1) {
        combobox_select_index(combobox, 0);
    }

    // If open, refresh list
    if (data->is_open) {
        refresh_list_ui(combobox);
    }
}

void combobox_remove_item(Widget* combobox, int index) {
    ComboBoxData* data = get_data(combobox);
    if (!data || index < 0 || index >= (int)data->item_count) return;

    free(data->items[index]);

    for (int i = index; i < (int)data->item_count - 1; i++) {
        data->items[i] = data->items[i + 1];
    }

    data->item_count--;

    // Handle selection change
    if (data->selected_index == index) {
        data->selected_index = -1;
        // Maybe select 0?
        if (data->item_count > 0) {
            combobox_select_index(combobox, 0);
        }
    } else if (data->selected_index > index) {
        data->selected_index--;
    }

    if (data->is_open) {
        refresh_list_ui(combobox);
    }
}

void combobox_clear_items(Widget* combobox) {
    ComboBoxData* data = get_data(combobox);
    if (!data) return;

    for (uint32_t i = 0; i < data->item_count; i++) {
        free(data->items[i]);
    }
    data->item_count = 0;
    data->selected_index = -1;

    if (data->is_open) {
        refresh_list_ui(combobox);
    }
}

void combobox_select_index(Widget* combobox, int index) {
    ComboBoxData* data = get_data(combobox);
    if (!data) return;

    if (index >= 0 && index < (int)data->item_count) {
        data->selected_index = index;

        widget_set_text(data->label_widget, data->items[index]);

        // Trigger callback
        if (data->on_select) {
            data->on_select(combobox, index, data->items[index], data->callback_user_data);
        }
    } else {
        data->selected_index = -1;
    }
}

int combobox_get_selected_index(const Widget* combobox) {
    ComboBoxData* data = get_data(combobox);
    return data ? data->selected_index : -1;
}

const char* combobox_get_selected_text(const Widget* combobox) {
    ComboBoxData* data = get_data(combobox);
    if (!data || data->selected_index < 0 || data->selected_index >= (int)data->item_count) {
        return NULL;
    }
    return data->items[data->selected_index];
}

void combobox_set_on_select(Widget* combobox, ComboBoxSelectCallback callback, void* user_data) {
    ComboBoxData* data = get_data(combobox);
    if (data) {
        data->on_select = callback;
        data->callback_user_data = user_data;
    }
}

void combobox_set_max_height(Widget* combobox, float height) {
    ComboBoxData* data = get_data(combobox);
    if (data) {
        data->max_dropdown_height = height;
    }
}

void combobox_toggle(Widget* combobox) {
    ComboBoxData* data = get_data(combobox);
    if (!data) return;

    data->is_open = !data->is_open;
    widget_set_visible(data->list_container, data->is_open);

    if (data->is_open) {
        refresh_list_ui(combobox);

        // Bring to front
        // In a real system we would move it to the end of parent's children array or use a layer system
    }
}
