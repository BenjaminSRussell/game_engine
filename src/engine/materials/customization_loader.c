/*
 * Material Customization Loader
 * Dynamic material loading and customization system
 */

#include "materials/material_system.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

// Simple Vec3 structure for material system
typedef struct {
    float x, y, z;
} SimpleVec3;

// Material structure
typedef struct {
    int id;
    char name[64];
    SimpleVec3 albedo;
    SimpleVec3 normal;
    SimpleVec3 metallic;
    SimpleVec3 roughness;
    SimpleVec3 emissive;
    float transparency;
    int texture_id;
    int shader_id;
    int loaded;
} Material;

#define MAX_MATERIALS 256
static Material g_materials[MAX_MATERIALS];
static int g_material_count = 0;
static int g_next_material_id = 1;

// Create default materials
void material_create_default(void) {
    // Stone material
    Material stone = {
        .id = g_next_material_id++,
        .name = "Stone",
        .albedo = (SimpleVec3){0.7f, 0.7f, 0.7f},
        .normal = (SimpleVec3){0.5f, 0.5f, 1.0f},
        .metallic = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .roughness = (SimpleVec3){0.8f, 0.8f, 0.8f},
        .emissive = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .transparency = 1.0f,
        .texture_id = 0,
        .shader_id = 0,
        .loaded = 1
    };
    g_materials[g_material_count++] = stone;
    
    // Wood material
    Material wood = {
        .id = g_next_material_id++,
        .name = "Wood",
        .albedo = (SimpleVec3){0.6f, 0.4f, 0.2f},
        .normal = (SimpleVec3){0.5f, 0.5f, 1.0f},
        .metallic = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .roughness = (SimpleVec3){0.6f, 0.6f, 0.6f},
        .emissive = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .transparency = 1.0f,
        .texture_id = 1,
        .shader_id = 0,
        .loaded = 1
    };
    g_materials[g_material_count++] = wood;
    
    // Metal material
    Material metal = {
        .id = g_next_material_id++,
        .name = "Metal",
        .albedo = (SimpleVec3){0.8f, 0.8f, 0.9f},
        .normal = (SimpleVec3){0.5f, 0.5f, 1.0f},
        .metallic = (SimpleVec3){1.0f, 1.0f, 1.0f},
        .roughness = (SimpleVec3){0.1f, 0.1f, 0.1f},
        .emissive = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .transparency = 1.0f,
        .texture_id = 2,
        .shader_id = 1,
        .loaded = 1
    };
    g_materials[g_material_count++] = metal;
    
    // Glass material
    Material glass = {
        .id = g_next_material_id++,
        .name = "Glass",
        .albedo = (SimpleVec3){0.1f, 0.2f, 0.3f},
        .normal = (SimpleVec3){0.5f, 0.5f, 1.0f},
        .metallic = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .roughness = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .emissive = (SimpleVec3){0.0f, 0.0f, 0.0f},
        .transparency = 0.3f,
        .texture_id = 3,
        .shader_id = 2,
        .loaded = 1
    };
    g_materials[g_material_count++] = glass;
    
    LOG_INFO("Default materials created");
}

// Initialize material system
void material_customization_init(void) {
    memset(g_materials, 0, sizeof(g_materials));
    g_material_count = 0;
    g_next_material_id = 1;
    
    material_create_default();
    
    LOG_INFO("Material customization system initialized");
}

// Shutdown material system
void material_customization_shutdown(void) {
    memset(g_materials, 0, sizeof(g_materials));
    g_material_count = 0;
    g_next_material_id = 1;
    
    LOG_INFO("Material customization system shutdown");
}


// Create custom material
int material_create_custom(const char* name, SimpleVec3 albedo, SimpleVec3 normal, 
                      SimpleVec3 metallic, SimpleVec3 roughness, SimpleVec3 emissive,
                      float transparency, int texture_id, int shader_id) {
    if (g_material_count >= MAX_MATERIALS) {
        LOG_ERROR("Material system: Maximum materials reached");
        return -1;
    }
    
    Material* material = &g_materials[g_material_count];
    material->id = g_next_material_id++;
    strncpy(material->name, name ? name : "Custom", sizeof(material->name) - 1);
    material->albedo = albedo;
    material->normal = normal;
    material->metallic = metallic;
    material->roughness = roughness;
    material->emissive = emissive;
    material->transparency = transparency;
    material->texture_id = texture_id;
    material->shader_id = shader_id;
    material->loaded = 1;
    
    g_material_count++;
    LOG_INFO("Custom material created: %s (ID: %d)", material->name, material->id);
    
    return material->id;
}

// Get material by ID
void* material_get(int material_id) {
    for (int i = 0; i < g_material_count; i++) {
        if (g_materials[i].loaded && g_materials[i].id == material_id) {
            return &g_materials[i];
        }
    }
    
    LOG_WARN("Material not found: ID %d", material_id);
    return NULL;
}

// Get material by name
void* material_get_by_name(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; i < g_material_count; i++) {
        if (g_materials[i].loaded && 
            strcmp(g_materials[i].name, name) == 0) {
            return &g_materials[i];
        }
    }
    
    LOG_WARN("Material not found: %s", name);
    return NULL;
}

