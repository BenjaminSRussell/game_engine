#include "asset_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// =============================================================================
// HOT RELOAD SUPPORT
// =============================================================================

void asset_enable_hot_reload(AssetRegistry *registry, bool enabled) {
  if (!registry)
    return;

  registry->config.enable_hot_reload = enabled;
  LOG_INFO(LOG_CAT_ASSET, "Asset hot-reload %s",
           enabled ? "enabled" : "disabled");
}

static uint64_t get_file_timestamp(const char *path) {
  struct stat file_stat;
  if (stat(path, &file_stat) == 0) {
    return (uint64_t)file_stat.st_mtime;
  }
  return 0;
}

void asset_check_for_changes(AssetRegistry *registry) {
  if (!registry || !registry->config.enable_hot_reload)
    return;

  uint32_t reloaded_count = 0;
  uint64_t current_time = (uint64_t)time(NULL);

  for (uint32_t i = 0; i < registry->asset_count; i++) {
    Asset *asset = registry->assets[i];
    if (!asset || asset->status != ASSET_STATUS_LOADED)
      continue;

    // Check timestamp every second (throttle)
    if (current_time - asset->last_check_time < 1)
      continue;

    asset->last_check_time = current_time;

    // Get current file timestamp
    uint64_t current_timestamp = get_file_timestamp(asset->metadata.path);

    // Check if file has changed
    if (current_timestamp > asset->file_timestamp) {
      LOG_INFO(LOG_CAT_ASSET, "Detected change in asset: %s",
               asset->metadata.path);
      asset_reload(registry, asset->handle);
      reloaded_count++;
    }
  }

  if (reloaded_count > 0) {
    LOG_INFO(LOG_CAT_ASSET, "Hot-reload complete: %u assets reloaded",
             reloaded_count);
  }
}

void asset_reload(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  if (!asset) {
    LOG_WARN(LOG_CAT_ASSET, "Attempting to reload invalid asset handle: %llu",
             handle);
    return;
  }

  if (!asset->loader || !asset->loader->load) {
    LOG_ERROR(LOG_CAT_ASSET, "Cannot reload asset: no loader available");
    return;
  }

  LOG_INFO(LOG_CAT_ASSET, "Reloading asset: %s", asset->metadata.path);

  asset->status = ASSET_STATUS_RELOADING;

  // Unload old data
  if (asset->data && asset->loader->unload) {
    asset->loader->unload(asset->data);
    registry->total_memory_used -= asset->data_size;
  }

  // Reload asset data
  void *new_data = NULL;
  uint64_t new_size = 0;

  if (asset->loader->load(asset->metadata.path, &new_data, &new_size)) {
    asset->data = new_data;
    asset->data_size = new_size;
    asset->metadata.size_bytes = new_size;
    asset->file_timestamp = get_file_timestamp(asset->metadata.path);
    asset->status = ASSET_STATUS_LOADED;
    registry->total_memory_used += new_size;

    LOG_INFO(LOG_CAT_ASSET, "Asset reloaded successfully: %s",
             asset->metadata.path);
  } else {
    asset->status = ASSET_STATUS_ERROR;
    LOG_ERROR(LOG_CAT_ASSET, "Failed to reload asset: %s",
              asset->metadata.path);
  }
}

// =============================================================================
// CUSTOM LOADER REGISTRATION
// =============================================================================

bool asset_register_loader(AssetRegistry *registry, AssetType type,
                           const AssetLoaderCallbacks *callbacks) {
  if (!registry || type >= ASSET_TYPE_UNKNOWN || !callbacks) {
    LOG_ERROR(LOG_CAT_ASSET, "Invalid parameters for loader registration");
    return false;
  }

  if (!callbacks->load) {
    LOG_ERROR(LOG_CAT_ASSET, "Loader must implement load callback");
    return false;
  }

  registry->loaders[type] = (AssetLoaderCallbacks *)callbacks;

  LOG_INFO(LOG_CAT_ASSET, "Registered custom loader for type: %s",
           asset_type_to_string(type));

  return true;
}
