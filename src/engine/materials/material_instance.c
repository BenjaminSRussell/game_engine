#include "materials/material_instance.h"
#include "materials/shader_permutations.h"
#include "include/core/memory.h"
#include "include/core/logger.h"
#include <string.h>

// === Master Material API ===

MaterialMaster *material_master_create(const char *name, const char *shader_path) {
    MaterialMaster *master = (MaterialMaster*)MALLOC(sizeof(MaterialMaster));
    if (!master) return NULL;

    strncpy(master->name, name, sizeof(master->name) - 1);
    master->name[sizeof(master->name) - 1] = '\0';

    strncpy(master->shader_base_path, shader_path, sizeof(master->shader_base_path) - 1);
    master->shader_base_path[sizeof(master->shader_base_path) - 1] = '\0';

    master->variant_cache = shader_variant_cache_create(shader_path);
    master->supported_features = SHADER_FEATURE_NONE;
    master->default_quality = SHADER_QUALITY_HIGH;

    master->default_params = NULL;
    master->param_count = 0;
    master->param_capacity = 0;

    master->two_sided = false;
    master->alpha_blend = false;
    master->alpha_test = false;

    return master;
}

void material_master_destroy(MaterialMaster *master) {
    if (!master) return;

    if (master->variant_cache) {
        shader_variant_cache_destroy(master->variant_cache);
    }

    if (master->default_params) {
        FREE(master->default_params);
    }

    FREE(master);
}

