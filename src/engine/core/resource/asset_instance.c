#include "resource/asset_instance.h"
#include "core/asset_manager.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

void asset_instance_registry_init(AssetInstanceRegistry *registry, uint32_t initial_capacity) {
    if (!registry) return;
    
    registry->capacity = initial_capacity > 0 ? initial_capacity : 64;
    registry->instances = (AssetInstance *)calloc(registry->capacity, sizeof(AssetInstance));
    registry->count = 0;
    registry->next_id = 1; // Start IDs at 1, reserve 0 for "invalid"
    
    if (!registry->instances) {
        LOG_ERROR("Failed to allocate asset instance registry");
    }
}

void asset_instance_registry_destroy(AssetInstanceRegistry *registry) {
    if (!registry) return;
    
    if (registry->instances) {
        free(registry->instances);
        registry->instances = NULL;
    }
    
    registry->count = 0;
    registry->capacity = 0;
    registry->next_id = 1;
}

AssetInstance *asset_instance_registry_create(AssetInstanceRegistry *registry, Asset *source_asset) {
    if (!registry || !source_asset) return NULL;
    
    // Check if we need to resize
    if (registry->count >= registry->capacity) {
        uint32_t new_capacity = registry->capacity * 2;
        AssetInstance *new_instances = (AssetInstance *)realloc(
            registry->instances, 
            new_capacity * sizeof(AssetInstance)
        );
        
        if (!new_instances) {
            LOG_ERROR("Failed to resize asset instance registry");
            return NULL;
        }
        
        registry->instances = new_instances;
        registry->capacity = new_capacity;
        
        // Zero out new memory
        memset(&registry->instances[registry->count], 0, 
               (new_capacity - registry->count) * sizeof(AssetInstance));
    }
    
    // Create new instance
    AssetInstance *instance = &registry->instances[registry->count];
    instance->instance_id = registry->next_id++;
    instance->source_asset = source_asset;
    instance->entity_id = 0; // Will be set when entity is created
    instance->transform_component_id = 0;
    instance->is_active = true;
    
    registry->count++;
    
    LOG_INFO("Created asset instance %u from asset '%s'", 
             instance->instance_id, source_asset->id);
    
    return instance;
}

void asset_instance_registry_destroy_instance(AssetInstanceRegistry *registry, uint32_t instance_id) {
    if (!registry) return;
    
    // Find instance by ID
    for (uint32_t i = 0; i < registry->count; i++) {
        if (registry->instances[i].instance_id == instance_id) {
            LOG_INFO("Destroying asset instance %u", instance_id);
            
            // Swap with last element and shrink
            registry->instances[i] = registry->instances[registry->count - 1];
            memset(&registry->instances[registry->count - 1], 0, sizeof(AssetInstance));
            registry->count--;
            return;
        }
    }
    
    LOG_WARN("Asset instance %u not found for destruction", instance_id);
}

AssetInstance *asset_instance_registry_get(AssetInstanceRegistry *registry, uint32_t instance_id) {
    if (!registry) return NULL;
    
    for (uint32_t i = 0; i < registry->count; i++) {
        if (registry->instances[i].instance_id == instance_id) {
            return &registry->instances[i];
        }
    }
    
    return NULL;
}

uint32_t asset_instance_registry_get_instances_for_asset(
    AssetInstanceRegistry *registry, 
    const char *asset_id,
    AssetInstance **out_instances, 
    uint32_t max_count
) {
    if (!registry || !asset_id || !out_instances) return 0;
    
    uint32_t found = 0;
    
    for (uint32_t i = 0; i < registry->count && found < max_count; i++) {
        AssetInstance *inst = &registry->instances[i];
        if (inst->source_asset && strcmp(inst->source_asset->id, asset_id) == 0) {
            out_instances[found++] = inst;
        }
    }
    
    return found;
}
