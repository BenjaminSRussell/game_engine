#ifndef ASSET_INSTANCE_H
#define ASSET_INSTANCE_H

#include <stdbool.h>
#include <stdint.h>

// Forward declaration
typedef struct Asset Asset;

/**
 * AssetInstance - Represents a single placed instance of an asset in the world
 * 
 * Each instance has:
 * - Unique ID for tracking
 * - Reference to source asset (mesh, model, etc.)
 * - Link to ECS entity for transform and rendering
 * - Active state flag
 */
typedef struct {
    uint32_t instance_id;           // Unique instance identifier
    Asset *source_asset;            // Pointer to source asset data
    uint32_t entity_id;             // Associated ECS entity (0 if detached)
    uint32_t transform_component_id; // Quick reference to transform component
    bool is_active;                 // Whether this instance is active
} AssetInstance;

/**
 * AssetInstanceRegistry - Dynamic array of asset instances
 * 
 * Manages all active instances with O(1) creation and lookup by ID
 */
typedef struct {
    AssetInstance *instances;   // Dynamic array of instances
    uint32_t capacity;          // Total allocated capacity
    uint32_t count;             // Current number of active instances
    uint32_t next_id;           // Counter for generating unique IDs
} AssetInstanceRegistry;

// Registry management
void asset_instance_registry_init(AssetInstanceRegistry *registry, uint32_t initial_capacity);
void asset_instance_registry_destroy(AssetInstanceRegistry *registry);

// Instance operations
AssetInstance *asset_instance_registry_create(AssetInstanceRegistry *registry, Asset *source_asset);
void asset_instance_registry_destroy_instance(AssetInstanceRegistry *registry, uint32_t instance_id);
AssetInstance *asset_instance_registry_get(AssetInstanceRegistry *registry, uint32_t instance_id);

// Queries
uint32_t asset_instance_registry_get_instances_for_asset(
    AssetInstanceRegistry *registry, 
    const char *asset_id,
    AssetInstance **out_instances, 
    uint32_t max_count
);

#endif // ASSET_INSTANCE_H
