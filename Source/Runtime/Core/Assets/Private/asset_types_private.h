#pragma once

#include "../Public/unified_assets.h"

// Internal asset structure
typedef struct Asset {
  AssetHandle handle;
  AssetMetadata metadata;
  AssetStatus status;
  AssetPriority priority;

  // Data
  void *data;
  uint64_t data_size;

  // Hot reload tracking
  uint64_t last_check_time;
  uint64_t file_timestamp;

  // Reference counting
  uint32_t ref_count;

  // Loader
  const AssetLoaderCallbacks *loader;
} Asset;

// Asset registry structure
struct AssetRegistry {
  AssetRegistryConfig config;
  bool initialized;

  // Asset storage
  Asset **assets;
  uint32_t asset_count;
  uint32_t asset_capacity;

  // Handle generation
  uint64_t next_handle;

  // Loaders per type
  AssetLoaderCallbacks *loaders[ASSET_TYPE_UNKNOWN];

  // Statistics
  uint64_t total_memory_used;
  uint32_t loads_successful;
  uint32_t loads_failed;
  uint32_t active_loads;
};

// Internal helpers
Asset *find_asset_by_handle(AssetRegistry *registry, AssetHandle handle);
Asset *find_asset_by_path(AssetRegistry *registry, const char *path);
AssetHandle generate_handle(AssetRegistry *registry);
