#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct AssetRegistry AssetRegistry;
typedef struct Asset Asset;
typedef struct AssetLoader AssetLoader;

// Asset types
typedef enum {
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MODEL,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_MATERIAL,
  ASSET_TYPE_ANIMATION,
  ASSET_TYPE_FONT,
  ASSET_TYPE_SCRIPT,
  ASSET_TYPE_LEVEL,
  ASSET_TYPE_PREFAB,
  ASSET_TYPE_UNKNOWN
} AssetType;

// Asset status
typedef enum {
  ASSET_STATUS_UNLOADED,
  ASSET_STATUS_LOADING,
  ASSET_STATUS_LOADED,
  ASSET_STATUS_ERROR,
  ASSET_STATUS_RELOADING
} AssetStatus;

// Asset load priority
typedef enum {
  ASSET_PRIORITY_CRITICAL, // Must be loaded immediately
  ASSET_PRIORITY_HIGH,     // Load soon
  ASSET_PRIORITY_NORMAL,   // Standard loading
  ASSET_PRIORITY_LOW       // Load when idle
} AssetPriority;

// Asset handle (opaque)
typedef uint64_t AssetHandle;
#define INVALID_ASSET_HANDLE 0

// Asset metadata
typedef struct {
  char name[256];
  char path[512];
  AssetType type;
  uint64_t size_bytes;
  uint64_t timestamp;
  uint32_t version;
  uint32_t ref_count;
} AssetMetadata;

// Asset registry config
typedef struct {
  const char *asset_root_path;
  uint32_t max_assets;
  uint32_t cache_size_mb;
  bool enable_hot_reload;
  bool enable_compression;
  bool enable_streaming;
} AssetRegistryConfig;

// =============================================================================
// ASSET REGISTRY
// =============================================================================

// Initialize asset registry
AssetRegistry *asset_registry_create(const AssetRegistryConfig *config);
void asset_registry_destroy(AssetRegistry *registry);

// Load assets
AssetHandle asset_load(AssetRegistry *registry, const char *path,
                       AssetType type, AssetPriority priority);
AssetHandle asset_load_async(AssetRegistry *registry, const char *path,
                             AssetType type, AssetPriority priority);
bool asset_unload(AssetRegistry *registry, AssetHandle handle);

// Asset queries
bool asset_is_loaded(AssetRegistry *registry, AssetHandle handle);
AssetStatus asset_get_status(AssetRegistry *registry, AssetHandle handle);
const AssetMetadata *asset_get_metadata(AssetRegistry *registry,
                                        AssetHandle handle);
void *asset_get_data(AssetRegistry *registry, AssetHandle handle);

// Asset reference counting
void asset_add_ref(AssetRegistry *registry, AssetHandle handle);
void asset_release(AssetRegistry *registry, AssetHandle handle);

// Hot reload
void asset_enable_hot_reload(AssetRegistry *registry, bool enabled);
void asset_check_for_changes(AssetRegistry *registry);
void asset_reload(AssetRegistry *registry, AssetHandle handle);

// Asset management
uint32_t asset_get_loaded_count(AssetRegistry *registry);
uint64_t asset_get_memory_usage(AssetRegistry *registry);
void asset_garbage_collect(AssetRegistry *registry);
void asset_flush_all(AssetRegistry *registry);

// Asset utilities
const char *asset_type_to_string(AssetType type);
AssetType asset_type_from_extension(const char *extension);
bool asset_validate_path(const char *path);

// =============================================================================
// ASSET LOADER INTERFACE (For custom loaders)
// =============================================================================

// Loader callbacks
typedef struct {
  bool (*load)(const char *path, void **out_data, uint64_t *out_size);
  bool (*unload)(void *data);
  bool (*validate)(const uint8_t *data, uint64_t size);
  const char *(*get_supported_extensions)();
} AssetLoaderCallbacks;

// Register custom loader
bool asset_register_loader(AssetRegistry *registry, AssetType type,
                           const AssetLoaderCallbacks *callbacks);
