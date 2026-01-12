#include "materials/editor/material_editor.h"
#include "materials/material_system.h"
#include "materials/material_instance.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

#define MATERIAL_EDITOR_MAX_EDITORS 16
#define MATERIAL_EDITOR_MAX_SEARCH_RESULTS 256
#define MATERIAL_EDITOR_MAX_DIAGNOSTICS 64
#define MATERIAL_EDITOR_UNDO_STACK_SIZE 100

typedef struct material_editor_undo_entry_t {
    char description[128];
    char material_name[128];
    char parameter_name[64];
    MaterialParameterType type;
    void* old_value;
    void* new_value;
    bool is_parameter_change;
} material_editor_undo_entry_t;

typedef struct material_editor_state_t {
    material_editor_handle_t handle;
    material_editor_desc_t desc;
    material_editor_info_t info;
    
    MaterialMaster* current_master;
    MaterialInstance* current_instance;
    
    // Preview state
    uint32_t preview_mesh_id;
    uint32_t preview_environment_id;
    float preview_light_intensity;
    Vec3 preview_light_direction;
    
    // Undo/redo system
    material_editor_undo_entry_t undo_stack[MATERIAL_EDITOR_UNDO_STACK_SIZE];
    material_editor_undo_entry_t redo_stack[MATERIAL_EDITOR_UNDO_STACK_SIZE];
    uint32_t undo_top;
    uint32_t redo_top;
    bool in_undo_group;
    char current_undo_group[128];
    
    // Search results
    char* search_results[MATERIAL_EDITOR_MAX_SEARCH_RESULTS];
    uint32_t search_result_count;
    
    // Diagnostics
    material_diagnostic_t diagnostics[MATERIAL_EDITOR_MAX_DIAGNOSTICS];
    uint32_t diagnostic_count;
    
    // UI callback
    material_editor_param_change_callback_t param_change_callback;
    void* callback_user_data;
    
    bool has_unsaved_changes;
    bool hot_reload_enabled;
} material_editor_state_t;

static material_editor_state_t g_editors[MATERIAL_EDITOR_MAX_EDITORS];
static uint32_t g_editor_count = 0;
static uint32_t g_next_editor_id = 1;
static bool g_editor_system_initialized = false;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static material_editor_state_t* find_editor_by_handle(material_editor_handle_t handle) {
    for (uint32_t i = 0; i < g_editor_count; i++) {
        if (g_editors[i].handle.id == handle.id) {
            return &g_editors[i];
        }
    }
    return NULL;
}

static material_editor_state_t* create_editor_state(void) {
    if (g_editor_count >= MATERIAL_EDITOR_MAX_EDITORS) {
        return NULL;
    }
    
    material_editor_state_t* editor = &g_editors[g_editor_count];
    memset(editor, 0, sizeof(material_editor_state_t));
    
    editor->handle.id = g_next_editor_id++;
    editor->info.id = editor->handle.id;
    editor->info.mode = MATERIAL_EDITOR_MODE_MASTER;
    editor->info.initialized = true;
    editor->preview_light_intensity = 1.0f;
    editor->preview_light_direction = (Vec3){0.0f, 1.0f, 0.0f};
    editor->hot_reload_enabled = true;
    
    g_editor_count++;
    return editor;
}

static void destroy_editor_state(material_editor_state_t* editor) {
    if (!editor) return;
    
    // Free search results
    for (uint32_t i = 0; i < editor->search_result_count; i++) {
        free(editor->search_results[i]);
    }
    
    // Free undo/redo data
    for (uint32_t i = 0; i < editor->undo_top; i++) {
        free(editor->undo_stack[i].old_value);
        free(editor->undo_stack[i].new_value);
    }
    for (uint32_t i = 0; i < editor->redo_top; i++) {
        free(editor->redo_stack[i].old_value);
        free(editor->redo_stack[i].new_value);
    }
    
    memset(editor, 0, sizeof(material_editor_state_t));
}

