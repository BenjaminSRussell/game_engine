#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MESH,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_PREFAB,
    ASSET_TYPE_SCRIPT,
    ASSET_TYPE_SCENE
} AssetType;

typedef struct {
    uint64_t id;
    AssetType type;
    const char *name;
    const char *filepath;
    void *thumbnail_texture; // Pointer to GPU texture
    bool thumbnail_loaded;
} AssetEntry;

typedef struct {
    AssetEntry *assets;
    int asset_count;
    int capacity;
    
    // Filtering
    AssetType current_filter;
    char search_query[128];
    
    // Directory navigation
    const char *current_directory;
} AssetBrowserView;

void asset_browser_init(AssetBrowserView *view);
void asset_browser_destroy(AssetBrowserView *view);

// Asset management
void asset_browser_add_asset(AssetBrowserView *view, const char *filepath, AssetType type);
void asset_browser_remove_asset(AssetBrowserView *view, uint64_t asset_id);

// Filtering
void asset_browser_set_filter(AssetBrowserView *view, AssetType type);
void asset_browser_set_search(AssetBrowserView *view, const char *query);

// Thumbnail generation (async)
void asset_browser_generate_thumbnail(AssetEntry *asset);

// Rendering
void asset_browser_draw(AssetBrowserView *view);

// Drag and drop
AssetEntry* asset_browser_get_dragged_asset(AssetBrowserView *view);
