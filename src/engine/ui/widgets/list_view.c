#include "list_view.h"
#include "core/memory.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct {
    ListViewAdapter adapter;
    int32_t selected_index;
    float scroll_offset;

    // Recycling
    Widget** recycled_widgets;
    uint32_t recycled_count;
    uint32_t recycled_capacity;

    // State
    int32_t first_visible_index;

    // Cache
    float total_content_height;
} ListViewData;

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================ */

static void recycle_widget(ListViewData* data, Widget* widget) {
    if (!widget) return;

    // Remove from parent if still attached (should be done by caller usually, but safe to check)
    // Actually caller should use widget_remove_child

    // Add to recycled pool
    if (data->recycled_count >= data->recycled_capacity) {
        uint32_t new_capacity = data->recycled_capacity == 0 ? 8 : data->recycled_capacity * 2;
        Widget** new_pool = memory_realloc(data->recycled_widgets, new_capacity * sizeof(Widget*));
        if (!new_pool) {
            LOG_ERROR(LOG_CAT_UI, "Failed to resize recycled widgets pool");
            widget_destroy(widget); // Can't recycle, so destroy
            return;
        }
        data->recycled_widgets = new_pool;
        data->recycled_capacity = new_capacity;
    }

    data->recycled_widgets[data->recycled_count++] = widget;

    // Hide recycled widget just in case
    widget_set_visible(widget, false);
}

static bool on_item_click(Widget* item, UIEvent* event, void* user_data) {
    // Only handle primary button clicks
    if (event->type == UI_EVENT_MOUSE_DOWN && event->mouse.button != UI_MOUSE_BUTTON_LEFT) {
        return false;
    }

    Widget* list_view = (Widget*)user_data;
    if (!list_view || !list_view->user_data) return false;

    ListViewData* data = (ListViewData*)list_view->user_data;

    // Find item in children to determine index
    for (uint32_t i = 0; i < list_view->child_count; i++) {
        if (list_view->children[i] == item) {
            int32_t index = data->first_visible_index + (int32_t)i;
            list_view_set_selection(list_view, index);

            // Notify adapter
            if (data->adapter.on_item_selected) {
                data->adapter.on_item_selected(data->adapter.data, index);
            }
            return true;
        }
    }
    return false;
}

static Widget* get_item_widget(Widget* list_view, ListViewData* data, uint32_t index) {
    Widget* widget = NULL;

    // Try to get from recycled pool
    if (data->recycled_count > 0) {
        widget = data->recycled_widgets[--data->recycled_count];
        widget_set_visible(widget, true);
    } else {
        // Create new
        if (data->adapter.create_item_widget) {
            widget = data->adapter.create_item_widget(data->adapter.data);
            if (widget) {
                // Add event handler for selection
                widget_add_event_handler(widget, UI_EVENT_MOUSE_DOWN, on_item_click, list_view);
                // Also handle touch
                widget_add_event_handler(widget, UI_EVENT_TOUCH_DOWN, on_item_click, list_view);
            }
        }
    }

    if (widget && data->adapter.update_item_widget) {
        data->adapter.update_item_widget(data->adapter.data, index, widget);
    }

    return widget;
}

static float calculate_total_height(ListViewData* data) {
    if (!data->adapter.get_count) return 0.0f;

    uint32_t count = data->adapter.get_count(data->adapter.data);
    if (count == 0) return 0.0f;

    float total = 0.0f;
    if (data->adapter.get_item_height) {
        for (uint32_t i = 0; i < count; i++) {
            total += data->adapter.get_item_height(data->adapter.data, i);
        }
    } else {
        // Assume default height if callback not provided? Or 0?
        // Let's assume 30.0f as default fallback
        total = count * 30.0f;
    }
    return total;
}

/* ============================================================================
 * WIDGET OVERRIDES
 * ============================================================================ */

static void list_view_destroy(Widget* widget) {
    if (!widget) return;

    ListViewData* data = (ListViewData*)widget->user_data;
    if (data) {
        // Destroy recycled widgets
        for (uint32_t i = 0; i < data->recycled_count; i++) {
            widget_destroy(data->recycled_widgets[i]);
        }
        if (data->recycled_widgets) {
            memory_free(data->recycled_widgets);
        }

        memory_free(data);
        widget->user_data = NULL;
    }

    // Base destroy (handles children) is called by caller of this function?
    // No, widget.c widget_destroy calls widget->destroy FIRST, then destroys children.
    // So we are good.
}

