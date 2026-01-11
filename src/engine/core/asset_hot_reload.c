#include "common.h"
#include <core/asset_manager.h>
#include <core/hashmap.h>
#include <core/hot_reload.h>
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
  char *path;
  AssetReloadCallback callback;
  void *user_data;
  time_t last_modified;
} AssetWatch;

static HashMap *g_asset_watches = NULL;

static u32 str_hash(const void *key) {
  const char *str = (const char *)key;
  u32 hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static bool str_equals(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b) == 0;
}

static time_t get_asset_time(const char *path) {
  struct stat attr;
  if (stat(path, &attr) == 0) {
    return attr.st_mtime;
  }
  return 0;
}

void hot_reload_watch_asset(const char *asset_path,
                            AssetReloadCallback callback, void *user_data) {
  if (!g_asset_watches) {
    g_asset_watches = hashmap_create(32, sizeof(char *), sizeof(AssetWatch *),
                                     str_hash, str_equals);
  }

  AssetWatch *watch = malloc(sizeof(AssetWatch));
  watch->path = strdup(asset_path);
  watch->callback = callback;
  watch->user_data = user_data;
  watch->last_modified = get_asset_time(asset_path);

  hashmap_set(g_asset_watches, asset_path, watch);
  LOG_INFO("Watching asset for hot-reload: %s", asset_path);
}

void hot_reload_update_assets(void) {
  if (!g_asset_watches)
    return;
  // Iterate logic
}
