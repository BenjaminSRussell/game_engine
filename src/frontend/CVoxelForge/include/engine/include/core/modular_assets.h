// include/engine/modular_assets.h
//
// Purpose: Modular asset system for Lego-like snap-together components.
// All assets are built from modular pieces that can be combined.
//
#ifndef MODULAR_ASSETS_H
#define MODULAR_ASSETS_H

#include "../common.h"
#include <math/vec3.h>
#include <math/mat4.h>

// Forward declarations
struct Mesh;

// Connection type (how components snap together)
typedef enum {
    CONNECTION_TYPE_SNAP,         // Simple snap (like Lego)
    CONNECTION_TYPE_ROTATE,       // Rotating connection
    CONNECTION_TYPE_SLIDE,        // Sliding connection
    CONNECTION_TYPE_MAGNETIC,     // Magnetic connection
    CONNECTION_TYPE_FLEXIBLE      // Flexible joint
} ConnectionType;

// Connection definition
typedef struct {
    u32 connection_id;
    ConnectionType type;
    Vec3 position;               // Connection point
    Vec3 axis;                   // Rotation/slide axis
    f32 strength;                // Connection strength
    bool locked;                  // Whether connection is locked
    u32 connected_to_id;          // ID of connected component
} Connection;

// Modular asset (composed of components)
typedef struct {
    u32 asset_id;
    const char* name;
    
    // Component tree (hierarchical)
    struct AssetComponent* root_component;
    u32 component_count;
    
    // Connections
    Connection* connections;
    u32 connection_count;
    u32 connection_capacity;
    
    // Bounds
    Vec3 bounds_min;
    Vec3 bounds_max;
    Vec3 center;
    
    // Rendering
    bool needs_rebuild;          // Mesh needs rebuilding
    struct Mesh* combined_mesh;  // Combined mesh for rendering
    struct Mesh* combined_mesh_2_5d; // 2.5D version
    
    // Metadata
    f32 total_complexity;
    u32 total_triangles;
} ModularAsset;

// Component attachment info
typedef struct {
    u32 base_component_id;
    u32 attachment_component_id;
    u32 base_snap_id;
    u32 attachment_snap_id;
    Mat4 transform;              // Transform for attachment
} AttachmentInfo;

// Modular asset builder
typedef struct {
    ModularAsset* asset;
    bool building;
} ModularAssetBuilder;

// Lifecycle
ModularAsset* modular_asset_create(const char* name);
void modular_asset_destroy(ModularAsset* asset);

// Component management
bool modular_asset_add_component(ModularAsset* asset, struct AssetComponent* component);
bool modular_asset_remove_component(ModularAsset* asset, u32 component_id);
struct AssetComponent* modular_asset_get_component(ModularAsset* asset, u32 component_id);

// Connection management
bool modular_asset_connect(ModularAsset* asset, 
                          u32 component1_id, u32 snap1_id,
                          u32 component2_id, u32 snap2_id);
bool modular_asset_disconnect(ModularAsset* asset, u32 connection_id);
bool modular_asset_can_connect(ModularAsset* asset,
                               u32 component1_id, u32 snap1_id,
                               u32 component2_id, u32 snap2_id);

// Mesh generation
bool modular_asset_rebuild_mesh(ModularAsset* asset);
bool modular_asset_rebuild_mesh_2_5d(ModularAsset* asset);

// Validation
bool modular_asset_validate(ModularAsset* asset);
bool modular_asset_check_collisions(ModularAsset* asset);

// Export/Import
bool modular_asset_export(ModularAsset* asset, const char* path);
ModularAsset* modular_asset_import(const char* path);

// Builder API
ModularAssetBuilder* modular_builder_create(const char* name);
void modular_builder_add_component(ModularAssetBuilder* builder, struct AssetComponent* component);
void modular_builder_connect(ModularAssetBuilder* builder, 
                             u32 comp1_id, u32 snap1_id,
                             u32 comp2_id, u32 snap2_id);
ModularAsset* modular_builder_finish(ModularAssetBuilder* builder);
void modular_builder_destroy(ModularAssetBuilder* builder);

#endif // MODULAR_ASSETS_H

