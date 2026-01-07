#ifndef EDITOR_PREFAB_PREFAB_SYSTEM_H
#define EDITOR_PREFAB_PREFAB_SYSTEM_H

#include "core/core.h"
#include "include/ecs/components/transform.h"
#include "core/serialization/binary_serializer.h"
#include "include/rendering/material.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                           PREFAB SYSTEM - AGENT_EDITOR_2
// =================================================================================================

// Maximum number of entities per prefab
#define MAX_PREFAB_ENTITIES 1024
#define MAX_PREFAB_DEPTH 32
#define MAX_OVERRIDE_PROPERTIES 256

// Prefab asset structure
typedef struct {
    char name[256];
    char description[512];
    char file_path[512];
    
    // Entity hierarchy data
    u32 entity_count;
    u32 entity_capacity;
    u32* entity_ids;           // Entity IDs in hierarchy
    u32* parent_ids;           // Parent entity IDs (-1 for root)
    Transform* transforms;     // Local transforms
    char** entity_names;       // Entity names
    
    // Component data (serialized)
    u8* component_data;        // Serialized component blob
    size_t component_data_size;
    
    // Metadata
    u64 created_time;
    u64 modified_time;
    u32 version;
    
    bool is_dirty;
} PrefabAsset;

// Prefab instance (runtime)
typedef struct {
    u32 instance_id;
    u32 prefab_asset_id;
    char instance_name[256];
    
    // Runtime entity hierarchy
    u32* entity_ids;           // Runtime entity IDs
    u32 entity_count;
    
    // Property overrides
    u32 override_count;
    struct {
        u32 entity_id;
        char property_name[128];
        char override_value[512];  // Serialized value
        bool is_active;
    } overrides[MAX_OVERRIDE_PROPERTIES];
    
    // Linkage to prefab asset
    PrefabAsset* source_prefab;
    bool is_linked;            // True if linked to prefab asset
    bool overrides_applied;    // True if overrides have been applied
    
    // Instance state
    Transform root_transform;
    bool is_active;
} PrefabInstance;

// Prefab system
typedef struct {
    // Asset management
    PrefabAsset* assets;
    u32 asset_count;
    u32 asset_capacity;
    
    // Instance management
    PrefabInstance* instances;
    u32 instance_count;
    u32 instance_capacity;
    
    // Runtime state
    u32 next_instance_id;
    u32 next_asset_id;
    
    // Auto-update settings
    bool auto_update_instances;
    bool track_changes;
    
    // Statistics
    u32 total_entities_in_prefabs;
    u32 active_instances;
} PrefabSystem;

// =================================================================================================
//                                    CORE API FUNCTIONS
// =================================================================================================

// System management
PrefabSystem* prefab_system_create();
void prefab_system_destroy(PrefabSystem* system);
void prefab_system_update(PrefabSystem* system, f32 delta_time);

// Asset management
PrefabAsset* prefab_create_asset(PrefabSystem* system, const char* name);
void prefab_destroy_asset(PrefabSystem* system, u32 asset_id);
PrefabAsset* prefab_get_asset(PrefabSystem* system, u32 asset_id);
PrefabAsset* prefab_load_asset_from_file(PrefabSystem* system, const char* file_path);
bool prefab_save_asset_to_file(PrefabSystem* system, u32 asset_id, const char* file_path);

// Asset building
bool prefab_build_from_entities(PrefabSystem* system, u32 asset_id, 
                               const u32* entity_ids, u32 entity_count);
bool prefab_add_entity_to_asset(PrefabSystem* system, u32 asset_id, u32 entity_id, u32 parent_id);
bool prefab_remove_entity_from_asset(PrefabSystem* system, u32 asset_id, u32 entity_id);

// Instance management
PrefabInstance* prefab_create_instance(PrefabSystem* system, u32 asset_id, const char* instance_name);
void prefab_destroy_instance(PrefabSystem* system, u32 instance_id);
PrefabInstance* prefab_get_instance(PrefabSystem* system, u32 instance_id);
void prefab_set_instance_transform(PrefabSystem* system, u32 instance_id, Transform transform);

// Prefab linkage
void prefab_link_instance_to_asset(PrefabSystem* system, u32 instance_id, u32 asset_id);
void prefab_unlink_instance(PrefabSystem* system, u32 instance_id);
bool prefab_is_instance_linked(PrefabInstance* instance);

