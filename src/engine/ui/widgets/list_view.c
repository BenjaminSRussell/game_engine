#include "list_view.h"
#include "scroll_view.h"
#include "core/memory.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    Widget* scroll_view;
    Widget* content_container;
    SelectionMode selection_mode;
    int selected_index;
    ListViewSelectCallback on_select;
    void* callback_user_data;
} ListViewData;

static void list_view_destroy(Widget* widget);
static void list_view_layout(Widget* widget, float available_width, float available_height);
static void content_container_layout(Widget* widget, float available_width, float available_height);
static bool on_item_click(Widget* widget, UIEvent* event, void* user_data);

Widget* list_view_create(const char* name) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    ListViewData* data = memory_alloc(sizeof(ListViewData));
    if (!data) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate ListView data");
        widget_destroy(widget);
        return NULL;
    }

    data->selection_mode = SELECTION_MODE_SINGLE;
    data->selected_index = -1;
    data->on_select = NULL;
    data->callback_user_data = NULL;

    // Create ScrollView
    data->scroll_view = scroll_view_create("list_view_scroll", (Vec2){100, 100});
    if (!data->scroll_view) {
        memory_free(data);
        widget_destroy(widget);
        return NULL;
    }

    // Create Content Container
    data->content_container = widget_create("list_view_content");
    if (!data->content_container) {
        widget_destroy(data->scroll_view); // This might double free if scroll_view_destroy frees it? No, it's ok.
        memory_free(data);
        widget_destroy(widget); // widget has no children yet
        return NULL;
    }

    // Set layout for content container
    data->content_container->layout = content_container_layout;

    scroll_view_set_content(data->scroll_view, data->content_container);
    widget_add_child(widget, data->scroll_view);

    widget->user_data = data;
    widget->destroy = list_view_destroy;
    widget->layout = list_view_layout;

    // Default size
    widget_set_size(widget, (Vec2){200.0f, 300.0f});

    return widget;
}

static void list_view_destroy(Widget* widget) {
    if (widget && widget->user_data) {
        // children are destroyed by base widget_destroy
        memory_free(widget->user_data);
        widget->user_data = NULL;
    }
}

static void list_view_layout(Widget* widget, float available_width, float available_height) {
    ListViewData* data = (ListViewData*)widget->user_data;
    if (!data || !data->scroll_view) return;

    // Resize scroll view to match list view
    Vec2 size = widget_get_size(widget);
    widget_set_size(data->scroll_view, size);

    // Trigger scroll view layout
    if (data->scroll_view->layout) {
        data->scroll_view->layout(data->scroll_view, size.x, size.y);
    }
}

static void content_container_layout(Widget* widget, float available_width, float available_height) {
    // Simple vertical stack layout
    float y_offset = 0.0f;
    float max_width = 0.0f;

    for (uint32_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (!widget_is_visible(child)) continue;

        // Set child position
        widget_set_position(child, (Vec2){0.0f, y_offset});

        // Use child's size if set, or width of container
        Vec2 child_size = widget_get_size(child);
        if (available_width > 0) {
            child_size.x = available_width;
            widget_set_size(child, child_size);
        }

        y_offset += child_size.y;
        if (child_size.x > max_width) max_width = child_size.x;

        // Recurse layout
        if (child->layout) {
            child->layout(child, child_size.x, child_size.y);
        }
    }

    // Set container size
    widget_set_size(widget, (Vec2){max_width, y_offset});
}

static bool on_item_click(Widget* item, UIEvent* event, void* user_data) {
    if (event->type != UI_EVENT_MOUSE_CLICK) return false;

    Widget* list_view = (Widget*)user_data;
    if (!list_view) return false;

    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return false;

    if (data->selection_mode == SELECTION_MODE_NONE) return false;

    // Find index of item
    int index = -1;
    for (uint32_t i = 0; i < data->content_container->child_count; i++) {
        if (data->content_container->children[i] == item) {
            index = (int)i;
            break;
        }
    }

    if (index != -1) {
        list_view_set_selected_index(list_view, index);
        event->handled = true;
        return true;
    }

    return false;
}

void list_view_add_item(Widget* list_view, Widget* item) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data || !item) return;

    widget_add_child(data->content_container, item);

    // Add click handler for selection
    widget_add_event_handler(item, UI_EVENT_MOUSE_CLICK, on_item_click, list_view);

    widget_invalidate_layout(list_view);
}

void list_view_remove_item(Widget* list_view, int index) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return;

    if (index >= 0 && index < (int)data->content_container->child_count) {
        Widget* item = data->content_container->children[index];
        widget_remove_child(data->content_container, item);

        // Base widget_remove_child does not destroy, we should destroy it?
        // Usually list owns the items.
        // But removing usually implies taking it out.
        // The API says "remove_item", generally in UI libs this might just remove from view.
        // But for memory management, if we created it, who destroys it?
        // Let's assume caller handles destruction if they want, or we destroy it?
        // "list_view_clear" usually destroys.
        // Let's destroy it to avoid leaks for now as items are usually created for the list.
        widget_destroy(item);

        // Update selection
        if (data->selected_index == index) {
            data->selected_index = -1; // Deselect
        } else if (data->selected_index > index) {
            data->selected_index--;
        }

        widget_invalidate_layout(list_view);
    }
}

void list_view_clear(Widget* list_view) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return;

    while (data->content_container->child_count > 0) {
        Widget* item = data->content_container->children[data->content_container->child_count - 1];
        widget_remove_child(data->content_container, item);
        widget_destroy(item);
    }

    data->selected_index = -1;
    widget_invalidate_layout(list_view);
}

void list_view_set_selection_mode(Widget* list_view, SelectionMode mode) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (data) {
        data->selection_mode = mode;
        if (mode == SELECTION_MODE_NONE) {
            data->selected_index = -1;
            // TODO: Visually update items to deselect
        }
    }
}

int list_view_get_selected_index(const Widget* list_view) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    return data ? data->selected_index : -1;
}

void list_view_set_selected_index(Widget* list_view, int index) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return;

    if (data->selection_mode == SELECTION_MODE_NONE) return;

    // Deselect previous
    if (data->selected_index != -1 && data->selected_index < (int)data->content_container->child_count) {
        Widget* prev_item = data->content_container->children[data->selected_index];
        widget_set_state(prev_item, WIDGET_STATE_NORMAL); // Reset state
    }

    if (index >= 0 && index < (int)data->content_container->child_count) {
        data->selected_index = index;

        // Select new
        Widget* new_item = data->content_container->children[index];
        widget_set_state(new_item, WIDGET_STATE_SELECTED);

        // Callback
        if (data->on_select) {
            data->on_select(list_view, index, data->callback_user_data);
        }
    } else {
        data->selected_index = -1;
    }
}

void list_view_set_on_select(Widget* list_view, ListViewSelectCallback callback, void* user_data) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (data) {
        data->on_select = callback;
        data->callback_user_data = user_data;
    }
}

Widget* list_view_get_item(const Widget* list_view, int index) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return NULL;

    if (index >= 0 && index < (int)data->content_container->child_count) {
        return data->content_container->children[index];
    }
    return NULL;
}

uint32_t list_view_get_item_count(const Widget* list_view) {
    ListViewData* data = (ListViewData*)list_view->user_data;
    if (!data) return 0;
    return data->content_container->child_count;
}
