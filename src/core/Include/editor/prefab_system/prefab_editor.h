#ifndef PREFAB_EDITOR_H
#define PREFAB_EDITOR_H

#include "core/types.h"
#include "math/vec3.h"
#include "math/quat.h"

#ifdef __cplusplus
extern "C" {
#endif

// Property override for prefab instances
typedef struct {
    char property_name[64];
    void* value_data;
    u32 value_size;
} PrefabPropertyOverride;

// Prefab variant definition
typedef struct {
    char variant_name[64];
    PrefabPropertyOverride* overrides;
    u32 override_count;
} PrefabVariant;

// Prefab definition
typedef struct {
    char name[128];
    char file_path[256];
    u32 entity_id; // Template entity
    
    // Serialized data
    void* serialized_data;
    u32 data_size;
    
    // Variants
    PrefabVariant* variants;
    u32 variant_count;
    
    // Metadata
    bool is_dirty;
    u64 last_modified;
} Prefab;

// Prefab instance tracking
typedef struct {
    u32 instance_entity_id;
    Prefab* source_prefab;
    PrefabPropertyOverride* overrides;
    u32 override_count;
    char active_variant[64];
} PrefabInstance;

// Prefab editor context
typedef struct {
    Prefab* prefabs;
    u32 prefab_count;
    u32 prefab_capacity;
    
    PrefabInstance* instances;
    u32 instance_count;
    u32 instance_capacity;
    
    char prefab_directory[256];
} PrefabEditor;

// Core API
void prefab_editor_init(PrefabEditor* editor, const char* prefab_dir);
void prefab_editor_shutdown(PrefabEditor* editor);

// Prefab creation and management
Prefab* prefab_create_from_entity(PrefabEditor* editor, u32 entity_id, const char* name);
void prefab_save(PrefabEditor* editor, Prefab* prefab);
Prefab* prefab_load(PrefabEditor* editor, const char* file_path);
void prefab_delete(PrefabEditor* editor, Prefab* prefab);

// Instance management
PrefabInstance* prefab_instantiate(PrefabEditor* editor, Prefab* prefab, Vec3 position, Quat rotation);
void prefab_destroy_instance(PrefabEditor* editor, PrefabInstance* instance);
void prefab_apply_to_instance(PrefabEditor* editor, PrefabInstance* instance);

// Property overrides
void prefab_set_override(PrefabInstance* instance, const char* property_name, void* value, u32 value_size);
void prefab_clear_override(PrefabInstance* instance, const char* property_name);
void prefab_revert_to_prefab(PrefabEditor* editor, PrefabInstance* instance);

// Variants
PrefabVariant* prefab_create_variant(Prefab* prefab, const char* variant_name);
void prefab_apply_variant(PrefabEditor* editor, PrefabInstance* instance, const char* variant_name);

// Utilities
void prefab_update_all_instances(PrefabEditor* editor, Prefab* prefab);
bool prefab_validate(Prefab* prefab);

#ifdef __cplusplus
}
#endif

#endif // PREFAB_EDITOR_H
