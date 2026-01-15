#include "asset_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// =============================================================================
// ASSET REGISTRY INITIALIZATION
// =============================================================================

AssetRegistry *asset_registry_create(const AssetRegistryConfig *config) {
  if (!config) {
    LOG_ERROR(LOG_CAT_ASSET, "Invalid asset registry config");
    return NULL;
  }

  AssetRegistry *registry =
      (AssetRegistry *)UNIFIED_ALLOC(sizeof(AssetRegistry));
  if (!registry) {
    LOG_ERROR(LOG_CAT_ASSET, "Failed to allocate asset registry");
    return NULL;
  }
  memset(registry, 0, sizeof(AssetRegistry));

  // Copy config
  registry->config = *config;

  // Initialize asset storage
  registry->asset_capacity = config->max_assets > 0 ? config->max_assets : 1024;
  registry->assets =
      (Asset **)UNIFIED_ALLOC(sizeof(Asset *) * registry->asset_capacity);
  memset(registry->assets, 0, sizeof(Asset *) * registry->asset_capacity);

  registry->next_handle = 1; // Start at 1, 0 is invalid
  registry->initialized = true;

  LOG_INFO(LOG_CAT_ASSET, "Asset registry created (max_assets=%u, root=%s)",
           registry->asset_capacity,
           config->asset_root_path ? config->asset_root_path : "none");

  return registry;
}

void asset_registry_destroy(AssetRegistry *registry) {
  if (!registry)
    return;

  LOG_INFO(LOG_CAT_ASSET, "Destroying asset registry");

  // Unload all assets
  for (uint32_t i = 0; i < registry->asset_count; i++) {
    Asset *asset = registry->assets[i];
    if (asset) {
      if (asset->data && asset->loader && asset->loader->unload) {
        asset->loader->unload(asset->data);
      }
      UNIFIED_FREE(asset);
    }
  }

  UNIFIED_FREE(registry->assets);
  UNIFIED_FREE(registry);
}

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

AssetHandle generate_handle(AssetRegistry *registry) {
  return registry->next_handle++;
}

Asset *find_asset_by_handle(AssetRegistry *registry, AssetHandle handle) {
  if (handle == INVALID_ASSET_HANDLE)
    return NULL;

  for (uint32_t i = 0; i < registry->asset_count; i++) {
    if (registry->assets[i] && registry->assets[i]->handle == handle) {
      return registry->assets[i];
    }
  }

  return NULL;
}

Asset *find_asset_by_path(AssetRegistry *registry, const char *path) {
  if (!path)
    return NULL;

  for (uint32_t i = 0; i < registry->asset_count; i++) {
    Asset *asset = registry->assets[i];
    if (asset && strcmp(asset->metadata.path, path) == 0) {
      return asset;
    }
  }

  return NULL;
}

// =============================================================================
// ASSET LOADING
// =============================================================================