void material_master_add_param(MaterialMaster *master, const char *name,
                                MaterialParameterType type, const void *default_value) {
    if (!master || !name) return;

    // Resize if needed
    if (master->param_count >= master->param_capacity) {
        u32 new_cap = master->param_capacity == 0 ? 8 : master->param_capacity * 2;
        master->default_params = (MaterialParameter*)REALLOC(master->default_params, new_cap * sizeof(MaterialParameter));
        master->param_capacity = new_cap;
    }

    MaterialParameter *param = &master->default_params[master->param_count++];
    strncpy(param->name, name, sizeof(param->name) - 1);
    param->name[sizeof(param->name) - 1] = '\0';
    param->type = type;
    param->is_overridden = false;

    if (default_value) {
        switch (type) {
            case MATERIAL_PARAM_FLOAT: param->value.float_val = *(f32*)default_value; break;
            case MATERIAL_PARAM_VEC2: memcpy(param->value.vec2_val, default_value, sizeof(f32) * 2); break;
            case MATERIAL_PARAM_VEC3: memcpy(param->value.vec3_val, default_value, sizeof(f32) * 3); break;
            case MATERIAL_PARAM_VEC4: memcpy(param->value.vec4_val, default_value, sizeof(f32) * 4); break;
            case MATERIAL_PARAM_COLOR: memcpy(param->value.vec4_val, default_value, sizeof(f32) * 4); break;
            case MATERIAL_PARAM_TEXTURE: param->value.texture_val = *(TextureID*)default_value; break;
            case MATERIAL_PARAM_INT: param->value.int_val = *(i32*)default_value; break;
            case MATERIAL_PARAM_BOOL: param->value.bool_val = *(bool*)default_value; break;
        }
    } else {
        memset(&param->value, 0, sizeof(param->value));
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

// === Instance API ===

MaterialInstance *material_instance_create(MaterialMaster *parent, const char *name) {
    if (!parent) return NULL;

    LOG_INFO("Allocating instance");
    MaterialInstance *instance = (MaterialInstance*)MALLOC(sizeof(MaterialInstance));
    if (!instance) return NULL;

    LOG_INFO("Copying name");
    strncpy(instance->name, name, sizeof(instance->name) - 1);
    instance->name[sizeof(instance->name) - 1] = '\0';

    instance->parent = parent;
    instance->overrides = NULL;
    instance->override_count = 0;
    instance->override_capacity = 0;

    instance->active_variant = NULL;
    // Initial compilation will happen on demand or explicitly
    instance->needs_recompile = true;

    return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
    if (!instance) return;

    if (instance->overrides) {
        FREE(instance->overrides);
    }

    FREE(instance);
}

// Helper to find or add override
static MaterialParameter *get_or_add_override(MaterialInstance *instance, const char *name) {
    // Check existing overrides
    for (u32 i = 0; i < instance->override_count; ++i) {
        if (strcmp(instance->overrides[i].name, name) == 0) {
            return &instance->overrides[i];
        }
    }

    // Find matching parameter in parent to get type
    MaterialParameter *parent_param = NULL;
    for (u32 i = 0; i < instance->parent->param_count; ++i) {
        if (strcmp(instance->parent->default_params[i].name, name) == 0) {
            parent_param = &instance->parent->default_params[i];
            break;
        }
    }

    if (!parent_param) {
        LOG_WARN("Attempting to override non-existent parameter '%s' in material '%s'", name, instance->parent->name);
        return NULL;
    }

    // Add new override
    if (instance->override_count >= instance->override_capacity) {
        u32 new_cap = instance->override_capacity == 0 ? 4 : instance->override_capacity * 2;
        instance->overrides = (MaterialParameter*)REALLOC(instance->overrides, new_cap * sizeof(MaterialParameter));
        instance->override_capacity = new_cap;
    }

    MaterialParameter *override = &instance->overrides[instance->override_count++];
    strncpy(override->name, name, sizeof(override->name) - 1);
    override->name[sizeof(override->name) - 1] = '\0';
    override->type = parent_param->type;
    override->is_overridden = true;

    return override;
}

void material_instance_set_float(MaterialInstance *instance, const char *name, f32 value) {
    MaterialParameter *p = get_or_add_override(instance, name);
    if (p && p->type == MATERIAL_PARAM_FLOAT) p->value.float_val = value;
}

void material_instance_set_vec3(MaterialInstance *instance, const char *name, const f32 *value) {
    MaterialParameter *p = get_or_add_override(instance, name);
    if (p && p->type == MATERIAL_PARAM_VEC3) memcpy(p->value.vec3_val, value, sizeof(f32) * 3);
}

void material_instance_set_vec4(MaterialInstance *instance, const char *name, const f32 *value) {
    MaterialParameter *p = get_or_add_override(instance, name);
    if (p && (p->type == MATERIAL_PARAM_VEC4 || p->type == MATERIAL_PARAM_COLOR))
        memcpy(p->value.vec4_val, value, sizeof(f32) * 4);
}

void material_instance_set_texture(MaterialInstance *instance, const char *name, TextureID texture) {
    MaterialParameter *p = get_or_add_override(instance, name);
    if (p && p->type == MATERIAL_PARAM_TEXTURE) p->value.texture_val = texture;
}

void material_instance_set_bool(MaterialInstance *instance, const char *name, bool value) {
    MaterialParameter *p = get_or_add_override(instance, name);
    if (p && p->type == MATERIAL_PARAM_BOOL) p->value.bool_val = value;
}

MaterialParameter *material_instance_get_param(MaterialInstance *instance, const char *name) {
    // Check overrides
    for (u32 i = 0; i < instance->override_count; ++i) {
        if (strcmp(instance->overrides[i].name, name) == 0) {
            return &instance->overrides[i];
        }
    }

    // Check parent
    for (u32 i = 0; i < instance->parent->param_count; ++i) {
        if (strcmp(instance->parent->default_params[i].name, name) == 0) {
            return &instance->parent->default_params[i];
        }
    }

    return NULL;
}

bool material_instance_compile(MaterialInstance *instance) {
    if (!instance || !instance->parent) return false;

    // Determine active features based on parameters (e.g., if normal map is set)
    u32 active_features = instance->parent->supported_features; // Start with all supported? Or base?
    // Actually, normally we check if texture slots are filled to enable features

    // Logic to determine features from parameters could be added here
    // For now, use default quality and supported features

    instance->active_permutation = shader_permutation_create(active_features, instance->parent->default_quality);

    instance->active_variant = shader_variant_get_or_compile(instance->parent->variant_cache, instance->active_permutation);

    instance->needs_recompile = false;
    return instance->active_variant != NULL;
}

void material_instance_bind(MaterialInstance *instance) {
    if (!instance) return;

    if (instance->needs_recompile || !instance->active_variant) {
        material_instance_compile(instance);
    }

    // Bind shader program (stub)
    // glUseProgram(instance->active_variant->program_id);
}

void material_instance_upload_params(MaterialInstance *instance) {
    if (!instance || !instance->active_variant) return;

    // Iterate over active variant uniforms and upload values from params
    // This requires mapping param names to uniform locations
    // For this stub, we just log
    // LOG_TRACE("Uploading params for material instance %s", instance->name);
}
