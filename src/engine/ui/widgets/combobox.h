#ifndef UI_COMBOBOX_H
#define UI_COMBOBOX_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ComboBox Widget
 *
 * A widget that allows selection from a dropdown list of items.
 * Implemented using the base Widget system.
 *
 * Usage:
 *   Widget* combo = combobox_create("my_combo");
 *   combobox_add_item(combo, "Option 1");
 *   combobox_add_item(combo, "Option 2");
 *   combobox_set_on_select(combo, my_callback, NULL);
 */

// Callback for selection change
// user_data is the data passed to combobox_set_on_select
typedef void (*ComboBoxSelectCallback)(Widget* combobox, int index, const char* text, void* user_data);

// Internal data structure for ComboBox
typedef struct ComboBoxData {
    char** items;
    uint32_t item_count;
    uint32_t item_capacity;
    int selected_index;
    bool is_open;
    float max_dropdown_height;

    // Child widgets references (owned by the widget hierarchy, but referenced here for access)
    Widget* label_widget;
    Widget* button_widget;
    Widget* list_container; // The dropdown list container

    // Callbacks
    ComboBoxSelectCallback on_select;
    void* callback_user_data;
} ComboBoxData;

// API

// Create a new ComboBox widget
Widget* combobox_create(const char* name);

// Add an item to the combobox
void combobox_add_item(Widget* combobox, const char* text);

// Remove an item at index
void combobox_remove_item(Widget* combobox, int index);

// Remove all items
void combobox_clear_items(Widget* combobox);

// Select an item by index
void combobox_select_index(Widget* combobox, int index);

// Get selected index (returns -1 if no selection)
int combobox_get_selected_index(const Widget* combobox);

// Get selected text (returns NULL if no selection)
const char* combobox_get_selected_text(const Widget* combobox);

// Set callback for selection change
void combobox_set_on_select(Widget* combobox, ComboBoxSelectCallback callback, void* user_data);

// Set maximum height for the dropdown list
void combobox_set_max_height(Widget* combobox, float height);

// Toggle dropdown visibility manually
void combobox_toggle(Widget* combobox);

#ifdef __cplusplus
}
#endif

#endif // UI_COMBOBOX_H
