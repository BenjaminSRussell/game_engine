#pragma once

#include <stdbool.h>

typedef void (*MenuCallback)(void *user_data);

typedef struct MenuItem {
    const char *label;
    const char *shortcut; // Display string like "Ctrl+D"
    MenuCallback callback;
    void *user_data;
    bool enabled;
    bool is_separator;
    
    struct MenuItem *submenu;
    int submenu_count;
} MenuItem;

typedef struct ContextMenu {
    MenuItem *items;
    int item_count;
    int capacity;
    
    bool is_open;
    int pos_x, pos_y;
    
    // Search filtering
    char search_buffer[64];
    bool show_search;
} ContextMenu;

void context_menu_init(ContextMenu *menu);
void context_menu_destroy(ContextMenu *menu);

// Building
void context_menu_add_item(ContextMenu *menu, const char *label, MenuCallback callback, void *user_data);
void context_menu_add_separator(ContextMenu *menu);
void context_menu_add_submenu(ContextMenu *menu, const char *label);

// Display
void context_menu_open(ContextMenu *menu, int x, int y);
void context_menu_close(ContextMenu *menu);
void context_menu_render(ContextMenu *menu);

// Filtering
void context_menu_set_filter(ContextMenu *menu, const char *filter);
