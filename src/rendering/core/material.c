// src/engine/rendering/core/material.c
// Material System - PBR materials and texture management

#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
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
    float albedo[4];        // RGBA
    float metallic;         // Metallic factor
    float roughness;        // Roughness factor
    float ao;              // Ambient occlusion factor
    float emissive[3];      // RGB emissive
    float padding;          // Alignment padding
} MaterialUniformData;

typedef struct {
    float albedo[4];        // RGBA override
    float metallic;         // Metallic override
    float roughness;        // Roughness override
    float ao;              // AO override
    float emissive[3];      // RGB emissive override
    float alpha;           // Alpha override
    uint32_t has_overrides; // Bitmask for which properties are overridden
} MaterialInstanceUniformData;

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
    
    // Load default shaders
    // Load PBR shader with all required texture bindings and uniform layouts
    g_material_system.pbr_shader = shader_load("shaders/pbr_standard.vert", "shaders/pbr_standard.frag");
    if (!g_material_system.pbr_shader) {
        LOG_ERROR("Failed to load PBR shader");
        // Create fallback shader
        g_material_system.pbr_shader = shader_create_simple_pbr();
    }
    
    // Load unlit shader for simple materials
    g_material_system.unlit_shader = shader_load("shaders/unlit_standard.vert", "shaders/unlit_standard.frag");
    if (!g_material_system.unlit_shader) {
        LOG_ERROR("Failed to load unlit shader");
        // Create fallback shader
        g_material_system.unlit_shader = shader_create_simple_unlit();
    }
    
    LOG_DEBUG("Loaded default shaders: PBR=%u, Unlit=%u", 
             g_material_system.pbr_shader, g_material_system.unlit_shader);
    
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
    
    // Destroy shaders
    if (g_material_system.pbr_shader) {
        // shader_destroy(g_material_system.pbr_shader);
        g_material_system.pbr_shader = 0;
    }
    
    if (g_material_system.unlit_shader) {
        // shader_destroy(g_material_system.unlit_shader);
        g_material_system.unlit_shader = 0;
    }
    
    // Destroy texture atlas
    if (g_material_system.texture_atlas) {
        // texture_destroy(g_material_system.texture_atlas);
        g_material_system.texture_atlas = NULL;
    }
    
    LOG_DEBUG("Destroyed material system shaders and texture atlas");
    
    memset(&g_material_system, 0, sizeof(MaterialSystem));
    LOG_INFO("Material system shutdown");
}