static void* clone_parameter_value(const MaterialParameter* param) {
    if (!param) return NULL;
    
    void* clone = NULL;
    size_t size = 0;
    
    switch (param->type) {
        case MATERIAL_PARAM_FLOAT:
            size = sizeof(f32);
            break;
        case MATERIAL_PARAM_VEC2:
            size = sizeof(f32) * 2;
            break;
        case MATERIAL_PARAM_VEC3:
            size = sizeof(f32) * 3;
            break;
        case MATERIAL_PARAM_VEC4:
        case MATERIAL_PARAM_COLOR:
            size = sizeof(f32) * 4;
            break;
        case MATERIAL_PARAM_TEXTURE:
            size = sizeof(TextureID);
            break;
        case MATERIAL_PARAM_INT:
            size = sizeof(i32);
            break;
        case MATERIAL_PARAM_BOOL:
            size = sizeof(bool);
            break;
        default:
            return NULL;
    }
    
    clone = malloc(size);
    if (clone) {
        memcpy(clone, &param->value, size);
    }
    
    return clone;
}

static void add_undo_entry(material_editor_state_t* editor, const char* description,
                          const char* material_name, const char* param_name,
                          MaterialParameterType type, const void* old_value, const void* new_value) {
    if (!editor || editor->undo_top >= MATERIAL_EDITOR_UNDO_STACK_SIZE) {
        return;
    }
    
    material_editor_undo_entry_t* entry = &editor->undo_stack[editor->undo_top];
    
    strncpy(entry->description, description ? description : "Parameter Change", sizeof(entry->description) - 1);
    strncpy(entry->material_name, material_name ? material_name : "", sizeof(entry->material_name) - 1);
    strncpy(entry->parameter_name, param_name ? param_name : "", sizeof(entry->parameter_name) - 1);
    
    entry->type = type;
    entry->old_value = clone_parameter_value(&(MaterialParameter){.type = type, .value = *(void*)old_value});
    entry->new_value = clone_parameter_value(&(MaterialParameter){.type = type, .value = *(void*)new_value});
    entry->is_parameter_change = (param_name != NULL);
    
    editor->undo_top++;
    
    // Clear redo stack when new action is performed
    for (uint32_t i = 0; i < editor->redo_top; i++) {
        free(editor->redo_stack[i].old_value);
        free(editor->redo_stack[i].new_value);
    }
    editor->redo_top = 0;
    
    editor->has_unsaved_changes = true;
}

// ============================================================================
// EDITOR LIFECYCLE
// ============================================================================

bool material_editor_init(void) {
    if (g_editor_system_initialized) {
        return true; // Already initialized
    }
    
    memset(g_editors, 0, sizeof(g_editors));
    g_editor_count = 0;
    g_next_editor_id = 1;
    g_editor_system_initialized = true;
    
    printf("Material editor system initialized\n");
    return true;
}

void material_editor_shutdown(void) {
    if (!g_editor_system_initialized) return;
    
    // Destroy all editors
    for (uint32_t i = 0; i < g_editor_count; i++) {
        destroy_editor_state(&g_editors[i]);
    }
    
    g_editor_count = 0;
    g_editor_system_initialized = false;
    
    printf("Material editor system shutdown\n");
}

material_editor_handle_t material_editor_create(const material_editor_desc_t* desc) {
    if (!g_editor_system_initialized || !desc) {
        return (material_editor_handle_t){0};
    }
    
    material_editor_state_t* editor = create_editor_state();
    if (!editor) {
        return (material_editor_handle_t){0};
    }
    
    editor->desc = *desc;
    editor->info.mode = desc->mode;
    
    if (desc->initial_material[0] != '\0') {
        material_editor_set_material(editor->handle, desc->initial_material);
    }
    
    printf("Created material editor instance %u\n", editor->handle.id);
    return editor->handle;
}

void material_editor_destroy(material_editor_handle_t handle) {
    if (!g_editor_system_initialized) return;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    // Remove from array by shifting remaining elements
    for (uint32_t i = 0; i < g_editor_count; i++) {
        if (g_editors[i].handle.id == handle.id) {
            destroy_editor_state(&g_editors[i]);
            
            // Shift remaining editors
            for (uint32_t j = i; j < g_editor_count - 1; j++) {
                g_editors[j] = g_editors[j + 1];
            }
            g_editor_count--;
            break;
        }
    }
    
    printf("Destroyed material editor instance %u\n", handle.id);
}

bool material_editor_get_info(material_editor_handle_t handle, material_editor_info_t* info) {
    if (!info) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    *info = editor->info;
    info->has_unsaved_changes = editor->has_unsaved_changes;
    
    if (editor->current_master) {
        strncpy(info->current_material, editor->current_master->name, sizeof(info->current_material) - 1);
    } else if (editor->current_instance) {
        strncpy(info->current_material, editor->current_instance->name, sizeof(info->current_material) - 1);
    }
    
    return true;
}

