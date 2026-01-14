#ifndef ASSET_INSTANCE_COMPONENT_H
#define ASSET_INSTANCE_COMPONENT_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct Asset Asset;
typedef struct World World;
typedef unsigned int ECSComponentID;
typedef struct World World;

/**
 * RenderFlags - Control rendering behavior for this instance
 */
typedef enum {
    RENDER_FLAG_VISIBLE = 1 << 0,
    RENDER_FLAG_CAST_SHADOW = 1 << 1,
    RENDER_FLAG_RECEIVE_SHADOW = 1 << 2,
    RENDER_FLAG_WIREFRAME = 1 << 3,
    RENDER_FLAG_NO_LOD = 1 << 4,
} RenderFlags;

/**
 * AssetInstanceComponent - ECS component for entities with asset instances
 * 
 * Links an entity to its asset instance and controls rendering behavior
 */
typedef struct {
    uint32_t asset_instance_id;  // Back-reference to AssetInstance in registry
    Asset *source_asset;          // Cached pointer for quick access
    uint32_t render_flags;        // Bitfield of RenderFlags
    float lod_bias;               // LOD distance bias multiplier
} AssetInstanceComponent;

// Component registration (called during ECS initialization)
void asset_instance_component_register(World *world);

// Get the component ID (auto-registers if needed)
typedef unsigned int ECSComponentID;
ECSComponentID asset_instance_component_get_id(World *world);

ECSComponentID asset_instance_component_get_id(World *world);

#endif // ASSET_INSTANCE_COMPONENT_H