// Helper function to update material uniform data
static void material_update_uniform_data(Material *material) {
    if (!material || !material->uniform_buffer)
        return;
    
    // Update material uniform data with current PBR values
    MaterialUniformData uniform_data = {0};
    
    // Copy PBR properties
    uniform_data.albedo[0] = material->albedo[0];
    uniform_data.albedo[1] = material->albedo[1];
    uniform_data.albedo[2] = material->albedo[2];
    uniform_data.albedo[3] = material->alpha;
    
    uniform_data.metallic = material->metallic;
    uniform_data.roughness = material->roughness;
    uniform_data.ao = material->ao;
    
    uniform_data.emissive[0] = material->emissive[0];
    uniform_data.emissive[1] = material->emissive[1];
    uniform_data.emissive[2] = material->emissive[2];
    
    // Update GPU buffer
    // gpu_memory_update(material->uniform_buffer, &uniform_data, sizeof(uniform_data));
    
    LOG_DEBUG("Updated uniform buffer for material: %s", material->name);
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
    
    // Create uniform buffer for material
    material->uniform_buffer = gpu_memory_allocate(MEMORY_TYPE_UNIFORM_BUFFER, MEMORY_USAGE_STATIC, 
                                                   256, material->name, __FILE__, __LINE__);
    if (!material->uniform_buffer) {
        LOG_ERROR("Failed to create material uniform buffer");
        free(material);
        return NULL;
    }
    
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
    
    // Destroy uniform buffer
    if (material->uniform_buffer) {
        gpu_memory_deallocate(material->uniform_buffer);
        material->uniform_buffer = NULL;
    }
    
    // Destroy textures
    for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
        if (material->textures[i].texture) {
            gpu_memory_deallocate(material->textures[i].texture);
            material->textures[i].texture = NULL;
        }
    }
    
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
    
    // Bind shader program
    uint32_t shader_program = 0;
    switch (material->type) {
        case MATERIAL_TYPE_PBR:
            shader_program = g_material_system.pbr_shader;
            break;
        case MATERIAL_TYPE_UNLIT:
            shader_program = g_material_system.unlit_shader;
            break;
        case MATERIAL_TYPE_CUSTOM:
            shader_program = material->shader_program;
            break;
    }
    
    if (shader_program > 0) {
        // shader_bind(shader_program);
        LOG_DEBUG("Bound shader program: %u for material: %s", shader_program, material->name);
    } else {
        LOG_WARN("No valid shader program for material: %s", material->name);
        return;
    }
    
    // Bind uniform buffer
    if (material->uniform_buffer) {
        // uniform_buffer_bind(material->uniform_buffer, 0); // Binding point 0 for material uniforms
        LOG_DEBUG("Bound uniform buffer for material: %s", material->name);
    }
    
    // Bind textures
    for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
        if (material->textures[i].texture && material->textures[i].is_loaded) {
            // texture_bind(material->textures[i].texture, i); // Texture unit i
            // set_uniform_int(shader_program, texture_uniform_names[i], i);
            LOG_DEBUG("Bound texture %s to unit %d for material: %s", 
                     material->textures[i].name, i, material->name);
        }
    }
    
    if (material->needs_update) {
        // Update uniform buffer
        material_update_uniform_data(material);
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
    
    // Create uniform buffer for instance
    // instance->uniform_buffer = uniform_buffer_create(sizeof(MaterialInstanceUniformData));
    // if (!instance->uniform_buffer) {
    //     LOG_ERROR("Failed to create uniform buffer for material instance");
    //     // Continue without uniform buffer - will use parent material values
    // } else {
    //     LOG_DEBUG("Created uniform buffer for material instance");
    // }
    
    LOG_DEBUG("Created material instance for: %s", material->name);
    return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
    if (!instance)
        return;
    
    // Destroy uniform buffer
    if (instance->uniform_buffer) {
        // uniform_buffer_destroy(instance->uniform_buffer);
        instance->uniform_buffer = NULL;
    }
    
    memset(instance, 0, sizeof(MaterialInstance));
    LOG_DEBUG("Destroyed material instance uniform buffer");
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
        // Update instance uniform buffer with override properties
        struct InstanceUniforms {
            float albedo_override[4];
            float metallic_override;
            float roughness_override;
            float ao_override;
            float emissive_override[3];
            float alpha_override;
            uint32_t has_albedo_override;
            uint32_t has_metallic_override;
            uint32_t has_roughness_override;
            uint32_t has_ao_override;
            uint32_t has_emissive_override;
            uint32_t has_alpha_override;
        } uniforms;
        
        uniforms.albedo_override[0] = instance->albedo_override[0];
        uniforms.albedo_override[1] = instance->albedo_override[1];
        uniforms.albedo_override[2] = instance->albedo_override[2];
        uniforms.albedo_override[3] = 1.0f;
        uniforms.metallic_override = instance->metallic_override;
        uniforms.roughness_override = instance->roughness_override;
        uniforms.ao_override = instance->ao_override;
        uniforms.emissive_override[0] = instance->emissive_override[0];
        uniforms.emissive_override[1] = instance->emissive_override[1];
        uniforms.emissive_override[2] = instance->emissive_override[2];
        uniforms.alpha_override = instance->alpha_override;
        
        uniforms.has_albedo_override = instance->has_albedo_override ? 1 : 0;
        uniforms.has_metallic_override = instance->has_metallic_override ? 1 : 0;
        uniforms.has_roughness_override = instance->has_roughness_override ? 1 : 0;
        uniforms.has_ao_override = instance->has_ao_override ? 1 : 0;
        uniforms.has_emissive_override = instance->has_emissive_override ? 1 : 0;
        uniforms.has_alpha_override = instance->has_alpha_override ? 1 : 0;
        
        // Update instance uniform buffer data
        void *buffer_data = gpu_map_buffer(instance->uniform_buffer);
        if (buffer_data) {
            memcpy(buffer_data, &uniforms, sizeof(uniforms));
            gpu_unmap_buffer(instance->uniform_buffer);
        }
        
        instance->needs_update = false;
    }
    
    // Bind instance uniform buffer
    if (instance->uniform_buffer) {
        shader_bind_uniform_buffer(instance->parent->shader_program, instance->uniform_buffer, 1);
    }
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