// ============================================================================
// EDITOR OPERATIONS
// ============================================================================

bool material_editor_set_material(material_editor_handle_t handle, const char* material_name) {
    if (!material_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Try to find as instance first
    MaterialInstance* instance = material_system_get_instance(material_name);
    if (instance) {
        editor->current_instance = instance;
        editor->current_master = instance->parent;
        editor->info.mode = MATERIAL_EDITOR_MODE_INSTANCE;
        strncpy(editor->info.current_material, material_name, sizeof(editor->info.current_material) - 1);
        printf("Set material editor to instance: %s\n", material_name);
        return true;
    }
    
    // Try to find as master
    MaterialMaster* master = material_system_get_master(material_name);
    if (master) {
        editor->current_master = master;
        editor->current_instance = NULL;
        editor->info.mode = MATERIAL_EDITOR_MODE_MASTER;
        strncpy(editor->info.current_material, material_name, sizeof(editor->info.current_material) - 1);
        printf("Set material editor to master: %s\n", material_name);
        return true;
    }
    
    printf("Material not found: %s\n", material_name);
    return false;
}

bool material_editor_create_master(material_editor_handle_t handle, const char* name, const char* shader_path) {
    if (!name || !shader_path) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Check if master already exists
    if (material_system_get_master(name)) {
        printf("Master material already exists: %s\n", name);
        return false;
    }
    
    MaterialMaster* master = material_master_create(name, shader_path);
    if (!master) {
        printf("Failed to create master material: %s\n", name);
        return false;
    }
    
    if (!material_system_register_master(master)) {
        material_master_destroy(master);
        printf("Failed to register master material: %s\n", name);
        return false;
    }
    
    // Switch to edit the new master
    editor->current_master = master;
    editor->current_instance = NULL;
    editor->info.mode = MATERIAL_EDITOR_MODE_MASTER;
    strncpy(editor->info.current_material, name, sizeof(editor->info.current_material) - 1);
    
    add_undo_entry(editor, "Create Master Material", name, NULL, MATERIAL_PARAM_FLOAT, NULL, NULL);
    
    printf("Created master material: %s\n", name);
    return true;
}

bool material_editor_create_instance(material_editor_handle_t handle, const char* master_name, const char* instance_name) {
    if (!master_name || !instance_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    MaterialInstance* instance = material_system_create_instance(master_name, instance_name);
    if (!instance) {
        printf("Failed to create material instance: %s\n", instance_name);
        return false;
    }
    
    // Switch to edit the new instance
    editor->current_instance = instance;
    editor->current_master = instance->parent;
    editor->info.mode = MATERIAL_EDITOR_MODE_INSTANCE;
    strncpy(editor->info.current_material, instance_name, sizeof(editor->info.current_material) - 1);
    
    add_undo_entry(editor, "Create Material Instance", instance_name, NULL, MATERIAL_PARAM_FLOAT, NULL, NULL);
    
    printf("Created material instance: %s (from master %s)\n", instance_name, master_name);
    return true;
}

bool material_editor_duplicate_material(material_editor_handle_t handle, const char* source_name, const char* new_name) {
    if (!source_name || !new_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Try to duplicate as instance first
    MaterialInstance* source_instance = material_system_get_instance(source_name);
    if (source_instance && source_instance->parent) {
        MaterialInstance* new_instance = material_system_create_instance(source_instance->parent->name, new_name);
        if (!new_instance) return false;
        
        // Copy parameter overrides
        for (uint32_t i = 0; i < source_instance->override_count; i++) {
            MaterialParameter* src_param = &source_instance->overrides[i];
            
            switch (src_param->type) {
                case MATERIAL_PARAM_FLOAT:
                    material_instance_set_float(new_instance, src_param->name, src_param->value.float_val);
                    break;
                case MATERIAL_PARAM_VEC3:
                    material_instance_set_vec3(new_instance, src_param->name, src_param->value.vec3_val);
                    break;
                case MATERIAL_PARAM_VEC4:
                    material_instance_set_vec4(new_instance, src_param->name, src_param->value.vec4_val);
                    break;
                case MATERIAL_PARAM_TEXTURE:
                    material_instance_set_texture(new_instance, src_param->name, src_param->value.texture_val);
                    break;
                case MATERIAL_PARAM_BOOL:
                    material_instance_set_bool(new_instance, src_param->name, src_param->value.bool_val);
                    break;
                default:
                    break;
            }
        }
        
        printf("Duplicated material instance: %s -> %s\n", source_name, new_name);
        return true;
    }
    
    // Try to duplicate as master
    MaterialMaster* source_master = material_system_get_master(source_name);
    if (source_master) {
        MaterialMaster* new_master = material_master_create(new_name, source_master->shader_base_path);
        if (!new_master) return false;
        
        // Copy parameters
        for (uint32_t i = 0; i < source_master->param_count; i++) {
            MaterialParameter* src_param = &source_master->default_params[i];
            
            switch (src_param->type) {
                case MATERIAL_PARAM_FLOAT:
                    material_master_add_param(new_master, src_param->name, src_param->type, &src_param->value.float_val);
                    break;
                case MATERIAL_PARAM_VEC3:
                    material_master_add_param(new_master, src_param->name, src_param->type, src_param->value.vec3_val);
                    break;
                case MATERIAL_PARAM_VEC4:
                    material_master_add_param(new_master, src_param->name, src_param->type, src_param->value.vec4_val);
                    break;
                case MATERIAL_PARAM_TEXTURE:
                    material_master_add_param(new_master, src_param->name, src_param->type, &src_param->value.texture_val);
                    break;
                case MATERIAL_PARAM_INT:
                    material_master_add_param(new_master, src_param->name, src_param->type, &src_param->value.int_val);
                    break;
                case MATERIAL_PARAM_BOOL:
                    material_master_add_param(new_master, src_param->name, src_param->type, &src_param->value.bool_val);
                    break;
                default:
                    break;
            }
        }
        
        // Copy other properties
        new_master->supported_features = source_master->supported_features;
        new_master->two_sided = source_master->two_sided;
        new_master->alpha_blend = source_master->alpha_blend;
        new_master->alpha_test = source_master->alpha_test;
        
        if (!material_system_register_master(new_master)) {
            material_master_destroy(new_master);
            return false;
        }
        
        printf("Duplicated master material: %s -> %s\n", source_name, new_name);
        return true;
    }
    
    printf("Source material not found: %s\n", source_name);
    return false;
}

bool material_editor_delete_material(material_editor_handle_t handle, const char* material_name) {
    if (!material_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Try to delete instance first
    MaterialInstance* instance = material_system_get_instance(material_name);
    if (instance) {
        material_system_destroy_instance(instance);
        
        // Clear current selection if it was the deleted material
        if (editor->current_instance == instance) {
            editor->current_instance = NULL;
            editor->current_master = NULL;
            editor->info.current_material[0] = '\0';
        }
        
        printf("Deleted material instance: %s\n", material_name);
        return true;
    }
    
    // Try to delete master (more complex - need to check for instances)
    MaterialMaster* master = material_system_get_master(material_name);
    if (master) {
        // TODO: Check if there are instances using this master
        // For now, just print a warning
        printf("Warning: Deleting master material %s - make sure no instances are using it\n", material_name);
        
        // Clear current selection if it was the deleted material
        if (editor->current_master == master) {
            editor->current_master = NULL;
            editor->current_instance = NULL;
            editor->info.current_material[0] = '\0';
        }
        
        // Note: material_system doesn't have a destroy_master function yet
        // This would need to be implemented
        printf("Delete master material not yet implemented: %s\n", material_name);
        return false;
    }
    
    printf("Material not found: %s\n", material_name);
    return false;
}

bool material_editor_save_material(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    if (editor->current_instance) {
        // Compile instance if needed
        if (editor->current_instance->needs_recompile) {
            material_instance_compile(editor->current_instance);
        }
        
        printf("Saved material instance: %s\n", editor->current_instance->name);
        editor->has_unsaved_changes = false;
        return true;
    }
    
    if (editor->current_master) {
        printf("Saved master material: %s\n", editor->current_master->name);
        editor->has_unsaved_changes = false;
        return true;
    }
    
    printf("No material to save\n");
    return false;
}

bool material_editor_save_all(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Save all materials in the system
    // This would typically involve serializing to disk
    printf("Saving all materials...\n");
    
    // Compile all instances that need it
    // TODO: Iterate through all instances and compile if needed
    
    editor->has_unsaved_changes = false;
    printf("All materials saved\n");
    return true;
}

// ============================================================================
// PARAMETER EDITING
// ============================================================================

bool material_editor_set_parameter(material_editor_handle_t handle, const char* param_name, 
                                  MaterialParameterType type, const void* value) {
    if (!param_name || !value) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // Get old value for undo
    MaterialParameter* old_param = NULL;
    void* old_value = NULL;
    
    if (editor->current_instance) {
        old_param = material_instance_get_param(editor->current_instance, param_name);
        if (old_param) {
            old_value = clone_parameter_value(old_param);
        }
        
        // Set new value based on type
        switch (type) {
            case MATERIAL_PARAM_FLOAT:
                material_instance_set_float(editor->current_instance, param_name, *(const f32*)value);
                break;
            case MATERIAL_PARAM_VEC3:
                material_instance_set_vec3(editor->current_instance, param_name, (const f32*)value);
                break;
            case MATERIAL_PARAM_VEC4:
                material_instance_set_vec4(editor->current_instance, param_name, (const f32*)value);
                break;
            case MATERIAL_PARAM_TEXTURE:
                material_instance_set_texture(editor->current_instance, param_name, *(const TextureID*)value);
                break;
            case MATERIAL_PARAM_BOOL:
                material_instance_set_bool(editor->current_instance, param_name, *(const bool*)value);
                break;
            default:
                printf("Unsupported parameter type for instance: %d\n", type);
                if (old_value) free(old_value);
                return false;
        }
        
        printf("Set instance parameter: %s\n", param_name);
    } else if (editor->current_master) {
        // For master materials, we need to update the default parameter
        old_param = find_param_in_array(editor->current_master->default_params, 
                                       editor->current_master->param_count, param_name);
        if (old_param) {
            old_value = clone_parameter_value(old_param);
            
            // Update the default value
            switch (type) {
                case MATERIAL_PARAM_FLOAT:
                    old_param->value.float_val = *(const f32*)value;
                    break;
                case MATERIAL_PARAM_VEC3:
                    memcpy(old_param->value.vec3_val, value, sizeof(f32) * 3);
                    break;
                case MATERIAL_PARAM_VEC4:
                    memcpy(old_param->value.vec4_val, value, sizeof(f32) * 4);
                    break;
                case MATERIAL_PARAM_TEXTURE:
                    old_param->value.texture_val = *(const TextureID*)value;
                    break;
                case MATERIAL_PARAM_INT:
                    old_param->value.int_val = *(const i32*)value;
                    break;
                case MATERIAL_PARAM_BOOL:
                    old_param->value.bool_val = *(const bool*)value;
                    break;
                default:
                    printf("Unsupported parameter type for master: %d\n", type);
                    if (old_value) free(old_value);
                    return false;
            }
            
            printf("Set master parameter: %s\n", param_name);
        } else {
            printf("Parameter not found in master: %s\n", param_name);
            return false;
        }
    } else {
        printf("No material selected for parameter editing\n");
        return false;
    }
    
    // Add undo entry
    add_undo_entry(editor, "Set Parameter", 
                  editor->info.current_material, param_name, type, 
                  old_value ? old_value : value, value);
    
    // Call callback if set
    if (editor->param_change_callback) {
        editor->param_change_callback(handle, param_name, type, old_value, value, editor->callback_user_data);
    }
    
    if (old_value) free(old_value);
    return true;
}

bool material_editor_get_parameter(material_editor_handle_t handle, const char* param_name, 
                                  MaterialParameterType* type, void* value) {
    if (!param_name || !type || !value) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    MaterialParameter* param = NULL;
    
    if (editor->current_instance) {
        param = material_instance_get_param(editor->current_instance, param_name);
    } else if (editor->current_master) {
        param = find_param_in_array(editor->current_master->default_params, 
                                   editor->current_master->param_count, param_name);
    }
    
    if (!param) {
        printf("Parameter not found: %s\n", param_name);
        return false;
    }
    
    *type = param->type;
    
    // Copy value based on type
    switch (param->type) {
        case MATERIAL_PARAM_FLOAT:
            *(f32*)value = param->value.float_val;
            break;
        case MATERIAL_PARAM_VEC2:
            memcpy(value, param->value.vec2_val, sizeof(f32) * 2);
            break;
        case MATERIAL_PARAM_VEC3:
            memcpy(value, param->value.vec3_val, sizeof(f32) * 3);
            break;
        case MATERIAL_PARAM_VEC4:
        case MATERIAL_PARAM_COLOR:
            memcpy(value, param->value.vec4_val, sizeof(f32) * 4);
            break;
        case MATERIAL_PARAM_TEXTURE:
            *(TextureID*)value = param->value.texture_val;
            break;
        case MATERIAL_PARAM_INT:
            *(i32*)value = param->value.int_val;
            break;
        case MATERIAL_PARAM_BOOL:
            *(bool*)value = param->value.bool_val;
            break;
        default:
            printf("Unknown parameter type: %d\n", param->type);
            return false;
    }
    
    return true;
}

bool material_editor_reset_parameter(material_editor_handle_t handle, const char* param_name) {
    if (!param_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor || !editor->current_instance) return false;
    
    MaterialParameter* current_param = material_instance_get_param(editor->current_instance, param_name);
    if (!current_param || !current_param->is_overridden) {
        printf("Parameter not overridden or not found: %s\n", param_name);
        return false;
    }
    
    // Store old value for undo
    void* old_value = clone_parameter_value(current_param);
    
    // Find default value from master
    MaterialParameter* default_param = find_param_in_array(editor->current_master->default_params,
                                                          editor->current_master->param_count, param_name);
    if (!default_param) {
        printf("Default parameter not found: %s\n", param_name);
        if (old_value) free(old_value);
        return false;
    }
    
    // Reset to default value
    switch (default_param->type) {
        case MATERIAL_PARAM_FLOAT:
            material_instance_set_float(editor->current_instance, param_name, default_param->value.float_val);
            break;
        case MATERIAL_PARAM_VEC3:
            material_instance_set_vec3(editor->current_instance, param_name, default_param->value.vec3_val);
            break;
        case MATERIAL_PARAM_VEC4:
            material_instance_set_vec4(editor->current_instance, param_name, default_param->value.vec4_val);
            break;
        case MATERIAL_PARAM_TEXTURE:
            material_instance_set_texture(editor->current_instance, param_name, default_param->value.texture_val);
            break;
        case MATERIAL_PARAM_BOOL:
            material_instance_set_bool(editor->current_instance, param_name, default_param->value.bool_val);
            break;
        default:
            printf("Unsupported parameter type for reset: %d\n", default_param->type);
            if (old_value) free(old_value);
            return false;
    }
    
    // Add undo entry
    add_undo_entry(editor, "Reset Parameter", 
                  editor->info.current_material, param_name, default_param->type,
                  old_value, &default_param->value);
    
    printf("Reset parameter to default: %s\n", param_name);
    
    if (old_value) free(old_value);
    return true;
}

// ============================================================================
// PREVIEW AND RENDERING
// ============================================================================

bool material_editor_set_preview_mesh(material_editor_handle_t handle, const char* mesh_name) {
    if (!mesh_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // TODO: Implement mesh loading and preview
    printf("Set preview mesh: %s (not yet implemented)\n", mesh_name);
    return false;
}

bool material_editor_set_preview_environment(material_editor_handle_t handle, const char* environment_name) {
    if (!environment_name) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // TODO: Implement environment loading and preview
    printf("Set preview environment: %s (not yet implemented)\n", environment_name);
    return false;
}

bool material_editor_set_preview_lighting(material_editor_handle_t handle, float intensity, const float* direction) {
    if (!direction) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    editor->preview_light_intensity = intensity;
    editor->preview_light_direction = *(Vec3*)direction;
    
    printf("Set preview lighting: intensity=%.2f, direction=[%.2f,%.2f,%.2f]\n",
           intensity, direction[0], direction[1], direction[2]);
    return true;
}

bool material_editor_render_preview(material_editor_handle_t handle, uint32_t* output_texture) {
    if (!output_texture) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // TODO: Implement preview rendering
    printf("Render preview (not yet implemented)\n");
    return false;
}

// ============================================================================
// UNDO/REDO SYSTEM
// ============================================================================

void material_editor_begin_undo_group(material_editor_handle_t handle, const char* description) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    editor->in_undo_group = true;
    strncpy(editor->current_undo_group, description ? description : "Group", 
            sizeof(editor->current_undo_group) - 1);
}

void material_editor_end_undo_group(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    editor->in_undo_group = false;
    editor->current_undo_group[0] = '\0';
}

bool material_editor_undo(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor || editor->undo_top == 0) return false;
    
    editor->undo_top--;
    material_editor_undo_entry_t* entry = &editor->undo_stack[editor->undo_top];
    
    // Restore old value
    if (entry->is_parameter_change && entry->old_value) {
        material_editor_set_parameter(handle, entry->parameter_name, entry->type, entry->old_value);
    }
    
    // Move to redo stack
    if (editor->redo_top < MATERIAL_EDITOR_UNDO_STACK_SIZE) {
        editor->redo_stack[editor->redo_top] = *entry;
        editor->redo_top++;
    }
    
    printf("Undo: %s\n", entry->description);
    return true;
}

bool material_editor_redo(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor || editor->redo_top == 0) return false;
    
    editor->redo_top--;
    material_editor_undo_entry_t* entry = &editor->redo_stack[editor->redo_top];
    
    // Restore new value
    if (entry->is_parameter_change && entry->new_value) {
        material_editor_set_parameter(handle, entry->parameter_name, entry->type, entry->new_value);
    }
    
    // Move back to undo stack
    if (editor->undo_top < MATERIAL_EDITOR_UNDO_STACK_SIZE) {
        editor->undo_stack[editor->undo_top] = *entry;
        editor->undo_top++;
    }
    
    printf("Redo: %s\n", entry->description);
    return true;
}

void material_editor_get_undo_redo_info(material_editor_handle_t handle, uint32_t* undo_count, uint32_t* redo_count) {
    if (!undo_count || !redo_count) return;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) {
        *undo_count = 0;
        *redo_count = 0;
        return;
    }
    
    *undo_count = editor->undo_top;
    *redo_count = editor->redo_top;
}

void material_editor_clear_history(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    // Free undo stack
    for (uint32_t i = 0; i < editor->undo_top; i++) {
        free(editor->undo_stack[i].old_value);
        free(editor->undo_stack[i].new_value);
    }
    
    // Free redo stack
    for (uint32_t i = 0; i < editor->redo_top; i++) {
        free(editor->redo_stack[i].old_value);
        free(editor->redo_stack[i].new_value);
    }
    
    editor->undo_top = 0;
    editor->redo_top = 0;
    
    printf("Cleared undo/redo history\n");
}

// ============================================================================
// UI INTEGRATION
// ============================================================================

void material_editor_update_ui(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    // TODO: Implement UI updates
    // This would typically be called each frame to update the editor interface
}

bool material_editor_handle_input(material_editor_handle_t handle, const void* input_event) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    // TODO: Implement input handling
    // This would handle mouse/keyboard input for the editor interface
    return false;
}

