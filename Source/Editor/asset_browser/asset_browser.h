#ifndef ASSET_BROWSER_H
#define ASSET_BROWSER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ASSET_TYPE_UNKNOWN,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MODEL,
    ASSET_TYPE_AUDIO,
    ASSET_TYPE_SCRIPT,
    ASSET_TYPE_LEVEL,
    ASSET_TYPE_PREFAB
} AssetType;

typedef struct {
    char name[64];
    char path[256];
    AssetType type;
    size_t size_bytes;
} AssetInfo;

void AssetBrowser_Init(void);
void AssetBrowser_Shutdown(void);

// Scan directory for assets
void AssetBrowser_ScanDirectory(const char* path);

// Get number of found assets
size_t AssetBrowser_GetAssetCount(void);

// Get asset info at index
bool AssetBrowser_GetAsset(size_t index, AssetInfo* out_info);

// Filter assets by type
void AssetBrowser_SetFilter(AssetType type);

#endif // ASSET_BROWSER_H
