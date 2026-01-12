#include "materials/material_instance.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

#define MATERIAL_INSTANCE_INITIAL_PARAMS 16
#define MATERIAL_MASTER_INITIAL_PARAMS 16

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static MaterialParameter* find_param_in_array(MaterialParameter* params, u32 count, const char* name) {
    if (!params || !name) return NULL;
    
    for (u32 i = 0; i < count; i++) {
        if (strcmp(params[i].name, name) == 0) {
            return &params[i];
        }
    }
    return NULL;
}

static bool copy_parameter_value(MaterialParameter* dest, const MaterialParameter* src) {
    if (!dest || !src || dest->type != src->type) {
        return false;
    }
    
    switch (src->type) {
        case MATERIAL_PARAM_FLOAT:
            dest->value.float_val = src->value.float_val;
            break;
        case MATERIAL_PARAM_VEC2:
            memcpy(dest->value.vec2_val, src->value.vec2_val, sizeof(f32) * 2);
            break;
        case MATERIAL_PARAM_VEC3:
            memcpy(dest->value.vec3_val, src->value.vec3_val, sizeof(f32) * 3);
            break;
        case MATERIAL_PARAM_VEC4:
        case MATERIAL_PARAM_COLOR:
            memcpy(dest->value.vec4_val, src->value.vec4_val, sizeof(f32) * 4);
            break;
        case MATERIAL_PARAM_TEXTURE:
            dest->value.texture_val = src->value.texture_val;
            break;
        case MATERIAL_PARAM_INT:
            dest->value.int_val = src->value.int_val;
            break;
        case MATERIAL_PARAM_BOOL:
            dest->value.bool_val = src->value.bool_val;
            break;
        default:
            return false;
    }
    
    dest->is_overridden = src->is_overridden;
    return true;
}

static void print_parameter(const MaterialParameter* param) {
    if (!param) return;
    
    printf("  %s (%s): ", param->name, 
           param->type == MATERIAL_PARAM_FLOAT ? "float" :
           param->type == MATERIAL_PARAM_VEC2 ? "vec2" :
           param->type == MATERIAL_PARAM_VEC3 ? "vec3" :
           param->type == MATERIAL_PARAM_VEC4 ? "vec4" :
           param->type == MATERIAL_PARAM_COLOR ? "color" :
           param->type == MATERIAL_PARAM_TEXTURE ? "texture" :
           param->type == MATERIAL_PARAM_INT ? "int" :
           param->type == MATERIAL_PARAM_BOOL ? "bool" : "unknown");
    
    switch (param->type) {
        case MATERIAL_PARAM_FLOAT:
            printf("%.3f", param->value.float_val);
            break;
        case MATERIAL_PARAM_VEC2:
            printf("[%.3f, %.3f]", param->value.vec2_val[0], param->value.vec2_val[1]);
            break;
        case MATERIAL_PARAM_VEC3:
            printf("[%.3f, %.3f, %.3f]", param->value.vec3_val[0], param->value.vec3_val[1], param->value.vec3_val[2]);
            break;
        case MATERIAL_PARAM_VEC4:
        case MATERIAL_PARAM_COLOR:
            printf("[%.3f, %.3f, %.3f, %.3f]", param->value.vec4_val[0], param->value.vec4_val[1], param->value.vec4_val[2], param->value.vec4_val[3]);
            break;
        case MATERIAL_PARAM_TEXTURE:
            printf("texture_id:%u", param->value.texture_val.id);
            break;
        case MATERIAL_PARAM_INT:
            printf("%d", param->value.int_val);
            break;
        case MATERIAL_PARAM_BOOL:
            printf("%s", param->value.bool_val ? "true" : "false");
            break;
    }
    
    if (param->is_overridden) {
        printf(" [OVERRIDE]");
    }
    printf("\n");
}

// ============================================================================
// MASTER MATERIAL API
// ============================================================================