// Update material properties
void material_update_properties(int material_id, SimpleVec3 albedo, SimpleVec3 normal,
                           SimpleVec3 metallic, SimpleVec3 roughness, SimpleVec3 emissive,
                           float transparency) {
    for (int i = 0; i < g_material_count; i++) {
        if (g_materials[i].loaded && g_materials[i].id == material_id) {
            g_materials[i].albedo = albedo;
            g_materials[i].normal = normal;
            g_materials[i].metallic = metallic;
            g_materials[i].roughness = roughness;
            g_materials[i].emissive = emissive;
            g_materials[i].transparency = transparency;
            
            LOG_DEBUG("Material %d properties updated", material_id);
            return;
        }
    }
    
    LOG_WARN("Material not found for update: ID %d", material_id);
}

// Remove material
void material_remove(int material_id) {
    for (int i = 0; i < g_material_count; i++) {
        if (g_materials[i].loaded && g_materials[i].id == material_id) {
            
            // Move last material to this slot
            if (i < g_material_count - 1) {
                g_materials[i] = g_materials[g_material_count - 1];
            }
            
            g_material_count--;
            LOG_INFO("Material removed: ID %d", material_id);
            return;
        }
    }
    
    LOG_WARN("Material not found for removal: ID %d", material_id);
}

// Get all materials
void* material_get_all(int* count) {
    if (count) {
        *count = g_material_count;
    }
    return g_materials;
}

// Get material count
int material_get_count(void) {
    return g_material_count;
}

// Save materials to file
int material_save_to_file(const char* filename) {
    if (!filename) {
        LOG_ERROR("Material save: NULL filename");
        return 0;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        LOG_ERROR("Material save: Failed to open file %s", filename);
        return 0;
    }
    
    fprintf(file, "# VoxelForge Materials\n");
    fprintf(file, "# Generated by material customization system\n\n");
    
    for (int i = 0; i < g_material_count; i++) {
        if (!g_materials[i].loaded) {
            continue;
        }
        
        Material* mat = &g_materials[i];
        fprintf(file, "material_%s {\n", mat->name);
        fprintf(file, "    id = %d\n", mat->id);
        fprintf(file, "    albedo = %.3f,%.3f,%.3f\n", 
                mat->albedo.x, mat->albedo.y, mat->albedo.z);
        fprintf(file, "    normal = %.3f,%.3f,%.3f\n", 
                mat->normal.x, mat->normal.y, mat->normal.z);
        fprintf(file, "    metallic = %.3f,%.3f,%.3f\n", 
                mat->metallic.x, mat->metallic.y, mat->metallic.z);
        fprintf(file, "    roughness = %.3f,%.3f,%.3f\n", 
                mat->roughness.x, mat->roughness.y, mat->roughness.z);
        fprintf(file, "    emissive = %.3f,%.3f,%.3f\n", 
                mat->emissive.x, mat->emissive.y, mat->emissive.z);
        fprintf(file, "    transparency = %.3f\n", mat->transparency);
        fprintf(file, "    texture_id = %d\n", mat->texture_id);
        fprintf(file, "    shader_id = %d\n", mat->shader_id);
        fprintf(file, "}\n\n");
    }
    
    fclose(file);
    LOG_INFO("Materials saved to %s", filename);
    return 1;
}

// Load materials from file
int material_load_from_file(const char* filename) {
    if (!filename) {
        LOG_ERROR("Material load: NULL filename");
        return 0;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Material load: Failed to open file %s", filename);
        return 0;
    }
    
    // Simple parsing (would need proper parser for production)
    char line[256];
    int loaded_count = 0;
    
    while (fgets(line, sizeof(line), file) && loaded_count < MAX_MATERIALS - g_material_count) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        
        // Simple material creation (stub implementation)
        if (strstr(line, "material_")) {
            Material* mat = &g_materials[g_material_count];
            mat->id = g_next_material_id++;
            mat->loaded = 1;
            
            // Extract name (simplified)
            char* name_start = strstr(line, "material_") + 9;
            char* name_end = strchr(name_start, ' ');
            if (name_end) {
                int name_len = name_end - name_start;
                strncpy(mat->name, name_start, name_len);
                mat->name[name_len] = '\0';
            }
            
            // Set default properties
            mat->albedo = (SimpleVec3){0.5f, 0.5f, 0.5f};
            mat->normal = (SimpleVec3){0.5f, 0.5f, 1.0f};
            mat->metallic = (SimpleVec3){0.0f, 0.0f, 0.0f};
            mat->roughness = (SimpleVec3){0.5f, 0.5f, 0.5f};
            mat->emissive = (SimpleVec3){0.0f, 0.0f, 0.0f};
            mat->transparency = 1.0f;
            mat->texture_id = 0;
            mat->shader_id = 0;
            
            g_material_count++;
            loaded_count++;
        }
    }
    
    fclose(file);
    LOG_INFO("Loaded %d materials from %s", loaded_count, filename);
    return loaded_count;
}
