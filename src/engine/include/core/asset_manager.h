// include/core/asset_manager.h
//
// Purpose: Defines the public API and data structures for a generic asset
// manager. This system is responsible for loading, storing, and providing
// access to various types of game assets (textures, meshes, sounds, shaders,
// configurations, and generic data). It includes reference counting for asset
// lifecycle management and optional thread-safe access.
//
// Public APIs:
// - `AssetType`: Enumeration categorizing the different types of assets
// managed.
// - `Asset`: Structure representing a single loaded asset, containing its
// unique ID, type,
//   a pointer to its data, size, loaded status, and a reference count.
// - `AssetManager`: Structure holding the collection of all managed assets,
// along with
//   its capacity, current count, and a mutex for thread-safe operations.
// - `asset_manager_create`: Initializes a new `AssetManager` with a specified
// initial capacity.
// - `asset_manager_destroy`: Frees all resources held by the `AssetManager` and
// its managed assets.
// - `asset_manager_load`: Loads an asset from a given path, assigns it an ID,
// and tracks its type.
// - `asset_manager_get`: Retrieves an already loaded asset by its unique ID.
// - `asset_manager_unload`: Removes an asset from the manager and frees its
// resources if its reference count allows.
// - `asset_manager_retain`: Increments the reference count of an asset.
// - `asset_manager_release`: Decrements the reference count of an asset,
// unloading it if the count drops to zero.
// - `asset_manager_preload`: Loads multiple assets specified in a manifest
// file.
// - `asset_manager_report`: Generates a report of currently loaded assets and
// their status.
//
// Ownership: The `AssetManager` owns the `Asset` objects it manages, including
// their `data` pointers. Users are responsible for calling
// `asset_manager_release` when they are done with an asset to allow the manager
// to correctly deallocate resources.
//
// Invariants:
// - An `AssetManager` must be created with `asset_manager_create` and destroyed
// with `asset_manager_destroy`.
// - Asset IDs (`Asset.id`) must be unique strings for proper retrieval.
// - Reference counting (`ref_count`) must be correctly managed by callers to
// prevent premature deallocation
//   or memory leaks.
// - Asset data is loaded into memory owned by the manager and should not be
// freed externally.
// - Thread-safe access is provided through `pthread_mutex_t` when
// `PLATFORM_WEB` is not defined.
//
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

// Forward declaration for ECS integration (Phase 3)
typedef struct World World;

#include "../common.h"
#include "../vfs/vfs.h"
#include "file_watcher.h"
#include <renderer/lod_system.h>
#include "../resource/asset_instance.h"

// Forward declarations
typedef struct AssetManager AssetManager;
typedef struct World World;

typedef enum {
  ASSET_TYPE_UNKNOWN,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MESH,
  ASSET_TYPE_MODEL,
  ASSET_TYPE_SOUND,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_AUDIO_STREAM,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_CONFIG,
  ASSET_TYPE_MATERIAL,
  ASSET_TYPE_DATA,

} AssetType;

// Asset struct - forward declared in asset_instance.h, definition here
struct Asset {
  char id[128];
  char path[256];
  AssetType type;
  void *data;
  u32 size;
  bool loaded;
  u32 ref_count;
};

typedef void (*AssetReloadCallback)(const char *asset_id, void *user_data);

typedef struct {
  char asset_id[128];
  AssetReloadCallback callback;
  void *user_data;
} ReloadCallbackEntry;

typedef struct AssetManager {
  Asset *assets;
  u32 count;
  u32 capacity;

  VFS *vfs;

  // Job System
  struct ThreadPool *thread_pool;

  // Hot-Reload
  FileWatcher *watcher;
  ReloadCallbackEntry *reload_callbacks;
  u32 callback_count;
  u32 callback_capacity;

  // LOD System
  LODConfig lod_config;
  bool lod_enabled;

  // Asset Instance Registry
  AssetInstanceRegistry instance_registry;
  
  // ECS integration (Phase 3)
  World *ecs_world;

#ifndef PLATFORM_WEB
  pthread_mutex_t mutex; // Protects asset array and operations
#endif
} AssetManager;

AssetManager *asset_manager_create(u32 initial_capacity, World *ecs_world);
void asset_manager_destroy(AssetManager *manager);
Asset *asset_manager_load(AssetManager *manager, const char *id, AssetType type,
                          const char *path);
Asset *asset_manager_get(AssetManager *manager, const char *id);
void asset_manager_unload(AssetManager *manager, const char *id);
void asset_manager_retain(Asset *asset);
void asset_manager_release(Asset *asset);
void asset_manager_preload(AssetManager *manager, const char *manifest_path);
void asset_manager_report(AssetManager *manager);

// Hot-Reload API
void asset_manager_enable_hot_reload(AssetManager *manager,
                                     const char *watch_path);
void asset_manager_disable_hot_reload(AssetManager *manager);
void asset_manager_register_reload_callback(AssetManager *manager,
                                            const char *asset_id,
                                            AssetReloadCallback callback,
                                            void *user_data);
void asset_manager_reload_asset(AssetManager *manager, const char *asset_id);
void asset_manager_update(AssetManager *manager);

// Async Loading API
typedef struct {
  u32 total_assets;
  u32 loaded_assets;
  float progress; // 0.0 to 1.0
} LoadingProgress;

typedef void (*LoadingProgressCallback)(const LoadingProgress *progress,
                                        void *user_data);

void asset_manager_preload_async(AssetManager *manager,
                                 const char *manifest_path,
                                 LoadingProgressCallback callback,
                                 void *user_data);

// LOD API
void asset_manager_set_lod_config(AssetManager *manager, LODConfig config);
void asset_manager_enable_lod(AssetManager *manager, bool enabled);
Asset *asset_manager_get_mesh_lod(AssetManager *manager, const char *id,
                                  LODLevel level);
Asset *asset_manager_get_texture_lod(AssetManager *manager, const char *id,
                                     u32 mip_level);

// ========================================
// Asset Instance Management (Phase 2)
// ========================================

#include <math/vec3.h>
#include <math/quat.h>

// Create an instance of an asset with transform
// Returns the instance, or NULL on failure
// Automatically creates an ECS entity with Transform and AssetInstanceComponent
AssetInstance *asset_manager_create_instance(AssetManager *manager, const char *asset_id, Vec3 position, Quat rotation);

// Destroy an asset instance and its associated entity
void asset_manager_destroy_instance(AssetManager *manager, uint32_t instance_id);

// Get an instance by ID
AssetInstance *asset_manager_get_instance(AssetManager *manager, uint32_t instance_id);

// Get all instances for a given asset
uint32_t asset_manager_get_instances_for_asset(AssetManager *manager, const char *asset_id, AssetInstance **out_instances, uint32_t max_count);

#endif