AssetHandle asset_load(AssetRegistry *registry, const char *path,
                       AssetType type, AssetPriority priority) {
  if (!registry || !path) {
    LOG_ERROR(LOG_CAT_ASSET, "Invalid registry or path");
    return INVALID_ASSET_HANDLE;
  }

  // Check if already loaded
  Asset *existing = find_asset_by_path(registry, path);
  if (existing) {
    LOG_INFO(LOG_CAT_ASSET, "Asset already loaded: %s", path);
    existing->ref_count++;
    return existing->handle;
  }

  // Check capacity
  if (registry->asset_count >= registry->asset_capacity) {
    LOG_ERROR(LOG_CAT_ASSET, "Asset registry full (max=%u)",
              registry->asset_capacity);
    return INVALID_ASSET_HANDLE;
  }

  // Create new asset
  Asset *asset = (Asset *)UNIFIED_ALLOC(sizeof(Asset));
  if (!asset) {
    LOG_ERROR(LOG_CAT_ASSET, "Failed to allocate asset");
    return INVALID_ASSET_HANDLE;
  }
  memset(asset, 0, sizeof(Asset));

  // Initialize metadata
  asset->handle = generate_handle(registry);
  strncpy(asset->metadata.path, path, sizeof(asset->metadata.path) - 1);
  asset->metadata.type = type;
  asset->metadata.ref_count = 1;
  asset->ref_count = 1;
  asset->priority = priority;
  asset->status = ASSET_STATUS_LOADING;

  // Extract name from path
  const char *name_start = strrchr(path, '/');
  if (!name_start)
    name_start = path;
  else
    name_start++;
  strncpy(asset->metadata.name, name_start, sizeof(asset->metadata.name) - 1);

  // Get loader
  asset->loader = registry->loaders[type];
  if (!asset->loader) {
    LOG_WARN(LOG_CAT_ASSET, "No loader registered for type %d (path=%s)", type,
             path);
    asset->status = ASSET_STATUS_ERROR;
  } else {
    // Load asset data
    LOG_INFO(LOG_CAT_ASSET, "Loading asset: %s (type=%d, priority=%d)", path,
             type, priority);

    if (asset->loader->load(path, &asset->data, &asset->data_size)) {
      asset->status = ASSET_STATUS_LOADED;
      asset->metadata.size_bytes = asset->data_size;
      registry->total_memory_used += asset->data_size;
      registry->loads_successful++;

      LOG_INFO(LOG_CAT_ASSET, "Asset loaded successfully: %s (%lu bytes)", path,
               asset->data_size);
    } else {
      asset->status = ASSET_STATUS_ERROR;
      registry->loads_failed++;
      LOG_ERROR(LOG_CAT_ASSET, "Failed to load asset: %s", path);
    }
  }

  // Add to registry
  registry->assets[registry->asset_count++] = asset;

  return asset->handle;
}

AssetHandle asset_load_async(AssetRegistry *registry, const char *path,
                             AssetType type, AssetPriority priority) {
  // TODO: Implement async loading
  LOG_WARN(LOG_CAT_ASSET,
           "Async loading not implemented, falling back to sync load");
  return asset_load(registry, path, type, priority);
}

bool asset_unload(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  if (!asset) {
    LOG_WARN(LOG_CAT_ASSET, "Attempting to unload invalid asset handle: %llu",
             handle);
    return false;
  }

  if (asset->ref_count > 0) {
    asset->ref_count--;
    if (asset->ref_count > 0) {
      return true; // Still referenced
    }
  }

  LOG_INFO(LOG_CAT_ASSET, "Unloading asset: %s", asset->metadata.path);

  // Unload data
  if (asset->data && asset->loader && asset->loader->unload) {
    asset->loader->unload(asset->data);
    registry->total_memory_used -= asset->data_size;
  }

  // Remove from registry
  for (uint32_t i = 0; i < registry->asset_count; i++) {
    if (registry->assets[i] == asset) {
      // Shift remaining assets
      for (uint32_t j = i; j < registry->asset_count - 1; j++) {
        registry->assets[j] = registry->assets[j + 1];
      }
      registry->asset_count--;
      break;
    }
  }

  UNIFIED_FREE(asset);
  return true;
}

// =============================================================================
// ASSET QUERIES
// =============================================================================

bool asset_is_loaded(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  return asset && asset->status == ASSET_STATUS_LOADED;
}

AssetStatus asset_get_status(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  return asset ? asset->status : ASSET_STATUS_ERROR;
}

const AssetMetadata *asset_get_metadata(AssetRegistry *registry,
                                        AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  return asset ? &asset->metadata : NULL;
}

void *asset_get_data(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  return (asset && asset->status == ASSET_STATUS_LOADED) ? asset->data : NULL;
}

// =============================================================================
// REFERENCE COUNTING
// =============================================================================

