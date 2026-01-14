#include "materials/material_system.h"
#include "materials/material_instance.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

#define MATERIAL_SYSTEM_MAX_MASTERS 1024
#define MATERIAL_SYSTEM_MAX_INSTANCES 4096
#define MATERIAL_SYSTEM_NAME_LENGTH 128

typedef struct material_registry_t {
    MaterialMaster** masters;
    u32 master_count;
    u32 master_capacity;
    
    MaterialInstance** instances;
    u32 instance_count;
    u32 instance_capacity;
    
    MaterialMaster* default_pbr;
    bool initialized;
} material_registry_t;

static material_registry_t g_material_registry = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static u32 hash_string(const char* str) {
    u32 hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static MaterialMaster* find_master_by_name(const char* name) {
    if (!name) return NULL;
    
    for (u32 i = 0; i < g_material_registry.master_count; i++) {
        if (strcmp(g_material_registry.masters[i]->name, name) == 0) {
            return g_material_registry.masters[i];
        }
    }
    return NULL;
}

static MaterialInstance* find_instance_by_name(const char* name) {
    if (!name) return NULL;
    
    for (u32 i = 0; i < g_material_registry.instance_count; i++) {
        if (strcmp(g_material_registry.instances[i]->name, name) == 0) {
            return g_material_registry.instances[i];
        }
    }
    return NULL;
}

static bool add_master_to_registry(MaterialMaster* master) {
    if (g_material_registry.master_count >= g_material_registry.master_capacity) {
        u32 new_capacity = g_material_registry.master_capacity * 2;
        MaterialMaster** new_masters = realloc(g_material_registry.masters, new_capacity * sizeof(MaterialMaster*));
        if (!new_masters) return false;
        
        g_material_registry.masters = new_masters;
        g_material_registry.master_capacity = new_capacity;
    }
    
    g_material_registry.masters[g_material_registry.master_count++] = master;
    return true;
}

static bool add_instance_to_registry(MaterialInstance* instance) {
    if (g_material_registry.instance_count >= g_material_registry.instance_capacity) {
        u32 new_capacity = g_material_registry.instance_capacity * 2;
        MaterialInstance** new_instances = realloc(g_material_registry.instances, new_capacity * sizeof(MaterialInstance*));
        if (!new_instances) return false;
        
        g_material_registry.instances = new_instances;
        g_material_registry.instance_capacity = new_capacity;
    }
    
    g_material_registry.instances[g_material_registry.instance_count++] = instance;
    return true;
}

static void remove_instance_from_registry(MaterialInstance* instance) {
    for (u32 i = 0; i < g_material_registry.instance_count; i++) {
        if (g_material_registry.instances[i] == instance) {
            // Shift remaining instances
            for (u32 j = i; j < g_material_registry.instance_count - 1; j++) {
                g_material_registry.instances[j] = g_material_registry.instances[j + 1];
            }
            g_material_registry.instance_count--;
            break;
        }
    }
}

// ============================================================================
// DEFAULT MATERIALS
// ============================================================================

static MaterialMaster* create_default_pbr_material(void) {
    MaterialMaster* pbr = material_master_create("DefaultPBR", "assets/shaders/pbr_standard");
    if (!pbr) return NULL;
    
    // Set default PBR parameters
    f32 zero[3] = {0.0f, 0.0f, 0.0f};
    f32 one[3] = {1.0f, 1.0f, 1.0f};
    f32 metallic = 0.0f;
    f32 roughness = 0.5f;
    f32 ao = 1.0f;
    f32 emissive[3] = {0.0f, 0.0f, 0.0f};
    
    material_master_add_param(pbr, "albedo", MATERIAL_PARAM_VEC3, one);
    material_master_add_param(pbr, "normal", MATERIAL_PARAM_TEXTURE, &(TextureID){0});
    material_master_add_param(pbr, "metallic", MATERIAL_PARAM_FLOAT, &metallic);
    material_master_add_param(pbr, "roughness", MATERIAL_PARAM_FLOAT, &roughness);
    material_master_add_param(pbr, "ao", MATERIAL_PARAM_FLOAT, &ao);
    material_master_add_param(pbr, "emissive", MATERIAL_PARAM_VEC3, emissive);
    
    // Set default render states
    pbr->two_sided = false;
    pbr->alpha_blend = false;
    pbr->alpha_test = false;
    
    return pbr;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

bool material_system_init(void) {
    if (g_material_registry.initialized) {
        return true; // Already initialized
    }
    
    memset(&g_material_registry, 0, sizeof(material_registry_t));
    
    // Allocate initial storage
    g_material_registry.master_capacity = 256;
    g_material_registry.instance_capacity = 1024;
    
    g_material_registry.masters = malloc(g_material_registry.master_capacity * sizeof(MaterialMaster*));
    g_material_registry.instances = malloc(g_material_registry.instance_capacity * sizeof(MaterialInstance*));
    
    if (!g_material_registry.masters || !g_material_registry.instances) {
        material_system_shutdown();
        return false;
    }
    
    // Create default PBR material
    g_material_registry.default_pbr = create_default_pbr_material();
    if (g_material_registry.default_pbr) {
        add_master_to_registry(g_material_registry.default_pbr);
    }
    
    g_material_registry.initialized = true;
    
    printf("Material system initialized\n");
    printf("  Masters: %u/%u\n", g_material_registry.master_count, g_material_registry.master_capacity);
    printf("  Instances: %u/%u\n", g_material_registry.instance_count, g_material_registry.instance_capacity);
    
    return true;
}

void material_system_shutdown(void) {
    if (!g_material_registry.initialized) return;
    
    // Destroy all instances
    for (u32 i = 0; i < g_material_registry.instance_count; i++) {
        material_instance_destroy(g_material_registry.instances[i]);
    }
    
    // Destroy all masters
    for (u32 i = 0; i < g_material_registry.master_count; i++) {
        material_master_destroy(g_material_registry.masters[i]);
    }
    
    free(g_material_registry.instances);
    free(g_material_registry.masters);
    
    memset(&g_material_registry, 0, sizeof(material_registry_t));
    
    printf("Material system shutdown\n");
}

bool material_system_register_master(MaterialMaster* master) {
    if (!master || !g_material_registry.initialized) {
        return false;
    }
    
    // Check for duplicate names
    if (find_master_by_name(master->name)) {
        printf("Error: Master material '%s' already exists\n", master->name);
        return false;
    }
    
    if (!add_master_to_registry(master)) {
        printf("Error: Failed to register master material '%s'\n", master->name);
        return false;
    }
    
    printf("Registered master material: %s\n", master->name);
    return true;
}

MaterialInstance* material_system_create_instance(const char* master_name, const char* instance_name) {
    if (!master_name || !instance_name || !g_material_registry.initialized) {
        return NULL;
    }
    
    // Find master material
    MaterialMaster* master = find_master_by_name(master_name);
    if (!master) {
        printf("Error: Master material '%s' not found\n", master_name);
        return NULL;
    }
    
    // Check for duplicate instance names
    if (find_instance_by_name(instance_name)) {
        printf("Error: Material instance '%s' already exists\n", instance_name);
        return NULL;
    }
    
    // Create instance
    MaterialInstance* instance = material_instance_create(master, instance_name);
    if (!instance) {
        printf("Error: Failed to create material instance '%s'\n", instance_name);
        return NULL;
    }
    
    if (!add_instance_to_registry(instance)) {
        material_instance_destroy(instance);
        printf("Error: Failed to register material instance '%s'\n", instance_name);
        return NULL;
    }
    
    printf("Created material instance: %s (from master %s)\n", instance_name, master_name);
    return instance;
}

MaterialMaster* material_system_get_master(const char* name) {
    if (!name || !g_material_registry.initialized) {
        return NULL;
    }
    
    return find_master_by_name(name);
}

MaterialInstance* material_system_get_instance(const char* name) {
    if (!name || !g_material_registry.initialized) {
        return NULL;
    }
    
    return find_instance_by_name(name);
}

void material_system_destroy_instance(MaterialInstance* instance) {
    if (!instance || !g_material_registry.initialized) {
        return;
    }
    
    // Remove from registry
    remove_instance_from_registry(instance);
    
    // Destroy the instance
    material_instance_destroy(instance);
    
    printf("Destroyed material instance: %s\n", instance->name);
}

MaterialMaster* material_system_get_default_pbr(void) {
    if (!g_material_registry.initialized) {
        return NULL;
    }
    
    return g_material_registry.default_pbr;
}

// ============================================================================
// UTILITY AND DEBUG FUNCTIONS
// ============================================================================

void material_system_print_statistics(void) {
    if (!g_material_registry.initialized) {
        printf("Material system not initialized\n");
        return;
    }
    
    printf("Material System Statistics:\n");
    printf("  Masters: %u/%u\n", g_material_registry.master_count, g_material_registry.master_capacity);
    printf("  Instances: %u/%u\n", g_material_registry.instance_count, g_material_registry.instance_capacity);
    printf("  Default PBR: %s\n", g_material_registry.default_pbr ? "Available" : "Not available");
    
    // Count total parameters
    u32 total_master_params = 0;
    u32 total_instance_overrides = 0;
    
    for (u32 i = 0; i < g_material_registry.master_count; i++) {
        total_master_params += g_material_registry.masters[i]->param_count;
    }
    
    for (u32 i = 0; i < g_material_registry.instance_count; i++) {
        total_instance_overrides += g_material_registry.instances[i]->override_count;
    }
    
    printf("  Total master parameters: %u\n", total_master_params);
    printf("  Total instance overrides: %u\n", total_instance_overrides);
}

void material_system_list_masters(void) {
    if (!g_material_registry.initialized) {
        printf("Material system not initialized\n");
        return;
    }
    
    printf("Registered Master Materials (%u):\n", g_material_registry.master_count);
    for (u32 i = 0; i < g_material_registry.master_count; i++) {
        MaterialMaster* master = g_material_registry.masters[i];
        printf("  %s: %s (%u params)\n", master->name, master->shader_base_path, master->param_count);
    }
}

void material_system_list_instances(void) {
    if (!g_material_registry.initialized) {
        printf("Material system not initialized\n");
        return;
    }
    
    printf("Material Instances (%u):\n", g_material_registry.instance_count);
    for (u32 i = 0; i < g_material_registry.instance_count; i++) {
        MaterialInstance* instance = g_material_registry.instances[i];
        printf("  %s: %s (%u overrides)\n", 
               instance->name, 
               instance->parent ? instance->parent->name : "NULL",
               instance->override_count);
    }
}

bool material_system_validate_instance(MaterialInstance* instance) {
    if (!instance || !instance->parent) {
        return false;
    }
    
    // Check if all required parameters are available
    for (u32 i = 0; i < instance->parent->param_count; i++) {
        MaterialParameter* param = &instance->parent->default_params[i];
        MaterialParameter* override = material_instance_get_param(instance, param->name);
        
        if (!override) {
            printf("Warning: Instance '%s' missing parameter '%s'\n", instance->name, param->name);
            return false;
        }
    }
    
    return true;
}