static void list_view_layout(Widget* widget, float available_width, float available_height) {
    if (!widget || !widget->user_data) return;

    ListViewData* data = (ListViewData*)widget->user_data;

    if (!data->adapter.get_count) return;

    uint32_t count = data->adapter.get_count(data->adapter.data);
    data->total_content_height = calculate_total_height(data);

    // Clamp scroll offset
    float max_scroll = fmaxf(0.0f, data->total_content_height - widget->size.y);
    data->scroll_offset = fmaxf(0.0f, fminf(data->scroll_offset, max_scroll));

    float current_y = 0.0f;
    float start_y = data->scroll_offset;
    float end_y = start_y + widget->size.y;

    // Find visible range
    int32_t new_first_index = -1;
    int32_t new_last_index = -1;

    float y_accum = 0.0f;

    // TODO: This loop iterates all items. For huge lists, use fixed height optimization or caching.
    for (uint32_t i = 0; i < count; i++) {
        float h = 30.0f;
        if (data->adapter.get_item_height) {
            h = data->adapter.get_item_height(data->adapter.data, i);
        }

        if (y_accum + h > start_y && y_accum < end_y) {
            if (new_first_index == -1) new_first_index = (int32_t)i;
            new_last_index = (int32_t)i;
        }

        if (y_accum >= end_y) break;

        y_accum += h;
    }

    if (new_first_index == -1) {
        // No items visible
        // Recycle all existing children
        while (widget->child_count > 0) {
            Widget* child = widget->children[widget->child_count - 1];
            widget_remove_child(widget, child);
            recycle_widget(data, child);
        }
        data->first_visible_index = 0;
        return;
    }

    // Reconcile children
    // Current range: [data->first_visible_index, data->first_visible_index + widget->child_count - 1]
    // New range: [new_first_index, new_last_index]

    // 1. Remove from top
    while (widget->child_count > 0 && data->first_visible_index < new_first_index) {
        Widget* child = widget->children[0];
        widget_remove_child(widget, child); // This shifts array
        recycle_widget(data, child);
        data->first_visible_index++;
    }

    // 2. Remove from bottom
    int32_t current_last_index = data->first_visible_index + (int32_t)widget->child_count - 1;
    while (widget->child_count > 0 && current_last_index > new_last_index) {
        Widget* child = widget->children[widget->child_count - 1];
        widget_remove_child(widget, child);
        recycle_widget(data, child);
        current_last_index--;
    }

    // If we cleared everything or started fresh
    if (widget->child_count == 0) {
        data->first_visible_index = new_first_index;
    }

    // 3. Add to top
    while (data->first_visible_index > new_first_index) {
        data->first_visible_index--;
        Widget* child = get_item_widget(widget, data, (uint32_t)data->first_visible_index);
        if (child) {
            // Add child at index 0 without removing others?
            // widget_add_child appends. We need prepend or manual array manipulation.
            // widget.c doesn't support insert_child_at.
            // So we must rely on append, meaning we can't easily prepend.
            // WORKAROUND: If we need to prepend, we might have to rebuild the list or accept that widget_add_child appends.
            // If we append, the order in 'children' array will be wrong (new top item is at end).
            // But for layout, we position them manually. So visual order is fine if Z-order doesn't matter (usually items don't overlap).
            // But tab order/hit testing relies on child order.

            // To fix this correctly, we would implement widget_insert_child.
            // Or, strictly for this implementation:
            // Since we can't prepend easily, and we want to keep array sorted by index.
            // If we need to add to top, it implies we scrolled UP.
            // Since we can't prepend, we might have to remove all and re-add? That's expensive.
            // Or we just add them and sort the array?

            // Let's implement a simple sort of the children array by index?
            // But we don't store index in child.
            // So we rely on knowing that we are adding 'data->first_visible_index'.

            // Alternative: Just clear and rebuild if we scrolled up significantly?
            // Or, implement widget_insert_child_at locally? We can't access Widget internal fields cleanly (opaque struct? no, defined in header).
            // Widget struct IS defined in header. We CAN manipulate children array!

            // Implementation of prepend:
            widget_add_child(widget, child); // Appends
            // Now move it to front
            for (uint32_t k = widget->child_count - 1; k > 0; k--) {
                widget->children[k] = widget->children[k-1];
            }
            widget->children[0] = child;
        }
    }

    // 4. Add to bottom
    int32_t current_end = data->first_visible_index + (int32_t)widget->child_count - 1;
    while (current_end < new_last_index) {
        current_end++;
        Widget* child = get_item_widget(widget, data, (uint32_t)current_end);
        if (child) {
            widget_add_child(widget, child);
        }
    }

    // 5. Layout children
    // Need to calculate Y position for first visible item
    float first_item_y = 0.0f;
    // Iterate to find y of first_visible_index
    // TODO: Optimize
    for (uint32_t i = 0; i < (uint32_t)data->first_visible_index; i++) {
        float h = 30.0f;
        if (data->adapter.get_item_height) h = data->adapter.get_item_height(data->adapter.data, i);
        first_item_y += h;
    }

    float item_y = first_item_y - data->scroll_offset;

    for (uint32_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        uint32_t index = (uint32_t)(data->first_visible_index + (int32_t)i);
        float h = 30.0f;
        if (data->adapter.get_item_height) h = data->adapter.get_item_height(data->adapter.data, index);

        widget_set_position(child, (Vec2){0, item_y});
        widget_set_size(child, (Vec2){available_width, h});

        // Ensure child layout is updated
        if (child->layout) {
            child->layout(child, available_width, h);
        }

        item_y += h;
    }
}

