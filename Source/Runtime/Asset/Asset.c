#include "Public/Asset.h"
#include <stdlib.h>

static AssetManager *g_asset_manager = NULL;

bool Asset_Init(void) {
  g_asset_manager = (AssetManager *)malloc(1);
  return true;
}

void Asset_Shutdown(void) {
  if (g_asset_manager)
    free(g_asset_manager);
}

void *Asset_Load(const char *path) {
  (void)path;
  return NULL;
}

void Asset_Unload(void *asset) { (void)asset; }
