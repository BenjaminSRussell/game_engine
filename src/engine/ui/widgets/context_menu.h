/*
 * context_menu.h
 * Context Menu Implementation
 * Popup menu for actions
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_CONTEXT_MENU_H
#define UI_CONTEXT_MENU_H

#include "widget.h"
#include "include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIContextMenu UIContextMenu;

// Callback for menu item selection
typedef void (*ContextMenuCallback)(UIContextMenu* menu, int item_id, void* user_data);

// Context menu structure
struct UIContextMenu {
    Widget base;                    // Inherit from Widget

    // State
    bool active;

    // Callback
    ContextMenuCallback on_select;
    void* user_data;

    // Visuals
    float item_height;
    float width;
    Vec4 bg_color;
    Vec4 text_color;
    Vec4 hover_color;
    Vec4 border_color;
};

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new context menu
 *
 * @param name     Name identifier
 * @return         Pointer to created context menu, or NULL on failure
 */
UIContextMenu* ui_context_menu_create(const char* name);

/**
 * Destroy a context menu
 *
 * @param menu     Menu to destroy
 */
void ui_context_menu_destroy(UIContextMenu* menu);

/* ============================================================================
 * API - Items
 * ============================================================================ */

/**
 * Add an actionable item to the menu
 *
 * @param menu     Target menu
 * @param id       Item ID (passed to callback)
 * @param label    Item label text
 */
void ui_context_menu_add_item(UIContextMenu* menu, int id, const char* label);

/**
 * Add a separator line
 *
 * @param menu     Target menu
 */
void ui_context_menu_add_separator(UIContextMenu* menu);

/* ============================================================================
 * API - Interaction
 * ============================================================================ */

/**
 * Set selection callback
 *
 * @param menu      Target menu
 * @param callback  Function to call when item selected
 * @param user_data User data passed to callback
 */
void ui_context_menu_set_callback(UIContextMenu* menu, ContextMenuCallback callback, void* user_data);

/**
 * Show menu at position
 *
 * @param menu     Target menu
 * @param position Global screen position
 */
void ui_context_menu_show(UIContextMenu* menu, Vec2 position);

/**
 * Hide menu
 *
 * @param menu     Target menu
 */
void ui_context_menu_hide(UIContextMenu* menu);

/**
 * Check if menu is visible
 *
 * @param menu     Target menu
 * @return         True if visible
 */
bool ui_context_menu_is_visible(const UIContextMenu* menu);

#ifdef __cplusplus
}
#endif

#endif // UI_CONTEXT_MENU_H
