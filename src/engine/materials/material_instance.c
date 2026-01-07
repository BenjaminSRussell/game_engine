// Material Instance System - Implementation
#include "materials/material_instance.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// === Material Master ===

MaterialMaster *material_master_create(const char *name, const char *shader_path) {
    if (!name || !shader_path) return NULL;
    
    MaterialMaster *master = (MaterialMaster *)calloc(1, sizeof(MaterialMaster));
    if (!master) {
        LOG_ERROR("Failed to allocate material master");
        return NULL;
    }
    
    strncpy(master->name, name, sizeof(master->name) - 1);
    strncpy(master->shader_base_path, shader_path, sizeof(master->shader_base_path) - 1);
    
    // Create shader variant cache
    master->variant_cache = shader_variant_cache_create(shader_path);
    if (!master->variant_cache) {
        free(master);
        return NULL;
    }
    
    // Initialize parameters array
    master->param_capacity = 16;
    master->default_params = (MaterialParameter *)calloc(master->param_capacity, sizeof(MaterialParameter));
    master->param_count = 0;
    
    master->supported_features = SHADER_FEATURE_NONE;
    master->default_quality = SHADER_QUALITY_HIGH;
    master->two_sided = false;
    master->alpha_blend = false;
    master->alpha_test = false;
    
    LOG_INFO("Created material master: %s", name);
    return master;
}

void material_master_destroy(MaterialMaster *master) {
    if (!master) return;
    
    if (master->variant_cache) {
        shader_variant_cache_destroy(master->variant_cache);
    }
    
    free(master->default_params);
    free(master);
}

void material_master_add_param(MaterialMaster *master, const char *name,
                                MaterialParameterType type, const void *default_value) {
    if (!master || !name) return;
    
    // Grow if needed
    if (master->param_count >= master->param_capacity) {
        master->param_capacity *= 2;
        master->default_params = (MaterialParameter *)realloc(master->default_params,
                                                               master->param_capacity * sizeof(MaterialParameter));
    }
    
    MaterialParameter *param = &master->default_params[master->param_count++];
    strncpy(param->name, name, sizeof(param->name) - 1);
    param->type = type;
    param->is_overridden = false;
    
    // Set default value
    if (default_value) {
        switch (type) {
            case MATERIAL_PARAM_FLOAT:
                param->value.float_val = *(f32*)default_value;
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
            case MATERIAL_PARAM_INT:
                param->value.int_val = *(i32*)default_value;
                break;
            case MATERIAL_PARAM_BOOL:
                param->value.bool_val = *(bool*)default_value;
                break;
            case MATERIAL_PARAM_TEXTURE:
                param->value.texture_val = *(TextureID*)default_value;
                break;
        }
    }
}

void material_master_add_texture(MaterialMaster *master, const char *name, TextureID default_tex) {
    material_master_add_param(master, name, MATERIAL_PARAM_TEXTURE, &default_tex);
}

void material_master_set_features(MaterialMaster *master, u32 feature_mask) {
    if (master) {
        master->supported_features = feature_mask;
    }
}

// === Material Instance ===

MaterialInstance *material_instance_create(MaterialMaster *parent, const char *name) {
    if (!parent || !name) return NULL;
    
    MaterialInstance *instance = (MaterialInstance *)calloc(1, sizeof(MaterialInstance));
    if (!instance) {
        LOG_ERROR("Failed to allocate material instance");
        return NULL;
    }
    
    strncpy(instance->name, name, sizeof(instance->name) - 1);
    instance->parent = parent;
    
    // Initialize overrides array
    instance->override_capacity = 8;
    instance->overrides = (MaterialParameter *)calloc(instance->override_capacity, sizeof(MaterialParameter));
    instance->override_count = 0;
    
    instance->active_variant = NULL;
    instance->needs_recompile = true;
    
    LOG_INFO("Created material instance: %s (parent: %s)", name, parent->name);
    return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
    if (!instance) return;
    
    free(instance->overrides);
    free(instance);
}