MaterialMaster* material_master_create(const char* name, const char* shader_path) {
    if (!name || !shader_path) {
        return NULL;
    }
    
    MaterialMaster* master = calloc(1, sizeof(MaterialMaster));
    if (!master) {
        return NULL;
    }
    
    strncpy(master->name, name, sizeof(master->name) - 1);
    strncpy(master->shader_base_path, shader_path, sizeof(master->shader_base_path) - 1);
    
    // Allocate initial parameter storage
    master->param_capacity = MATERIAL_MASTER_INITIAL_PARAMS;
    master->default_params = malloc(master->param_capacity * sizeof(MaterialParameter));
    if (!master->default_params) {
        free(master);
        return NULL;
    }
    
    // Initialize shader variant cache (placeholder)
    master->variant_cache = NULL;
    master->supported_features = 0;
    master->default_quality = 1; // Medium quality
    
    // Default render states
    master->two_sided = false;
    master->alpha_blend = false;
    master->alpha_test = false;
    
    printf("Created master material: %s (shader: %s)\n", name, shader_path);
    return master;
}

void material_master_destroy(MaterialMaster* master) {
    if (!master) return;
    
    free(master->default_params);
    
    // Destroy shader variant cache (placeholder)
    if (master->variant_cache) {
        // TODO: Implement shader variant cache destruction
    }
    
    free(master);
    printf("Destroyed master material: %s\n", master->name);
}

void material_master_add_param(MaterialMaster* master, const char* name, 
                             MaterialParameterType type, const void* default_value) {
    if (!master || !name || !default_value) {
        return;
    }
    
    // Check if parameter already exists
    if (find_param_in_array(master->default_params, master->param_count, name)) {
        printf("Warning: Parameter '%s' already exists in master '%s'\n", name, master->name);
        return;
    }
    
    // Expand parameter array if needed
    if (master->param_count >= master->param_capacity) {
        u32 new_capacity = master->param_capacity * 2;
        MaterialParameter* new_params = realloc(master->default_params, new_capacity * sizeof(MaterialParameter));
        if (!new_params) {
            printf("Error: Failed to expand parameter array for master '%s'\n", master->name);
            return;
        }
        
        master->default_params = new_params;
        master->param_capacity = new_capacity;
    }
    
    // Add new parameter
    MaterialParameter* param = &master->default_params[master->param_count];
    strncpy(param->name, name, sizeof(param->name) - 1);
    param->type = type;
    param->is_overridden = false;
    
    // Copy default value
    switch (type) {
        case MATERIAL_PARAM_FLOAT:
            param->value.float_val = *(const f32*)default_value;
            break;
        case MATERIAL_PARAM_VEC2:
            memcpy(param->value.vec2_val, default_value, sizeof(f32) * 2);
            break;
        case MATERIAL_PARAM_VEC3:
            memcpy(param->value.vec3_val, default_value, sizeof(f32) * 3);
            break;
        case MATERIAL_PARAM_VEC4:
        case MATERIAL_PARAM_COLOR:
            memcpy(param->value.vec4_val, default_value, sizeof(f32) * 4);
            break;
        case MATERIAL_PARAM_TEXTURE:
            param->value.texture_val = *(const TextureID*)default_value;
            break;
        case MATERIAL_PARAM_INT:
            param->value.int_val = *(const i32*)default_value;
            break;
        case MATERIAL_PARAM_BOOL:
            param->value.bool_val = *(const bool*)default_value;
            break;
        default:
            printf("Error: Unknown parameter type for '%s'\n", name);
            return;
    }
    
    master->param_count++;
    printf("Added parameter '%s' to master '%s'\n", name, master->name);
}

void material_master_add_texture(MaterialMaster* master, const char* name, TextureID default_tex) {
    material_master_add_param(master, name, MATERIAL_PARAM_TEXTURE, &default_tex);
}