// Override system
void prefab_add_property_override(PrefabSystem* system, u32 instance_id, u32 entity_id,
                                 const char* property_name, const char* value);
void prefab_remove_property_override(PrefabSystem* system, u32 instance_id, u32 entity_id,
                                    const char* property_name);
void prefab_revert_property_to_prefab(PrefabSystem* system, u32 instance_id, u32 entity_id,
                                      const char* property_name);
void prefab_revert_all_properties(PrefabSystem* system, u32 instance_id);

// Propagation system
void prefab_apply_changes_to_prefab(PrefabSystem* system, u32 instance_id, u32 entity_id,
                                    const char* property_name);
void prefab_propagate_prefab_changes(PrefabSystem* system, u32 asset_id);
void prefab_update_all_instances(PrefabSystem* system, u32 asset_id);

// Nested prefabs
bool prefab_add_nested_prefab(PrefabSystem* system, u32 parent_asset_id, u32 nested_asset_id,
                             u32 parent_entity_id);
void prefab_get_nested_prefabs(PrefabSystem* system, u32 asset_id, u32* nested_ids, u32* count);

// Prefab detachment
void prefab_detach_instance(PrefabSystem* system, u32 instance_id);
bool prefab_can_detach_instance(PrefabInstance* instance);

// =================================================================================================
//                                SERIALIZATION
// =================================================================================================

// Asset serialization
bool prefab_serialize_asset_to_binary(PrefabAsset* asset, const char* file_path);
bool prefab_deserialize_asset_from_binary(const char* file_path, PrefabAsset* out_asset);

// Instance serialization
bool prefab_serialize_instance_to_binary(PrefabInstance* instance, const char* file_path);
bool prefab_deserialize_instance_from_binary(const char* file_path, PrefabInstance* out_instance);

// =================================================================================================
//                                VALIDATION
// =================================================================================================

bool prefab_validate_asset(PrefabAsset* asset);
bool prefab_validate_instance(PrefabInstance* instance);
bool prefab_validate_hierarchy(PrefabAsset* asset, char* error_buffer, size_t error_buffer_size);

// =================================================================================================
//                                UTILITIES
// =================================================================================================

// Search and filtering
u32 prefab_find_assets_by_name(PrefabSystem* system, const char* name_pattern, 
                              u32* out_asset_ids, u32 max_results);
u32 prefab_find_instances_by_name(PrefabSystem* system, const char* name_pattern,
                                  u32* out_instance_ids, u32 max_results);

// Statistics
void prefab_get_system_stats(PrefabSystem* system, u32* asset_count, u32* instance_count,
                            u32* total_entities, u32* active_instances);
void prefab_get_asset_stats(PrefabAsset* asset, u32* entity_count, size_t* component_data_size);

// Debug utilities
void prefab_print_asset_info(PrefabAsset* asset);
void prefab_print_instance_info(PrefabInstance* instance);
void prefab_print_system_stats(PrefabSystem* system);

// =================================================================================================
//                            EDITOR INTEGRATION
// =================================================================================================

// UI helpers
const char* prefab_get_property_display_name(const char* property_name);
const char* prefab_get_property_type(const char* property_name);
bool prefab_is_property_overridden(PrefabInstance* instance, u32 entity_id, const char* property_name);

// Selection helpers
u32 prefab_select_entities_in_instance(PrefabInstance* instance, u32* out_entity_ids, u32 max_count);
u32 prefab_select_root_entities(PrefabAsset* asset, u32* out_entity_ids, u32 max_count);

// =================================================================================================
//                                 CALLBACKS
// =================================================================================================

// Event callbacks
typedef void (*PrefabAssetChangedCallback)(u32 asset_id, void* user_data);
typedef void (*PrefabInstanceCreatedCallback)(u32 instance_id, void* user_data);
typedef void (*PrefabInstanceDestroyedCallback)(u32 instance_id, void* user_data);

// Callback registration
void prefab_set_asset_changed_callback(PrefabSystem* system, PrefabAssetChangedCallback callback, void* user_data);
void prefab_set_instance_created_callback(PrefabSystem* system, PrefabInstanceCreatedCallback callback, void* user_data);
void prefab_set_instance_destroyed_callback(PrefabSystem* system, PrefabInstanceDestroyedCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_PREFAB_PREFAB_SYSTEM_H
