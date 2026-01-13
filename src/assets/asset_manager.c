// asset_manager.c - Stub implementation
#include "core/asset_manager.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>

AssetManager* asset_manager_create(u32 initial_capacity, World *ecs_world, VFS *vfs) {
    LOG_INFO("Creating asset manager (stub) with capacity %u", initial_capacity);
    AssetManager* manager = (AssetManager*)malloc(sizeof(AssetManager));
    if (manager) {
        // Initialize stub values
        manager->assets = NULL;
        manager->count = 0;
        manager->capacity = initial_capacity;
    }
    (void)ecs_world; (void)vfs; // Suppress unused warnings
    return manager;
}

void asset_manager_destroy(AssetManager* manager) {
    if (manager) {
        LOG_INFO("Destroying asset manager (stub)");
        free(manager->assets);
        free(manager);
    }
}