static MaterialParameter *find_or_create_override(MaterialInstance *instance, const char *name) {
    // Check if override exists
    for (u32 i = 0; i < instance->override_count; i++) {
        if (strcmp(instance->overrides[i].name, name) == 0) {
            return &instance->overrides[i];
        }
    }
    
    // Find in parent defaults
    MaterialParameter *parent_param = NULL;
    for (u32 i = 0; i < instance->parent->param_count; i++) {
        if (strcmp(instance->parent->default_params[i].name, name) == 0) {
            parent_param = &instance->parent->default_params[i];
            break;
        }
    }
    
    if (!parent_param) {
        LOG_WARN("Parameter %s not found in parent material %s", name, instance->parent->name);
        return NULL;
    }
    
    // Grow overrides if needed
    if (instance->override_count >= instance->override_capacity) {
        instance->override_capacity *= 2;
        instance->overrides = (MaterialParameter *)realloc(instance->overrides,
                                                            instance->override_capacity * sizeof(MaterialParameter));
    }
    
    // Create new override
    MaterialParameter *override = &instance->overrides[instance->override_count++];
    *override = *parent_param;  // Copy from parent
    override->is_overridden = true;
    
    instance->needs_recompile = true;
    
    return override;
}

void material_instance_set_float(MaterialInstance *instance, const char *name, f32 value) {
    MaterialParameter *param = find_or_create_override(instance, name);
    if (param && param->type == MATERIAL_PARAM_FLOAT) {
        param->value.float_val = value;
    }
}

void material_instance_set_vec3(MaterialInstance *instance, const char *name, const f32 *value) {
    MaterialParameter *param = find_or_create_override(instance, name);
    if (param && param->type == MATERIAL_PARAM_VEC3) {
        memcpy(param->value.vec3_val, value, sizeof(f32) * 3);
    }
}

void material_instance_set_vec4(MaterialInstance *instance, const char *name, const f32 *value) {
    MaterialParameter *param = find_or_create_override(instance, name);
    if (param && (param->type == MATERIAL_PARAM_VEC4 || param->type == MATERIAL_PARAM_COLOR)) {
        memcpy(param->value.vec4_val, value, sizeof(f32) * 4);
    }
}

void material_instance_set_texture(MaterialInstance *instance, const char *name, TextureID texture) {
    MaterialParameter *param = find_or_create_override(instance, name);
    if (param && param->type == MATERIAL_PARAM_TEXTURE) {
        param->value.texture_val = texture;
    }
}

void material_instance_set_bool(MaterialInstance *instance, const char *name, bool value) {
    MaterialParameter *param = find_or_create_override(instance, name);
    if (param && param->type == MATERIAL_PARAM_BOOL) {
        param->value.bool_val = value;
    }
}

MaterialParameter *material_instance_get_param(MaterialInstance *instance, const char *name) {
    if (!instance || !name) return NULL;
    
    // Check overrides first
    for (u32 i = 0; i < instance->override_count; i++) {
        if (strcmp(instance->overrides[i].name, name) == 0) {
            return &instance->overrides[i];
        }
    }
    
    // Fall back to parent defaults
    for (u32 i = 0; i < instance->parent->param_count; i++) {
        if (strcmp(instance->parent->default_params[i].name, name) == 0) {
            return &instance->parent->default_params[i];
        }
    }
    
    return NULL;
}

bool material_instance_compile(MaterialInstance *instance) {
    if (!instance || !instance->parent) return false;
    
    // Determine active features based on texture assignments
    u32 features = SHADER_FEATURE_NONE;
    
    // Check if textures are assigned to enable features
    MaterialParameter *normal_map = material_instance_get_param(instance, "NormalMap");
    if (normal_map && normal_map->value.texture_val.id != 0) {
        features |= SHADER_FEATURE_NORMAL_MAP;
    }
    
    MaterialParameter *roughness_map = material_instance_get_param(instance, "RoughnessMap");
    if (roughness_map && roughness_map->value.texture_val.id != 0) {
        features |= SHADER_FEATURE_ROUGHNESS_MAP;
    }
    
    MaterialParameter *metallic_map = material_instance_get_param(instance, "MetallicMap");
    if (metallic_map && metallic_map->value.texture_val.id != 0) {
        features |= SHADER_FEATURE_METALLIC_MAP;
    }
    
    MaterialParameter *ao_map = material_instance_get_param(instance, "AOMap");
    if (ao_map && ao_map->value.texture_val.id != 0) {
        features |= SHADER_FEATURE_AO_MAP;
    }
    
    MaterialParameter *emissive_map = material_instance_get_param(instance, "EmissiveMap");
    if (emissive_map && emissive_map->value.texture_val.id != 0) {
        features |= SHADER_FEATURE_EMISSIVE;
    }
    
    // Create permutation
    ShaderPermutation perm = shader_permutation_create(features, instance->parent->default_quality);
    
    // Get or compile variant
    instance->active_variant = shader_variant_get_or_compile(instance->parent->variant_cache, perm);
    if (!instance->active_variant) {
        LOG_ERROR("Failed to compile shader variant for material %s", instance->name);
        return false;
    }
    
    instance->active_permutation = perm;
    instance->needs_recompile = false;
    
    LOG_DEBUG("Material %s compiled with features: 0x%X", instance->name, features);
    return true;
}

