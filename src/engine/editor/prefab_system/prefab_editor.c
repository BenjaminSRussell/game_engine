#include "editor/prefab_system/prefab_editor.h"
#include "core/memory.h"
#include "core/logger.h"
#include "core/json.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ecs/ecs.h"
#include "ecs/components/transform.h"

void prefab_editor_init(PrefabEditor* editor, const char* prefab_dir) {
    if (!editor) return;
    memset(editor, 0, sizeof(PrefabEditor));
    
    editor->prefab_capacity = 128;
    editor->prefabs = (Prefab*)calloc(editor->prefab_capacity, sizeof(Prefab));
    
    editor->instance_capacity = 512;
    editor->instances = (PrefabInstance*)calloc(editor->instance_capacity, sizeof(PrefabInstance));
    
    if (prefab_dir) {
        strncpy(editor->prefab_directory, prefab_dir, sizeof(editor->prefab_directory) - 1);
    } else {
        strcpy(editor->prefab_directory, "assets/prefabs/");
    }
    
    LOG_INFO("Prefab Editor initialized (dir: %s)", editor->prefab_directory);
}

void prefab_editor_shutdown(PrefabEditor* editor) {
    if (!editor) return;
    
    // Free prefabs
    for (u32 i = 0; i < editor->prefab_count; i++) {
        Prefab* prefab = &editor->prefabs[i];
        if (prefab->serialized_data) free(prefab->serialized_data);
        if (prefab->variants) {
            for (u32 j = 0; j < prefab->variant_count; j++) {
                if (prefab->variants[j].overrides) free(prefab->variants[j].overrides);
            }
            free(prefab->variants);
        }
    }
    free(editor->prefabs);
    
    // Free instances
    for (u32 i = 0; i < editor->instance_count; i++) {
        PrefabInstance* inst = &editor->instances[i];
        if (inst->overrides) free(inst->overrides);
    }
    free(editor->instances);
    
    memset(editor, 0, sizeof(PrefabEditor));
}

Prefab* prefab_create_from_entity(PrefabEditor* editor, u32 entity_id, const char* name) {
    if (!editor || editor->prefab_count >= editor->prefab_capacity) return NULL;
    
    Prefab* prefab = &editor->prefabs[editor->prefab_count++];
    memset(prefab, 0, sizeof(Prefab));
    
    strncpy(prefab->name, name, sizeof(prefab->name) - 1);
    snprintf(prefab->file_path, sizeof(prefab->file_path), "%s%s.prefab", 
             editor->prefab_directory, name);
    
    prefab->entity_id = entity_id;
    prefab->is_dirty = true;
    prefab->last_modified = (u64)time(NULL);
    
    // Serialize entity data
    // Use ECS system to serialize current entity state to JSON string
    char* json_data = ecs_entity_serialize(editor->ecs_world, entity_id);
    if (json_data) {
        prefab->data_size = strlen(json_data) + 1;
        prefab->serialized_data = malloc(prefab->data_size);
        strcpy(prefab->serialized_data, json_data);
        free(json_data); // Free the temp string from ECS
    } else {
        // Fallback or empty entity
        prefab->data_size = 0;
        prefab->serialized_data = NULL;
    }
    
    LOG_INFO("Created prefab '%s' from entity %u (Data size: %zu)", name, entity_id, prefab->data_size);
    return prefab;
}

void prefab_save(PrefabEditor* editor, Prefab* prefab) {
    if (!editor || !prefab) return;
    
    // Serialize to JSON and write to file
    FILE* file = fopen(prefab->file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to save prefab to %s", prefab->file_path);
        return;
    }
    
    // Simple JSON output (real impl would use json.h properly)
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", prefab->name);
    fprintf(file, "  \"entity_id\": %u,\n", prefab->entity_id);
    fprintf(file, "  \"variant_count\": %u,\n", prefab->variant_count);
    
    // Write serialized ECS data as a nested object or string
    if (prefab->serialized_data) {
        fprintf(file, "  \"ecs_data\": %s\n", (char*)prefab->serialized_data);
    } else {
        fprintf(file, "  \"ecs_data\": null\n");
    }
    
    fprintf(file, "}\n");
    
    fclose(file);
    
    prefab->is_dirty = false;
    prefab->last_modified = (u64)time(NULL);
    
    LOG_INFO("Saved prefab '%s' to %s", prefab->name, prefab->file_path);
}

