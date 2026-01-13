#include "asset_browser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock storage for assets
#define MAX_ASSETS 100
static AssetInfo g_Assets[MAX_ASSETS];
static size_t g_AssetCount = 0;
static AssetType g_FilterType = ASSET_TYPE_UNKNOWN;

void AssetBrowser_Init(void) {
    printf("[AssetBrowser] Initialized\n");
    g_AssetCount = 0;
}

void AssetBrowser_Shutdown(void) {
    printf("[AssetBrowser] Shutdown\n");
}

void AssetBrowser_ScanDirectory(const char* path) {
    printf("[AssetBrowser] Scanning directory: %s\n", path);
    // Mock implementation: Add some dummy assets based on path

    // Clear previous
    g_AssetCount = 0;

    // Add dummy texture
    if (g_AssetCount < MAX_ASSETS) {
        strcpy(g_Assets[g_AssetCount].name, "grass_texture.png");
        strcpy(g_Assets[g_AssetCount].path, "assets/textures/grass_texture.png");
        g_Assets[g_AssetCount].type = ASSET_TYPE_TEXTURE;
        g_Assets[g_AssetCount].size_bytes = 1024 * 512;
        g_AssetCount++;
    }

    // Add dummy model
    if (g_AssetCount < MAX_ASSETS) {
        strcpy(g_Assets[g_AssetCount].name, "hero_mesh.obj");
        strcpy(g_Assets[g_AssetCount].path, "assets/models/hero_mesh.obj");
        g_Assets[g_AssetCount].type = ASSET_TYPE_MODEL;
        g_Assets[g_AssetCount].size_bytes = 1024 * 1024 * 2;
        g_AssetCount++;
    }

    // Add dummy sound
    if (g_AssetCount < MAX_ASSETS) {
        strcpy(g_Assets[g_AssetCount].name, "jump.wav");
        strcpy(g_Assets[g_AssetCount].path, "assets/audio/jump.wav");
        g_Assets[g_AssetCount].type = ASSET_TYPE_AUDIO;
        g_Assets[g_AssetCount].size_bytes = 1024 * 50;
        g_AssetCount++;
    }

    printf("[AssetBrowser] Found %zu assets\n", g_AssetCount);
}

size_t AssetBrowser_GetAssetCount(void) {
    if (g_FilterType == ASSET_TYPE_UNKNOWN) {
        return g_AssetCount;
    }

    size_t filtered_count = 0;
    for (size_t i = 0; i < g_AssetCount; ++i) {
        if (g_Assets[i].type == g_FilterType) {
            filtered_count++;
        }
    }
    return filtered_count;
}

bool AssetBrowser_GetAsset(size_t index, AssetInfo* out_info) {
    if (!out_info) return false;

    if (g_FilterType == ASSET_TYPE_UNKNOWN) {
        if (index < g_AssetCount) {
            *out_info = g_Assets[index];
            return true;
        }
        return false;
    }

    // Handle filtered index
    size_t current_match = 0;
    for (size_t i = 0; i < g_AssetCount; ++i) {
        if (g_Assets[i].type == g_FilterType) {
            if (current_match == index) {
                *out_info = g_Assets[i];
                return true;
            }
            current_match++;
        }
    }

    return false;
}

void AssetBrowser_SetFilter(AssetType type) {
    g_FilterType = type;
    printf("[AssetBrowser] Filter set to %d\n", type);
}