void material_master_set_features(MaterialMaster* master, u32 feature_mask) {
    if (!master) return;
    
    master->supported_features = feature_mask;
    printf("Set features 0x%08x for master '%s'\n", feature_mask, master->name);
}

// ============================================================================
// INSTANCE API
// ============================================================================

MaterialInstance* material_instance_create(MaterialMaster* parent, const char* name) {
    if (!parent || !name) {
        return NULL;
    }
    
    MaterialInstance* instance = calloc(1, sizeof(MaterialInstance));
    if (!instance) {
        return NULL;
    }
    
    strncpy(instance->name, name, sizeof(instance->name) - 1);
    instance->parent = parent;
    
    // Allocate override storage
    instance->override_capacity = MATERIAL_INSTANCE_INITIAL_PARAMS;
    instance->overrides = malloc(instance->override_capacity * sizeof(MaterialParameter));
    if (!instance->overrides) {
        free(instance);
        return NULL;
    }
    
    // Initialize shader state
    instance->active_variant = NULL;
    instance->active_permutation = 0;
    instance->needs_recompile = true;
    
    printf("Created material instance: %s (parent: %s)\n", name, parent->name);
    return instance;
}

void material_instance_destroy(MaterialInstance* instance) {
    if (!instance) return;
    
    free(instance->overrides);
    
    // Destroy active shader variant (placeholder)
    if (instance->active_variant) {
        // TODO: Implement shader variant destruction
    }
    
    free(instance);
    printf("Destroyed material instance: %s\n", instance->name);
}

static MaterialParameter* add_or_update_override(MaterialInstance* instance, const char* name, MaterialParameterType type) {
    // Check if override already exists
    MaterialParameter* existing = find_param_in_array(instance->overrides, instance->override_count, name);
    if (existing) {
        if (existing->type != type) {
            printf("Error: Parameter '%s' type mismatch in instance '%s'\n", name, instance->name);
            return NULL;
        }
        return existing;
    }
    
    // Expand override array if needed
    if (instance->override_count >= instance->override_capacity) {
        u32 new_capacity = instance->override_capacity * 2;
        MaterialParameter* new_overrides = realloc(instance->overrides, new_capacity * sizeof(MaterialParameter));
        if (!new_overrides) {
            printf("Error: Failed to expand override array for instance '%s'\n", instance->name);
            return NULL;
        }
        
        instance->overrides = new_overrides;
        instance->override_capacity = new_capacity;
    }
    
    // Add new override
    MaterialParameter* param = &instance->overrides[instance->override_count];
    strncpy(param->name, name, sizeof(param->name) - 1);
    param->type = type;
    param->is_overridden = true;
    
    instance->override_count++;
    instance->needs_recompile = true;
    
    return param;
}

void material_instance_set_float(MaterialInstance* instance, const char* name, f32 value) {
    MaterialParameter* param = add_or_update_override(instance, name, MATERIAL_PARAM_FLOAT);
    if (param) {
        param->value.float_val = value;
    }
}

void material_instance_set_vec3(MaterialInstance* instance, const char* name, const f32* value) {
    MaterialParameter* param = add_or_update_override(instance, name, MATERIAL_PARAM_VEC3);
    if (param) {
        memcpy(param->value.vec3_val, value, sizeof(f32) * 3);
    }
}

void material_instance_set_vec4(MaterialInstance* instance, const char* name, const f32* value) {
    MaterialParameter* param = add_or_update_override(instance, name, MATERIAL_PARAM_VEC4);
    if (param) {
        memcpy(param->value.vec4_val, value, sizeof(f32) * 4);
    }
}

void material_instance_set_texture(MaterialInstance* instance, const char* name, TextureID texture) {
    MaterialParameter* param = add_or_update_override(instance, name, MATERIAL_PARAM_TEXTURE);
    if (param) {
        param->value.texture_val = texture;
    }
}

