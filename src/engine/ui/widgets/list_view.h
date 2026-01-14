#ifndef UI_LIST_VIEW_H
#define UI_LIST_VIEW_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SELECTION_MODE_NONE,
    SELECTION_MODE_SINGLE,
    SELECTION_MODE_MULTIPLE
} SelectionMode;

typedef void (*ListViewSelectCallback)(Widget* list_view, int index, void* user_data);

/**
 * Creates a new ListView widget.
 *
 * @param name The name of the widget.
 * @return A pointer to the created widget.
 */
Widget* list_view_create(const char* name);

/**
 * Adds an item to the list view.
 *
 * @param list_view The ListView widget.
 * @param item The widget to add as an item.
 */
void list_view_add_item(Widget* list_view, Widget* item);

/**
 * Removes an item from the list view by index.
 *
 * @param list_view The ListView widget.
 * @param index The index of the item to remove.
 */
void list_view_remove_item(Widget* list_view, int index);

/**
 * Removes all items from the list view.
 *
 * @param list_view The ListView widget.
 */
void list_view_clear(Widget* list_view);

/**
 * Sets the selection mode.
 *
 * @param list_view The ListView widget.
 * @param mode The selection mode.
 */
void list_view_set_selection_mode(Widget* list_view, SelectionMode mode);

/**
 * Gets the index of the currently selected item (for single selection).
 * Returns -1 if no selection.
 *
 * @param list_view The ListView widget.
 * @return The selected index.
 */
int list_view_get_selected_index(const Widget* list_view);

/**
 * Sets the selected item by index.
 *
 * @param list_view The ListView widget.
 * @param index The index to select.
 */
void list_view_set_selected_index(Widget* list_view, int index);

/**
 * Sets the callback for selection changes.
 *
 * @param list_view The ListView widget.
 * @param callback The function to call when selection changes.
 * @param user_data User data passed to the callback.
 */
void list_view_set_on_select(Widget* list_view, ListViewSelectCallback callback, void* user_data);

/**
 * Gets an item widget by index.
 *
 * @param list_view The ListView widget.
 * @param index The index of the item.
 * @return The item widget or NULL if invalid index.
 */
Widget* list_view_get_item(const Widget* list_view, int index);

/**
 * Gets the number of items in the list.
 *
 * @param list_view The ListView widget.
 * @return The item count.
 */
uint32_t list_view_get_item_count(const Widget* list_view);

#ifdef __cplusplus
}
#endif

#endif // UI_LIST_VIEW_H