void material_instance_bind(MaterialInstance *instance) {
    if (!instance) return;
    
    // Compile if needed
    if (instance->needs_recompile || !instance->active_variant) {
        material_instance_compile(instance);
    }
    
    // Bind shader variant
    if (instance->active_variant && instance->active_variant->is_compiled) {
        glUseProgram(instance->active_variant->program_id);
    }
}

void material_instance_upload_params(MaterialInstance *instance) {
    if (!instance || !instance->active_variant) return;
    
    ShaderVariant *variant = instance->active_variant;
    
    // Upload scalar parameters
    MaterialParameter *base_color = material_instance_get_param(instance, "BaseColor");
    if (base_color && variant->uniforms.base_color != -1) {
        glUniform3fv(variant->uniforms.base_color, 1, base_color->value.vec3_val);
    }
    
    MaterialParameter *metallic = material_instance_get_param(instance, "Metallic");
    if (metallic && variant->uniforms.metallic != -1) {
        glUniform1f(variant->uniforms.metallic, metallic->value.float_val);
    }
    
    MaterialParameter *roughness = material_instance_get_param(instance, "Roughness");
    if (roughness && variant->uniforms.roughness != -1) {
        glUniform1f(variant->uniforms.roughness, roughness->value.float_val);
    }
    
    MaterialParameter *ao = material_instance_get_param(instance, "AO");
    if (ao && variant->uniforms.ao != -1) {
        glUniform1f(variant->uniforms.ao, ao->value.float_val);
    }
    
    // Bind textures
    u32 texture_slot = 0;
    
    MaterialParameter *base_color_map = material_instance_get_param(instance, "BaseColorMap");
    if (base_color_map && base_color_map->value.texture_val.id != 0) {
        glActiveTexture(GL_TEXTURE0 + texture_slot);
        glBindTexture(GL_TEXTURE_2D, base_color_map->value.texture_val.id);
        glUniform1i(variant->uniforms.base_color_map, texture_slot);
        texture_slot++;
    }
    
    MaterialParameter *normal_map = material_instance_get_param(instance, "NormalMap");
    if (normal_map && normal_map->value.texture_val.id != 0) {
        glActiveTexture(GL_TEXTURE0 + texture_slot);
        glBindTexture(GL_TEXTURE_2D, normal_map->value.texture_val.id);
        glUniform1i(variant->uniforms.normal_map, texture_slot);
        texture_slot++;
    }
    
    MaterialParameter *roughness_map = material_instance_get_param(instance, "RoughnessMap");
    if (roughness_map && roughness_map->value.texture_val.id != 0) {
        glActiveTexture(GL_TEXTURE0 + texture_slot);
        glBindTexture(GL_TEXTURE_2D, roughness_map->value.texture_val.id);
        glUniform1i(variant->uniforms.roughness_map, texture_slot);
        texture_slot++;
    }
    
    MaterialParameter *metallic_map = material_instance_get_param(instance, "MetallicMap");
    if (metallic_map && metallic_map->value.texture_val.id != 0) {
        glActiveTexture(GL_TEXTURE0 + texture_slot);
        glBindTexture(GL_TEXTURE_2D, metallic_map->value.texture_val.id);
        glUniform1i(variant->uniforms.metallic_map, texture_slot);
        texture_slot++;
    }
}