void asset_add_ref(AssetRegistry *registry, AssetHandle handle) {
  Asset *asset = find_asset_by_handle(registry, handle);
  if (asset) {
    asset->ref_count++;
    asset->metadata.ref_count = asset->ref_count;
  }
}

void asset_release(AssetRegistry *registry, AssetHandle handle) {
  asset_unload(registry, handle);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

const char *asset_type_to_string(AssetType type) {
  switch (type) {
  case ASSET_TYPE_TEXTURE:
    return "TEXTURE";
  case ASSET_TYPE_MODEL:
    return "MODEL";
  case ASSET_TYPE_AUDIO:
    return "AUDIO";
  case ASSET_TYPE_SHADER:
    return "SHADER";
  case ASSET_TYPE_MATERIAL:
    return "MATERIAL";
  case ASSET_TYPE_ANIMATION:
    return "ANIMATION";
  case ASSET_TYPE_FONT:
    return "FONT";
  case ASSET_TYPE_SCRIPT:
    return "SCRIPT";
  case ASSET_TYPE_LEVEL:
    return "LEVEL";
  case ASSET_TYPE_PREFAB:
    return "PREFAB";
  default:
    return "UNKNOWN";
  }
}

AssetType asset_type_from_extension(const char *extension) {
  if (!extension)
    return ASSET_TYPE_UNKNOWN;

  if (strcmp(extension, ".png") == 0 || strcmp(extension, ".jpg") == 0 ||
      strcmp(extension, ".dds") == 0)
    return ASSET_TYPE_TEXTURE;
  if (strcmp(extension, ".obj") == 0 || strcmp(extension, ".fbx") == 0 ||
      strcmp(extension, ".gltf") == 0)
    return ASSET_TYPE_MODEL;
  if (strcmp(extension, ".wav") == 0 || strcmp(extension, ".ogg") == 0)
    return ASSET_TYPE_AUDIO;
  if (strcmp(extension, ".glsl") == 0 || strcmp(extension, ".hlsl") == 0)
    return ASSET_TYPE_SHADER;
  if (strcmp(extension, ".mat") == 0)
    return ASSET_TYPE_MATERIAL;
  if (strcmp(extension, ".anim") == 0)
    return ASSET_TYPE_ANIMATION;
  if (strcmp(extension, ".ttf") == 0)
    return ASSET_TYPE_FONT;
  if (strcmp(extension, ".lua") == 0)
    return ASSET_TYPE_SCRIPT;

  return ASSET_TYPE_UNKNOWN;
}

bool asset_validate_path(const char *path) {
  if (!path || path[0] == '\0')
    return false;

  // Check for directory traversal attempts
  if (strstr(path, ".."))
    return false;

  return true;
}

uint32_t asset_get_loaded_count(AssetRegistry *registry) {
  if (!registry)
    return 0;
  return registry->asset_count;
}

uint64_t asset_get_memory_usage(AssetRegistry *registry) {
  if (!registry)
    return 0;
  return registry->total_memory_used;
}

void asset_garbage_collect(AssetRegistry *registry) {
  if (!registry)
    return;

  LOG_INFO(LOG_CAT_ASSET, "Running asset garbage collection");

  uint32_t collected = 0;
  for (int32_t i = (int32_t)registry->asset_count - 1; i >= 0; i--) {
    Asset *asset = registry->assets[i];
    if (asset && asset->ref_count == 0) {
      asset_unload(registry, asset->handle);
      collected++;
    }
  }

  LOG_INFO(LOG_CAT_ASSET, "Garbage collection complete: %u assets collected",
           collected);
}

void asset_flush_all(AssetRegistry *registry) {
  if (!registry)
    return;

  LOG_INFO(LOG_CAT_ASSET, "Flushing all assets");

  while (registry->asset_count > 0) {
    Asset *asset = registry->assets[0];
    if (asset) {
      asset->ref_count = 0; // Force unload
      asset_unload(registry, asset->handle);
    }
  }
}