static bool list_view_handle_event(Widget* widget, UIEvent* event) {
    if (!widget || !widget->user_data) return false;
    ListViewData* data = (ListViewData*)widget->user_data;

    if (event->type == UI_EVENT_SCROLL) {
        float delta = event->scroll.delta.y * 20.0f; // Scroll speed
        data->scroll_offset -= delta;

        // Layout will clamp
        widget_invalidate_layout(widget);
        return true;
    }

    return false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

Widget* widget_create_list_view(const char* name) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    ListViewData* data = memory_alloc(sizeof(ListViewData));
    if (!data) {
        widget_destroy(widget);
        return NULL;
    }

    memset(data, 0, sizeof(ListViewData));
    data->selected_index = -1;
    data->first_visible_index = 0;

    widget->user_data = data;
    widget->destroy = list_view_destroy;
    widget->layout = list_view_layout;
    widget->handle_event = list_view_handle_event;

    return widget;
}

void list_view_set_adapter(Widget* list_view, ListViewAdapter* adapter) {
    if (!list_view || !list_view->user_data || !adapter) return;
    ListViewData* data = (ListViewData*)list_view->user_data;

    data->adapter = *adapter;
    list_view_refresh(list_view);
}

void list_view_refresh(Widget* list_view) {
    if (!list_view || !list_view->user_data) return;
    ListViewData* data = (ListViewData*)list_view->user_data;

    // Remove all children and recycle them
    while (list_view->child_count > 0) {
        Widget* child = list_view->children[list_view->child_count - 1];
        widget_remove_child(list_view, child);
        recycle_widget(data, child);
    }

    data->first_visible_index = 0;
    data->scroll_offset = 0.0f;
    data->selected_index = -1;

    widget_invalidate_layout(list_view);
}

void list_view_set_selection(Widget* list_view, int32_t index) {
    if (!list_view || !list_view->user_data) return;
    ListViewData* data = (ListViewData*)list_view->user_data;

    data->selected_index = index;
    // We could visually update the selected item here if we had a way to signal it
    // For now, the adapter callback handles logic.
    // If visual state needs update, the user can call list_view_refresh or update specific row.
    // Or we can iterate children and re-update them.

    // Update visible widgets to reflect selection state (e.g. highlight)
    for (uint32_t i = 0; i < list_view->child_count; i++) {
        uint32_t child_idx = (uint32_t)(data->first_visible_index + (int32_t)i);
        if (data->adapter.update_item_widget) {
            data->adapter.update_item_widget(data->adapter.data, child_idx, list_view->children[i]);
        }
    }
}

int32_t list_view_get_selection(const Widget* list_view) {
    if (!list_view || !list_view->user_data) return -1;
    ListViewData* data = (ListViewData*)list_view->user_data;
    return data->selected_index;
}

void list_view_scroll_to_index(Widget* list_view, uint32_t index) {
    if (!list_view || !list_view->user_data) return;
    ListViewData* data = (ListViewData*)list_view->user_data;

    // Calculate position
    float y = 0.0f;
    for (uint32_t i = 0; i < index; i++) {
        float h = 30.0f;
        if (data->adapter.get_item_height) h = data->adapter.get_item_height(data->adapter.data, i);
        y += h;
    }

    data->scroll_offset = y;
    widget_invalidate_layout(list_view);
}
