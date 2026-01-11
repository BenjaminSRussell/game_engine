// src/engine/rendering/core/material.c
// Material System - PBR materials and texture management

#include <core/logger.h>
#include <core/memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../render_pipeline.h"

// ============================================================================
// Material Types
// ============================================================================

typedef enum {
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_UNLIT,
    MATERIAL_TYPE_CUSTOM
} MaterialType;

typedef enum {
    TEXTURE_TYPE_ALBEDO,
    TEXTURE_TYPE_NORMAL,
    TEXTURE_TYPE_METALLIC,
    TEXTURE_TYPE_ROUGHNESS,
    TEXTURE_TYPE_AO,
    TEXTURE_TYPE_EMISSIVE,
    TEXTURE_TYPE_COUNT
} TextureType;

typedef struct {
    void *texture;
    char name[64];
    uint32_t width, height;
    uint32_t format;
    bool is_loaded;
} MaterialTexture;

typedef struct {
    MaterialType type;
    char name[64];
    
    // PBR properties
    float albedo[3];
    float metallic;
    float roughness;
    float ao;
    float emissive[3];
    float alpha;
    
    // Textures
    MaterialTexture textures[TEXTURE_TYPE_COUNT];
    
    // Shader
    uint32_t shader_program;
    
    // GPU data
    void *uniform_buffer;
    bool needs_update;
    
    // Instance management
    struct MaterialInstance *instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    
    bool enabled;
} Material;

typedef struct MaterialInstance {
    Material *parent;
    float albedo_override[3];
    float metallic_override;
    float roughness_override;
    float ao_override;
    float emissive_override[3];
    float alpha_override;
    
    bool has_albedo_override;
    bool has_metallic_override;
    bool has_roughness_override;
    bool has_ao_override;
    bool has_emissive_override;
    bool has_alpha_override;
    
    void *uniform_buffer;
    bool needs_update;
} MaterialInstance;

typedef struct {
    Material *materials[1024];
    uint32_t material_count;
    
    // Global texture atlas
    void *texture_atlas;
    
    // Default materials
    Material *default_material;
    Material *error_material;
    
    // Shader cache
    uint32_t pbr_shader;
    uint32_t unlit_shader;
    
    bool initialized;
} MaterialSystem;

static MaterialSystem g_material_system = {0};

// ============================================================================
// Material System API
// ============================================================================

bool material_system_init(void) {
    if (g_material_system.initialized) {
        LOG_WARN("Material system already initialized");
        return true;
    }
    
    // Initialize material array
    memset(&g_material_system, 0, sizeof(MaterialSystem));
    
    // TODO: Load default shaders
    // g_material_system.pbr_shader = shader_load("pbr_standard");
    // g_material_system.unlit_shader = shader_load("unlit_standard");
    
    // Create default materials
    g_material_system.default_material = material_create("default");
    if (g_material_system.default_material) {
        material_set_type(g_material_system.default_material, MATERIAL_TYPE_PBR);
        material_set_albedo(g_material_system.default_material, 0.8f, 0.8f, 0.8f);
        material_set_metallic(g_material_system.default_material, 0.0f);
        material_set_roughness(g_material_system.default_material, 0.5f);
    }
    
    g_material_system.error_material = material_create("error");
    if (g_material_system.error_material) {
        material_set_type(g_material_system.error_material, MATERIAL_TYPE_UNLIT);
        material_set_albedo(g_material_system.error_material, 1.0f, 0.0f, 1.0f);
    }
    
    g_material_system.initialized = true;
    LOG_INFO("Material system initialized");
    return true;
}

void material_system_shutdown(void) {
    if (!g_material_system.initialized)
        return;
    
    // Destroy all materials
    for (uint32_t i = 0; i < g_material_system.material_count; i++) {
        if (g_material_system.materials[i]) {
            material_destroy(g_material_system.materials[i]);
        }
    }
    
    // TODO: Destroy shaders
    // TODO: Destroy texture atlas
    
    memset(&g_material_system, 0, sizeof(MaterialSystem));
    LOG_INFO("Material system shutdown");
}

Material *material_create(const char *name) {
    if (!g_material_system.initialized || !name) {
        LOG_ERROR("Material system not initialized or invalid name");
        return NULL;
    }
    
    if (g_material_system.material_count >= 1024) {
        LOG_ERROR("Too many materials");
        return NULL;
    }
    
    Material *material = calloc(1, sizeof(Material));
    if (!material) {
        LOG_ERROR("Failed to allocate material");
        return NULL;
    }
    
    strncpy(material->name, name, sizeof(material->name) - 1);
    material->type = MATERIAL_TYPE_PBR;
    
    // Default PBR values
    material->albedo[0] = 1.0f;
    material->albedo[1] = 1.0f;
    material->albedo[2] = 1.0f;
    material->metallic = 0.0f;
    material->roughness = 0.5f;
    material->ao = 1.0f;
    material->emissive[0] = 0.0f;
    material->emissive[1] = 0.0f;
    material->emissive[2] = 0.0f;
    material->alpha = 1.0f;
    
    material->enabled = true;
    material->needs_update = true;
    
    // TODO: Create uniform buffer
    
    g_material_system.materials[g_material_system.material_count++] = material;
    
    LOG_DEBUG("Created material: %s", name);
    return material;
}

