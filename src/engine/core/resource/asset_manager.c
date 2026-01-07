#include "../include/core/asset_manager.h"
#include "core/logger.h"
#include "ecs/ecs.h"
#include "ecs/components/transform.h"
#include "ecs/components/asset_instance_component.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AssetManager *asset_manager_create(u32 initial_capacity, World *ecs_world) {
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

Asset *asset_manager_load(AssetManager *manager, const char *id, AssetType type, const char *path) {
    // Stub
    return NULL;
}

Asset *asset_manager_get(AssetManager *manager, const char *id) {
    if (!manager || !id) return NULL;
    
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
    // Stub
}

void asset_manager_release(Asset *asset) {
    // Stub
}

void asset_manager_preload(AssetManager *manager, const char *manifest_path) {
    // Stub
}

void asset_manager_report(AssetManager *manager) {
    // Stub
}

// Hot-Reload API Stubs
void asset_manager_enable_hot_reload(AssetManager *manager, const char *watch_path) {}
void asset_manager_disable_hot_reload(AssetManager *manager) {}
void asset_manager_register_reload_callback(AssetManager *manager, const char *asset_id, AssetReloadCallback callback, void *user_data) {}
void asset_manager_reload_asset(AssetManager *manager, const char *asset_id) {}
void asset_manager_update(AssetManager *manager) {}

// Async Loading API Stub
void asset_manager_preload_async(AssetManager *manager, const char *manifest_path, LoadingProgressCallback callback, void *user_data) {}

// LOD API Stubs
void asset_manager_set_lod_config(AssetManager *manager, LODConfig config) {}
void asset_manager_enable_lod(AssetManager *manager, bool enabled) {}
Asset *asset_manager_get_mesh_lod(AssetManager *manager, const char *id, LODLevel level) { return NULL; }
Asset *asset_manager_get_texture_lod(AssetManager *manager, const char *id, u32 mip_level) { return NULL; }

// ========================================
// Asset Instance Management
// ========================================

AssetInstance *asset_manager_create_instance(AssetManager *manager, const char *asset_id, Vec3 position, Quat rotation) {
    if (!manager || !asset_id) return NULL;
    
    Asset *asset = asset_manager_get(manager, asset_id);
    if (!asset) {
        LOG_WARN("Cannot create instance: Asset '%s' not found", asset_id);
        return NULL;
    }
    
    // Create basic instance in registry
    AssetInstance *instance = asset_instance_registry_create(&manager->instance_registry, asset);
    if (!instance) return NULL;
    
    // Phase 3: Create ECS entity and link it

    
    return instance;
}

void asset_manager_destroy_instance(AssetManager *manager, uint32_t instance_id) {
    if (!manager) return;
    
    // Phase 3: Destroy ECS entity before removing instance

    
    asset_instance_registry_destroy_instance(&manager->instance_registry, instance_id);
}

AssetInstance *asset_manager_get_instance(AssetManager *manager, uint32_t instance_id) {
    if (!manager) return NULL;
    return asset_instance_registry_get(&manager->instance_registry, instance_id);
}

uint32_t asset_manager_get_instances_for_asset(AssetManager *manager, const char *asset_id, AssetInstance **out_instances, uint32_t max_count) {
    if (!manager) return 0;
    return asset_instance_registry_get_instances_for_asset(&manager->instance_registry, asset_id, out_instances, max_count);
}

// ========================================