void material_instance_set_bool(MaterialInstance* instance, const char* name, bool value) {
    MaterialParameter* param = add_or_update_override(instance, name, MATERIAL_PARAM_BOOL);
    if (param) {
        param->value.bool_val = value;
    }
}

MaterialParameter* material_instance_get_param(MaterialInstance* instance, const char* name) {
    if (!instance || !name) {
        return NULL;
    }
    
    // First check overrides
    MaterialParameter* override = find_param_in_array(instance->overrides, instance->override_count, name);
    if (override) {
        return override;
    }
    
    // Then check parent defaults
    if (instance->parent) {
        return find_param_in_array(instance->parent->default_params, instance->parent->param_count, name);
    }
    
    return NULL;
}

bool material_instance_compile(MaterialInstance* instance) {
    if (!instance || !instance->parent) {
        return false;
    }
    
    // TODO: Implement shader variant compilation
    // This would involve:
    // 1. Determining required shader features based on active parameters
    // 2. Creating a shader permutation
    // 3. Compiling the shader variant
    // 4. Caching the variant
    
    instance->needs_recompile = false;
    printf("Compiled material instance: %s\n", instance->name);
    return true;
}

void material_instance_bind(MaterialInstance* instance) {
    if (!instance) return;
    
    // TODO: Implement material binding
    // This would involve:
    // 1. Binding the compiled shader variant
    // 2. Setting render states (blend, cull, etc.)
    // 3. Uploading parameters to GPU
    
    printf("Bound material instance: %s\n", instance->name);
}

void material_instance_upload_params(MaterialInstance* instance) {
    if (!instance) return;
    
    // TODO: Implement parameter upload to GPU
    // This would involve:
    // 1. Getting all effective parameters (overrides + defaults)
    // 2. Uploading them to the appropriate shader uniforms
    
    printf("Uploaded parameters for material instance: %s\n", instance->name);
}

// ============================================================================
// UTILITY AND DEBUG FUNCTIONS
// ============================================================================

void material_master_print_info(const MaterialMaster* master) {
    if (!master) return;
    
    printf("Master Material: %s\n", master->name);
    printf("  Shader: %s\n", master->shader_base_path);
    printf("  Parameters: %u\n", master->param_count);
    printf("  Features: 0x%08x\n", master->supported_features);
    printf("  Two-sided: %s\n", master->two_sided ? "Yes" : "No");
    printf("  Alpha blend: %s\n", master->alpha_blend ? "Yes" : "No");
    printf("  Alpha test: %s\n", master->alpha_test ? "Yes" : "No");
    
    if (master->param_count > 0) {
        printf("  Default Parameters:\n");
        for (u32 i = 0; i < master->param_count; i++) {
            print_parameter(&master->default_params[i]);
        }
    }
}

void material_instance_print_info(const MaterialInstance* instance) {
    if (!instance) return;
    
    printf("Material Instance: %s\n", instance->name);
    printf("  Parent: %s\n", instance->parent ? instance->parent->name : "NULL");
    printf("  Overrides: %u\n", instance->override_count);
    printf("  Needs recompile: %s\n", instance->needs_recompile ? "Yes" : "No");
    
    if (instance->override_count > 0) {
        printf("  Parameter Overrides:\n");
        for (u32 i = 0; i < instance->override_count; i++) {
            print_parameter(&instance->overrides[i]);
        }
    }
    
    // Show effective parameters (overrides + defaults)
    printf("  Effective Parameters:\n");
    if (instance->parent) {
        for (u32 i = 0; i < instance->parent->param_count; i++) {
            const MaterialParameter* default_param = &instance->parent->default_params[i];
            MaterialParameter* override = find_param_in_array(instance->overrides, instance->override_count, default_param->name);
            
            printf("  %s: ", default_param->name);
            if (override) {
                print_parameter(override);
            } else {
                print_parameter(default_param);
            }
        }
    }
}
