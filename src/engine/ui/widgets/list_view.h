#ifndef UI_LIST_VIEW_H
#define UI_LIST_VIEW_H

#include "../layout/flexbox_layout.h" // For BoxEdges and Size
#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct ListViewAdapter ListViewAdapter;

// Adapter interface for providing data to the list view
struct ListViewAdapter {
    // Get the total number of items
    uint32_t (*get_count)(void* adapter_data);

    // Create a new widget for an item
    // The widget should be created but not necessarily populated with data yet
    Widget* (*create_item_widget)(void* adapter_data);

    // Update an item widget with data for the specific index
    void (*update_item_widget)(void* adapter_data, uint32_t index, Widget* widget);

    // Get the height of an item (for vertical lists)
    float (*get_item_height)(void* adapter_data, uint32_t index);

    // Callback when an item is selected
    void (*on_item_selected)(void* adapter_data, uint32_t index);

    // User data passed to callbacks
    void* data;
};

// Create a new ListView widget
Widget* widget_create_list_view(const char* name);

// Set the adapter for the list view
void list_view_set_adapter(Widget* list_view, ListViewAdapter* adapter);

// Refresh the list view (reload data from adapter)
void list_view_refresh(Widget* list_view);

// Set the currently selected item index (-1 for none)
void list_view_set_selection(Widget* list_view, int32_t index);

// Get the currently selected item index (-1 for none)
int32_t list_view_get_selection(const Widget* list_view);

// Scroll to a specific item index
void list_view_scroll_to_index(Widget* list_view, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // UI_LIST_VIEW_H
