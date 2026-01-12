#ifndef ASSET_BROWSER_H
#define ASSET_BROWSER_H

#include <core/types.h>

typedef struct {
    char name[256];
    char path[512];
    u32 asset_id;
    bool thumbnail_generated;
    bool needs_update;
    void *thumbnail_data;
    u32 thumbnail_size;
    u32 thumbnail_width;
} BrowserAsset;

typedef struct {
    BrowserAsset *assets;
    u32 asset_count;
    u32 max_assets;
    bool initialized;
} AssetBrowser;

void asset_browser_init(void);
void asset_browser_shutdown(void);
void asset_browser_add_asset(const char *name, const char *path, u32 asset_id);
void asset_browser_generate_thumbnail(BrowserAsset *asset);
void asset_browser_regenerate_thumbnail(BrowserAsset *asset);
void Editor_DrawAssetBrowser(void);

#endif // ASSET_BROWSER_H