Prefab* prefab_load(PrefabEditor* editor, const char* file_path) {
    if (!editor || editor->prefab_count >= editor->prefab_capacity) return NULL;
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        LOG_ERROR("Failed to load prefab from %s", file_path);
        return NULL;
    }
    
    Prefab* prefab = &editor->prefabs[editor->prefab_count++];
    memset(prefab, 0, sizeof(Prefab));
    
    // Parse JSON (simplified)
    char line[1024];
    char json_buffer[4096] = {0}; // Temp buffer for accumulating JSON
    bool reading_ecs_data = false;
    
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "\"name\"")) {
            sscanf(line, "  \"name\": \"%127[^\"]\"", prefab->name);
        } else if (strstr(line, "\"entity_id\"")) {
            sscanf(line, "  \"entity_id\": %u", &prefab->entity_id);
        } else if (strstr(line, "\"ecs_data\"")) {
            // Very simplified JSON extraction for demo purposes
            // In reality, use a proper JSON parser
             char* start = strchr(line, ':');
             if (start) {
                 strcpy(json_buffer, start + 1);
                 // Trim trailing brace/comma
             }
        }
    }
    
    if (strlen(json_buffer) > 0) {
        prefab->data_size = strlen(json_buffer) + 1;
        prefab->serialized_data = malloc(prefab->data_size);
        strcpy(prefab->serialized_data, json_buffer);
    }
    
    fclose(file);
    
    strncpy(prefab->file_path, file_path, sizeof(prefab->file_path) - 1);
    
    LOG_INFO("Loaded prefab '%s' from %s", prefab->name, file_path);
    return prefab;
}

void prefab_delete(PrefabEditor* editor, Prefab* prefab) {
    if (!editor || !prefab) return;
    
    // Remove from array (swap with last)
    for (u32 i = 0; i < editor->prefab_count; i++) {
        if (&editor->prefabs[i] == prefab) {
            if (prefab->serialized_data) free(prefab->serialized_data);
            if (prefab->variants) {
                for (u32 j = 0; j < prefab->variant_count; j++) {
                    if (prefab->variants[j].overrides) free(prefab->variants[j].overrides);
                }
                free(prefab->variants);
            }
            
            editor->prefabs[i] = editor->prefabs[editor->prefab_count - 1];
            editor->prefab_count--;
            
            LOG_INFO("Deleted prefab");
            return;
        }
    }
}

PrefabInstance* prefab_instantiate(PrefabEditor* editor, Prefab* prefab, Vec3 position, Quat rotation) {
    if (!editor || !prefab || editor->instance_count >= editor->instance_capacity) return NULL;
    
    PrefabInstance* instance = &editor->instances[editor->instance_count++];
    memset(instance, 0, sizeof(PrefabInstance));
    
    // Create new entity from prefab template
    // Actually instantiate entity in ECS
    EntityID new_entity = ecs_entity_create(editor->ecs_world);
    
    // Apply initial transform
    TransformComponent transform;
    transform.position = position;
    transform.rotation = rotation;
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    ecs_component_set(editor->ecs_world, new_entity, COMPONENT_TRANSFORM, &transform);
    
    // If we have serialized data, deserialize it onto the entity
    if (prefab->serialized_data) {
        ecs_entity_deserialize(editor->ecs_world, new_entity, (const char*)prefab->serialized_data);
    }

    instance->instance_entity_id = new_entity;
    instance->source_prefab = prefab;
    
    LOG_INFO("Instantiated prefab '%s' as entity %u", prefab->name, instance->instance_entity_id);
    return instance;
}

void prefab_destroy_instance(PrefabEditor* editor, PrefabInstance* instance) {
    if (!editor || !instance) return;
    
    // Destroy ECS entity
    if (instance->instance_entity_id != 0) {
        ecs_entity_destroy(editor->ecs_world, instance->instance_entity_id);
    }
    
    for (u32 i = 0; i < editor->instance_count; i++) {
        if (&editor->instances[i] == instance) {
            if (instance->overrides) free(instance->overrides);
            
            editor->instances[i] = editor->instances[editor->instance_count - 1];
            editor->instance_count--;
            return;
        }
    }
}

void prefab_apply_to_instance(PrefabEditor* editor, PrefabInstance* instance) {
    if (!editor || !instance || !instance->source_prefab) return;
    
    // Apply prefab data to instance entity
    // Reset entity to base prefab state first
    if (instance->source_prefab->serialized_data) {
         ecs_entity_deserialize(editor->ecs_world, instance->instance_entity_id, 
                               (const char*)instance->source_prefab->serialized_data);
    }
    
    // Apply overrides
    for (u32 i = 0; i < instance->override_count; i++) {
        PrefabPropertyOverride* override = &instance->overrides[i];
        
        // Apply property override to entity
        // Parse property name to find component and field (e.g., "Transform.position")
        char component_name[64];
        char field_name[64];
        
        // Simple parsing (stub logic for demonstration)
        if (sscanf(override->property_name, "%[^.].%s", component_name, field_name) == 2) {
             ecs_entity_set_property(editor->ecs_world, instance->instance_entity_id, 
                                   component_name, field_name, 
                                   override->value_data);
        }
        
        LOG_INFO("Applying override '%s' to instance %u", 
                 override->property_name, instance->instance_entity_id);
    }
}