void material_destroy(Material *material) {
    if (!material)
        return;
    
    // Destroy instances
    for (uint32_t i = 0; i < material->instance_count; i++) {
        material_instance_destroy(&material->instances[i]);
    }
    free(material->instances);
    
    // TODO: Destroy uniform buffer
    // TODO: Destroy textures
    
    // Remove from global list
    for (uint32_t i = 0; i < g_material_system.material_count; i++) {
        if (g_material_system.materials[i] == material) {
            g_material_system.materials[i] = g_material_system.materials[g_material_system.material_count - 1];
            g_material_system.material_count--;
            break;
        }
    }
    
    free(material);
    LOG_DEBUG("Destroyed material: %s", material->name);
}

void material_set_type(Material *material, MaterialType type) {
    if (!material)
        return;
    
    material->type = type;
    material->needs_update = true;
}

void material_set_albedo(Material *material, float r, float g, float b) {
    if (!material)
        return;
    
    material->albedo[0] = r;
    material->albedo[1] = g;
    material->albedo[2] = b;
    material->needs_update = true;
}

void material_set_metallic(Material *material, float metallic) {
    if (!material)
        return;
    
    material->metallic = metallic;
    material->needs_update = true;
}

void material_set_roughness(Material *material, float roughness) {
    if (!material)
        return;
    
    material->roughness = roughness;
    material->needs_update = true;
}

void material_set_texture(Material *material, TextureType type, void *texture) {
    if (!material || type >= TEXTURE_TYPE_COUNT)
        return;
    
    material->textures[type].texture = texture;
    material->textures[type].is_loaded = (texture != NULL);
    material->needs_update = true;
}

void material_bind(Material *material) {
    if (!material || !material->enabled)
        return;
    
    // TODO: Bind shader program
    // TODO: Bind uniform buffer
    // TODO: Bind textures
    
    if (material->needs_update) {
        // TODO: Update uniform buffer
        material->needs_update = false;
    }
}

MaterialInstance *material_create_instance(Material *material) {
    if (!material)
        return NULL;
    
    // Check capacity
    if (material->instance_count >= material->instance_capacity) {
        uint32_t new_capacity = material->instance_capacity == 0 ? 16 : material->instance_capacity * 2;
        MaterialInstance *instances = realloc(material->instances, new_capacity * sizeof(MaterialInstance));
        if (!instances) {
            LOG_ERROR("Failed to resize material instances");
            return NULL;
        }
        material->instances = instances;
        material->instance_capacity = new_capacity;
    }
    
    MaterialInstance *instance = &material->instances[material->instance_count++];
    memset(instance, 0, sizeof(MaterialInstance));
    
    instance->parent = material;
    instance->metallic_override = material->metallic;
    instance->roughness_override = material->roughness;
    instance->ao_override = material->ao;
    instance->alpha_override = material->alpha;
    
    memcpy(instance->albedo_override, material->albedo, sizeof(material->albedo));
    memcpy(instance->emissive_override, material->emissive, sizeof(material->emissive));
    
    // TODO: Create uniform buffer for instance
    
    LOG_DEBUG("Created material instance for: %s", material->name);
    return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
    if (!instance)
        return;
    
    // TODO: Destroy uniform buffer
    memset(instance, 0, sizeof(MaterialInstance));
}

void material_instance_set_albedo(MaterialInstance *instance, float r, float g, float b) {
    if (!instance)
        return;
    
    instance->albedo_override[0] = r;
    instance->albedo_override[1] = g;
    instance->albedo_override[2] = b;
    instance->has_albedo_override = true;
    instance->needs_update = true;
}

void material_instance_set_metallic(MaterialInstance *instance, float metallic) {
    if (!instance)
        return;
    
    instance->metallic_override = metallic;
    instance->has_metallic_override = true;
    instance->needs_update = true;
}

void material_instance_set_roughness(MaterialInstance *instance, float roughness) {
    if (!instance)
        return;
    
    instance->roughness_override = roughness;
    instance->has_roughness_override = true;
    instance->needs_update = true;
}

void material_instance_bind(MaterialInstance *instance) {
    if (!instance || !instance->parent)
        return;
    
    // Bind parent material first
    material_bind(instance->parent);
    
    // Override with instance properties
    if (instance->needs_update) {
        // TODO: Update instance uniform buffer
        instance->needs_update = false;
    }
    
    // TODO: Bind instance uniform buffer
}

Material *material_find(const char *name) {
    if (!g_material_system.initialized || !name)
        return g_material_system.default_material;
    
    for (uint32_t i = 0; i < g_material_system.material_count; i++) {
        if (strcmp(g_material_system.materials[i]->name, name) == 0) {
            return g_material_system.materials[i];
        }
    }
    
    LOG_WARN("Material not found: %s, using default", name);
    return g_material_system.default_material;
}

Material *material_get_default(void) {
    return g_material_system.default_material;
}

Material *material_get_error(void) {
    return g_material_system.error_material;
}