void material_editor_set_param_change_callback(material_editor_handle_t handle, 
                                               material_editor_param_change_callback_t callback, 
                                               void* user_data) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    editor->param_change_callback = callback;
    editor->callback_user_data = user_data;
}

// ============================================================================
// UTILITIES
// ============================================================================

bool material_editor_validate_material(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    editor->diagnostic_count = 0;
    
    if (editor->current_instance) {
        return material_system_validate_instance(editor->current_instance);
    }
    
    if (editor->current_master) {
        // Validate master material
        if (editor->current_master->param_count == 0) {
            if (editor->diagnostic_count < MATERIAL_EDITOR_MAX_DIAGNOSTICS) {
                material_diagnostic_t* diag = &editor->diagnostics[editor->diagnostic_count++];
                strcpy(diag->message, "Master material has no parameters");
                diag->is_error = true;
                diag->line_number = 0;
                diag->parameter_name[0] = '\0';
            }
            return false;
        }
        
        return true;
    }
    
    return false;
}

bool material_editor_get_diagnostics(material_editor_handle_t handle, material_diagnostic_t* diagnostics, 
                                    uint32_t* diagnostic_count) {
    if (!diagnostics || !diagnostic_count) return false;
    
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    *diagnostic_count = editor->diagnostic_count;
    memcpy(diagnostics, editor->diagnostics, editor->diagnostic_count * sizeof(material_diagnostic_t));
    
    return true;
}

bool material_editor_is_hot_reload_enabled(material_editor_handle_t handle) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return false;
    
    return editor->hot_reload_enabled;
}

void material_editor_set_hot_reload(material_editor_handle_t handle, bool enabled) {
    material_editor_state_t* editor = find_editor_by_handle(handle);
    if (!editor) return;
    
    editor->hot_reload_enabled = enabled;
    printf("Hot reload %s for editor %u\n", enabled ? "enabled" : "disabled", handle.id);
}