void prefab_set_override(PrefabInstance* instance, const char* property_name, void* value, u32 value_size) {
    if (!instance) return;
    
    // Check if override already exists
    for (u32 i = 0; i < instance->override_count; i++) {
        if (strcmp(instance->overrides[i].property_name, property_name) == 0) {
            // Update existing override
            if (instance->overrides[i].value_data) free(instance->overrides[i].value_data);
            instance->overrides[i].value_data = malloc(value_size);
            memcpy(instance->overrides[i].value_data, value, value_size);
            instance->overrides[i].value_size = value_size;
            return;
        }
    }
    
    // Add new override
    instance->overrides = (PrefabPropertyOverride*)realloc(
        instance->overrides, 
        (instance->override_count + 1) * sizeof(PrefabPropertyOverride)
    );
    
    PrefabPropertyOverride* override = &instance->overrides[instance->override_count++];
    strncpy(override->property_name, property_name, sizeof(override->property_name) - 1);
    override->value_data = malloc(value_size);
    memcpy(override->value_data, value, value_size);
    override->value_size = value_size;
}

void prefab_clear_override(PrefabInstance* instance, const char* property_name) {
    if (!instance) return;
    
    for (u32 i = 0; i < instance->override_count; i++) {
        if (strcmp(instance->overrides[i].property_name, property_name) == 0) {
            if (instance->overrides[i].value_data) free(instance->overrides[i].value_data);
            
            // Swap with last
            instance->overrides[i] = instance->overrides[instance->override_count - 1];
            instance->override_count--;
            return;
        }
    }
}

void prefab_revert_to_prefab(PrefabEditor* editor, PrefabInstance* instance) {
    if (!editor || !instance) return;
    
    // Clear all overrides
    for (u32 i = 0; i < instance->override_count; i++) {
        if (instance->overrides[i].value_data) free(instance->overrides[i].value_data);
    }
    free(instance->overrides);
    instance->overrides = NULL;
    instance->override_count = 0;
    
    // Reapply prefab data
    prefab_apply_to_instance(editor, instance);
    
    LOG_INFO("Reverted instance %u to prefab '%s'", 
             instance->instance_entity_id, instance->source_prefab->name);
}

PrefabVariant* prefab_create_variant(Prefab* prefab, const char* variant_name) {
    if (!prefab) return NULL;
    
    prefab->variants = (PrefabVariant*)realloc(
        prefab->variants, 
        (prefab->variant_count + 1) * sizeof(PrefabVariant)
    );
    
    PrefabVariant* variant = &prefab->variants[prefab->variant_count++];
    memset(variant, 0, sizeof(PrefabVariant));
    
    strncpy(variant->variant_name, variant_name, sizeof(variant->variant_name) - 1);
    
    prefab->is_dirty = true;
    
    LOG_INFO("Created variant '%s' for prefab '%s'", variant_name, prefab->name);
    return variant;
}

void prefab_apply_variant(PrefabEditor* editor, PrefabInstance* instance, const char* variant_name) {
    if (!editor || !instance || !instance->source_prefab) return;
    
    // Find variant
    Prefab* prefab = instance->source_prefab;
    for (u32 i = 0; i < prefab->variant_count; i++) {
        if (strcmp(prefab->variants[i].variant_name, variant_name) == 0) {
            PrefabVariant* variant = &prefab->variants[i];
            
            // Apply variant overrides
            for (u32 j = 0; j < variant->override_count; j++) {
                PrefabPropertyOverride* override = &variant->overrides[j];
                prefab_set_override(instance, override->property_name, 
                                  override->value_data, override->value_size);
            }
            
            strncpy(instance->active_variant, variant_name, sizeof(instance->active_variant) - 1);
            
            LOG_INFO("Applied variant '%s' to instance %u", variant_name, instance->instance_entity_id);
            return;
        }
    }
    
    LOG_WARN("Variant '%s' not found in prefab '%s'", variant_name, prefab->name);
}

void prefab_update_all_instances(PrefabEditor* editor, Prefab* prefab) {
    if (!editor || !prefab) return;
    
    u32 updated_count = 0;
    for (u32 i = 0; i < editor->instance_count; i++) {
        PrefabInstance* instance = &editor->instances[i];
        if (instance->source_prefab == prefab) {
            prefab_apply_to_instance(editor, instance);
            updated_count++;
        }
    }
    
    LOG_INFO("Updated %u instances of prefab '%s'", updated_count, prefab->name);
}

bool prefab_validate(Prefab* prefab) {
    if (!prefab) return false;
    if (strlen(prefab->name) == 0) return false;
    if (strlen(prefab->file_path) == 0) return false;
    return true;
}
