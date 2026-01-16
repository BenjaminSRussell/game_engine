#ifndef ULTIMATE_ENGINE_ASSET_H
#define ULTIMATE_ENGINE_ASSET_H

#include <stdbool.h>

typedef struct AssetManager AssetManager;

bool Asset_Init(void);
void Asset_Shutdown(void);

// Stub functions
void *Asset_Load(const char *path);
void Asset_Unload(void *asset);

#endif // ULTIMATE_ENGINE_ASSET_H
