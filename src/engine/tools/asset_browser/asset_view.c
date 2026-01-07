/**
 * =================================================================================================
 *                          ASSET BROWSER VIEW
 * =================================================================================================
 */

#include "asset_view.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void asset_browser_init(AssetBrowserView *view) {
    view->assets = NULL;
    view->asset_count = 0;
    view->capacity = 0;
    view->current_filter = ASSET_TYPE_TEXTURE; // Show all initially (would use -1)
    view->search_query[0] = '\0';
    view->current_directory = ".";
}

void asset_browser_destroy(AssetBrowserView *view) {
    free(view->assets);
}

void asset_browser_add_asset(AssetBrowserView *view, const char *filepath, AssetType type) {
    if (view->asset_count >= view->capacity) {
        view->capacity = view->capacity == 0 ? 64 : view->capacity * 2;
        view->assets = realloc(view->assets, sizeof(AssetEntry) * view->capacity);
    }
    
    AssetEntry *asset = &view->assets[view->asset_count++];
    asset->id = (uint64_t)view->asset_count; // Simple ID
    asset->type = type;
    asset->filepath = filepath;
    asset->name = filepath; // Simplification
    asset->thumbnail_texture = NULL;
    asset->thumbnail_loaded = false;
}

void asset_browser_remove_asset(AssetBrowserView *view, uint64_t asset_id) {
    for (int i = 0; i < view->asset_count; i++) {
        if (view->assets[i].id == asset_id) {
            memmove(&view->assets[i], &view->assets[i+1], 
                    sizeof(AssetEntry) * (view->asset_count - i - 1));
            view->asset_count--;
            return;
        }
    }
}

void asset_browser_set_filter(AssetBrowserView *view, AssetType type) {
    view->current_filter = type;
}

void asset_browser_set_search(AssetBrowserView *view, const char *query) {
    strncpy(view->search_query, query, sizeof(view->search_query) - 1);
}

void asset_browser_generate_thumbnail(AssetEntry *asset) {
    // Async thumbnail generation would:
    // 1. Load asset
    // 2. Render to offscreen buffer
    // 3. Create GPU texture
    // 4. Set asset->thumbnail_texture
    // 5. Set asset->thumbnail_loaded = true
    
    asset->thumbnail_loaded = true; // Stub
}

static bool asset_matches_filter(AssetEntry *asset, AssetBrowserView *view) {
    // Type filter
    if (view->current_filter != asset->type) {
        // Would check if showing all types
    }
    
    // Search filter
    if (view->search_query[0] != '\0') {
        // Case-insensitive search
        const char *name = asset->name;
        const char *query = view->search_query;
        
        // Simple substring match
        if (strstr(name, query) == NULL) {
            return false;
        }
    }
    
    return true;
}

void asset_browser_draw(AssetBrowserView *view) {
    // Would integrate with UI system
    // ui_begin_panel("Asset Browser");
    
    // Toolbar: filter dropdown, search box
    // ui_text_input("Search", view->search_query, sizeof(view->search_query));
    
    // Directory tree navigation
    // ui_directory_tree(view->current_directory);
    
    // Asset grid
    int columns = 4;
    int col = 0;
    
    for (int i = 0; i < view->asset_count; i++) {
        AssetEntry *asset = &view->assets[i];
        
        if (!asset_matches_filter(asset, view)) {
            continue;
        }
        
        // ui_begin_asset_tile();
        
        if (!asset->thumbnail_loaded) {
            asset_browser_generate_thumbnail(asset);
        }
        
        // ui_image(asset->thumbnail_texture);
        // ui_text(asset->name);
        
       // Handle drag start
        // if (ui_is_item_active() && ui_is_mouse_dragging()) {
        //     // Start drag operation
        // }
        
        // Context menu
        // if (ui_begin_popup_context_item()) {
        //     if (ui_menu_item("Rename")) { }
        //     if (ui_menu_item("Move")) { }
        //     if (ui_menu_item("Delete")) { asset_browser_remove_asset(view, asset->id); }
        //     ui_end_popup();
        // }
        
        // ui_end_asset_tile();
        
        col++;
        if (col >= columns) {
            col = 0;
            // ui_new_line();
        }
    }
    
    // ui_end_panel();
}

AssetEntry* asset_browser_get_dragged_asset(AssetBrowserView *view) {
    // Would return currently dragging asset
    return NULL;
}
