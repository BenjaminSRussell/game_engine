#include "../include/core/asset_manager.h"
#include "core/logger.h"
#include "ecs/components/asset_instance_component.h"
#include "ecs/components/transform.h"
#include "ecs/ecs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AssetManager *asset_manager_create(u32 initial_capacity, World *ecs_world,
                                   VFS *vfs) {
  AssetManager *manager = (AssetManager *)calloc(1, sizeof(AssetManager));
  if (!manager) {
    LOG_ERROR("Failed to allocate AssetManager");
    return NULL;
  }

  manager->capacity = initial_capacity;
  manager->count = 0;

  if (initial_capacity > 0) {
    manager->assets = (Asset *)calloc(initial_capacity, sizeof(Asset));
    if (!manager->assets) {
      LOG_ERROR("Failed to allocate assets array");
      free(manager);
      return NULL;
    }
  } else {
    manager->assets = NULL;
  }

  // Store ECS world reference
  manager->ecs_world = ecs_world;
  manager->vfs = vfs;

  // Initialize Instance Registry
  asset_instance_registry_init(&manager->instance_registry, initial_capacity);

  LOG_INFO("Asset Manager created with capacity %u", initial_capacity);
  return manager;
}

void asset_manager_destroy(AssetManager *manager) {
  if (manager) {
    asset_instance_registry_destroy(&manager->instance_registry);
    free(manager);
  }
}

Asset *asset_manager_load(AssetManager *manager, const char *id, AssetType type,
                          const char *path) {
  if (!manager || !manager->vfs) {
    LOG_ERROR("Asset Manager or VFS not initialized");
    return NULL;
  }

  // Check if already loaded
  Asset *existing = asset_manager_get(manager, id);
  if (existing) {
    asset_manager_retain(existing);
    return existing;
  }

  if (!vfs_exists(manager->vfs, path)) {
    LOG_ERROR("Asset load failed: File not found: %s", path);
    return NULL;
  }

  VFSFile *file = vfs_open(manager->vfs, path, VFS_MODE_READ_BINARY);
  if (!file) {
    LOG_ERROR("Asset load failed: Could not open file: %s", path);
    return NULL;
  }

  u64 size = vfs_size(file);
  void *data = calloc(1, size);
  if (!data) {
    LOG_ERROR("Asset load failed: Memory allocation failed (%llu bytes)", size);
    vfs_close(file);
    return NULL;
  }

  if (vfs_read(file, data, size) != size) {
    LOG_WARN("Asset load warning: Read fewer bytes than expected");
  }
  vfs_close(file);

  // Create new asset
  if (manager->count >= manager->capacity) {
    // Expand storage (Stub: Fixed capacity for now)
    LOG_ERROR("Asset Manager capacity reached!");
    free(data);
    return NULL;
  }

  Asset *asset = &manager->assets[manager->count++];
  strncpy(asset->id, id, sizeof(asset->id) - 1);
  strncpy(asset->path, path, sizeof(asset->path) - 1);
  asset->type = type;
  asset->data = data;
  asset->size = (u32)size;
  asset->loaded = true;
  asset->ref_count = 1;

  LOG_INFO("Asset loaded: %s (Type: %d, Size: %u)", id, type, asset->size);
  return asset;
}

Asset *asset_manager_get(AssetManager *manager, const char *id) {
  if (!manager || !id)
    return NULL;

  for (uint32_t i = 0; i < manager->count; i++) {
    if (strcmp(manager->assets[i].id, id) == 0) {
      return &manager->assets[i];
    }
  }

  return NULL;
}

void asset_manager_unload(AssetManager *manager, const char *id) {
  // Stub
}

void asset_manager_retain(Asset *asset) {
  if (asset)
    asset->ref_count++;
}

void asset_manager_release(Asset *asset) {
  if (asset && asset->ref_count > 0) {
    asset->ref_count--;
    if (asset->ref_count == 0) {
      // In a real system, we might unload here or keep it cached
      // For now, simpler logic
    }
  }
}

void asset_manager_register_asset(AssetManager *manager, Asset *asset) {
  if (!manager || !asset)
    return;

  // Simple linear add (ignoring resizing for V1 stub sake, assuming capacity >
  // count)
  if (manager->count < manager->capacity) {
    manager->assets[manager->count] = *asset;
    manager->count++;
    LOG_INFO("Registered asset: %s (Type: %d)", asset->id, asset->type);
  } else {
    LOG_ERROR("Asset Manager capacity reached, cannot register asset: %s",
              asset->id);
  }
}

void asset_manager_preload(AssetManager *manager, const char *manifest_path) {
  // Stub
}

void asset_manager_report(AssetManager *manager) {
  // Stub
}

// Hot-Reload API Stubs
void asset_manager_enable_hot_reload(AssetManager *manager,
                                     const char *watch_path) {}
void asset_manager_disable_hot_reload(AssetManager *manager) {}
void asset_manager_register_reload_callback(AssetManager *manager,
                                            const char *asset_id,
                                            AssetReloadCallback callback,
                                            void *user_data) {}
void asset_manager_reload_asset(AssetManager *manager, const char *asset_id) {}
void asset_manager_update(AssetManager *manager) {}

// Async Loading API Stub
void asset_manager_preload_async(AssetManager *manager,
                                 const char *manifest_path,
                                 LoadingProgressCallback callback,
                                 void *user_data) {}

// LOD API Stubs
void asset_manager_set_lod_config(AssetManager *manager, LODConfig config) {}
void asset_manager_enable_lod(AssetManager *manager, bool enabled) {}
Asset *asset_manager_get_mesh_lod(AssetManager *manager, const char *id,
                                  LODLevel level) {
  return NULL;
}
Asset *asset_manager_get_texture_lod(AssetManager *manager, const char *id,
                                     u32 mip_level) {
  return NULL;
}

// ========================================
// Asset Instance Management
// ========================================

AssetInstance *asset_manager_create_instance(AssetManager *manager,
                                             const char *asset_id,
                                             Vec3 position, Quat rotation) {
  if (!manager || !asset_id)
    return NULL;

  Asset *asset = asset_manager_get(manager, asset_id);
  if (!asset) {
    LOG_WARN("Cannot create instance: Asset '%s' not found", asset_id);
    return NULL;
  }

  // Create basic instance in registry
  AssetInstance *instance =
      asset_instance_registry_create(&manager->instance_registry, asset);
  if (!instance)
    return NULL;

  // Phase 3: Create ECS entity and link it

  return instance;
}

void asset_manager_destroy_instance(AssetManager *manager,
                                    uint32_t instance_id) {
  if (!manager)
    return;

  // Phase 3: Destroy ECS entity before removing instance

  asset_instance_registry_destroy_instance(&manager->instance_registry,
                                           instance_id);
}

AssetInstance *asset_manager_get_instance(AssetManager *manager,
                                          uint32_t instance_id) {
  if (!manager)
    return NULL;
  return asset_instance_registry_get(&manager->instance_registry, instance_id);
}

uint32_t asset_manager_get_instances_for_asset(AssetManager *manager,
                                               const char *asset_id,
                                               AssetInstance **out_instances,
                                               uint32_t max_count) {
  if (!manager)
    return 0;
  return asset_instance_registry_get_instances_for_asset(
      &manager->instance_registry, asset_id, out_instances, max_count);
}

// ========================================
