// Material Serialization - Implementation
#include "materials/material_serialization.h"
#include "core/logger.h"
#include "core/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: Write JSON value for parameter
static void write_param_json(FILE *f, MaterialParameter *param, int indent) {
    for (int i = 0; i < indent; i++) fprintf(f, "  ");
    fprintf(f, "\"%s\": ", param->name);
    
    switch (param->type) {
        case MATERIAL_PARAM_FLOAT:
            fprintf(f, "%.3f", param->value.float_val);
            break;
        case MATERIAL_PARAM_VEC2:
            fprintf(f, "[%.3f, %.3f]", 
                    param->value.vec2_val[0], param->value.vec2_val[1]);
            break;
        case MATERIAL_PARAM_VEC3:
        case MATERIAL_PARAM_COLOR:
            fprintf(f, "[%.3f, %.3f, %.3f]",
                    param->value.vec3_val[0], param->value.vec3_val[1], param->value.vec3_val[2]);
            break;
        case MATERIAL_PARAM_VEC4:
            fprintf(f, "[%.3f, %.3f, %.3f, %.3f]",
                    param->value.vec4_val[0], param->value.vec4_val[1],
                    param->value.vec4_val[2], param->value.vec4_val[3]);
            break;
        case MATERIAL_PARAM_INT:
            fprintf(f, "%d", param->value.int_val);
            break;
        case MATERIAL_PARAM_BOOL:
            fprintf(f, "%s", param->value.bool_val ? "true" : "false");
            break;
        case MATERIAL_PARAM_TEXTURE:
            fprintf(f, "null");  // Texture IDs are runtime, we'll use paths instead
            break;
    }
}

bool material_save_to_file(MaterialInstance *instance, const char *filepath) {
    if (!instance || !filepath) return false;
    
    FILE *f = fopen(filepath, "w");
    if (!f) {
        LOG_ERROR("Failed to open %s for writing", filepath);
        return false;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"material\": {\n");
    fprintf(f, "    \"name\": \"%s\",\n", instance->name);
    fprintf(f, "    \"parent\": \"%s\",\n", instance->parent->name);
    
    // Parameters
    fprintf(f, "    \"parameters\": {\n");
    for (u32 i = 0; i < instance->override_count; i++) {
        MaterialParameter *param = &instance->overrides[i];
        if (param->type != MATERIAL_PARAM_TEXTURE) {  // Skip textures for now
            fprintf(f, "      ");
            write_param_json(f, param, 0);
            if (i < instance->override_count - 1) fprintf(f, ",");
            fprintf(f, "\n");
        }
    }
    fprintf(f, "    },\n");
    
    // Textures (placeholder - would need texture path mapping)
    fprintf(f, "    \"textures\": {\n");
    fprintf(f, "    },\n");
    
    // Active features
    fprintf(f, "    \"features\": [");
    u32 features = instance->active_permutation.features;
    bool first = true;
    if (features & SHADER_FEATURE_NORMAL_MAP) {
        fprintf(f, "\"NORMAL_MAP\"");
        first = false;
    }
    if (features & SHADER_FEATURE_ROUGHNESS_MAP) {
        if (!first) fprintf(f, ", ");
        fprintf(f, "\"ROUGHNESS_MAP\"");
        first = false;
    }
    if (features & SHADER_FEATURE_METALLIC_MAP) {
        if (!first) fprintf(f, ", ");
        fprintf(f, "\"METALLIC_MAP\"");
    }
    fprintf(f, "],\n");
    
    // Quality
    const char *quality_str = "HIGH";
    switch (instance->active_permutation.quality) {
        case SHADER_QUALITY_LOW: quality_str = "LOW"; break;
        case SHADER_QUALITY_MEDIUM: quality_str = "MEDIUM"; break;
        case SHADER_QUALITY_HIGH: quality_str = "HIGH"; break;
        case SHADER_QUALITY_ULTRA: quality_str = "ULTRA"; break;
    }
    fprintf(f, "    \"quality\": \"%s\"\n", quality_str);
    
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    
    fclose(f);
    
    LOG_INFO("Saved material %s to %s", instance->name, filepath);
    return true;
}

// Simple JSON parser for material loading
static f32 parse_float(const char *str) {
    return (f32)atof(str);
}

static void parse_vec3(const char *str, f32 *out) {
    // Simple parser for "[x, y, z]"
    sscanf(str, "[%f, %f, %f]", &out[0], &out[1], &out[2]);
}

MaterialInstance *material_load_from_file(const char *filepath, MaterialMaster *parent) {
    if (!filepath || !parent) return NULL;
    
    // Read file
    FILE *f = fopen(filepath, "r");
    if (!f) {
        LOG_ERROR("Failed to open material file: %s", filepath);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *json_str = (char *)malloc(size + 1);
    fread(json_str, 1, size, f);
    json_str[size] = '\0';
    fclose(f);
    
    // For MVP: Simple manual parsing
    // Production would use proper JSON library
    
    // Extract name from JSON
    char name[128] = "LoadedMaterial";
    const char *name_key = "\"name\": \"";
    const char *name_start = strstr(json_str, name_key);
    if (name_start) {
        name_start += strlen(name_key);
        const char *name_end = strchr(name_start, '"');
        if (name_end) {
            size_t len = name_end - name_start;
            if (len < sizeof(name)) {
                strncpy(name, name_start, len);
                name[len] = '\0';
            }
        }
    }
    
    // Create instance
    MaterialInstance *instance = material_instance_create(parent, name);
    
    // Parse parameters (simplified)
    // In production, use proper JSON parsing
    
    // Look for BaseColor
    const char *base_color_key = "\"BaseColor\": [";
    const char *bc_start = strstr(json_str, base_color_key);
    if (bc_start) {
        bc_start += strlen(base_color_key);
        f32 color[3];
        sscanf(bc_start, "%f, %f, %f", &color[0], &color[1], &color[2]);
        material_instance_set_vec3(instance, "BaseColor", color);
    }
    
    // Look for Roughness
    const char *roughness_key = "\"Roughness\": ";
    const char *r_start = strstr(json_str, roughness_key);
    if (r_start) {
        r_start += strlen(roughness_key);
        f32 roughness = parse_float(r_start);
        material_instance_set_float(instance, "Roughness", roughness);
    }
    
    // Look for Metallic
    const char *metallic_key = "\"Metallic\": ";
    const char *m_start = strstr(json_str, metallic_key);
    if (m_start) {
        m_start += strlen(metallic_key);
        f32 metallic = parse_float(m_start);
        material_instance_set_float(instance, "Metallic", metallic);
    }
    
    free(json_str);
    
    // Compile variant
    material_instance_compile(instance);
    
    LOG_INFO("Loaded material %s from %s", name, filepath);
    return instance;
}

char *material_instance_to_json(MaterialInstance *instance) {
    if (!instance) return NULL;
    
    // Allocate buffer for JSON string
    char *buffer = (char *)malloc(4096);
    size_t offset = 0;
    
    offset += snprintf(buffer + offset, 4096 - offset, "{\n");
    offset += snprintf(buffer + offset, 4096 - offset, "  \"name\": \"%s\",\n", instance->name);
    offset += snprintf(buffer + offset, 4096 - offset, "  \"parent\": \"%s\",\n", instance->parent->name);
    offset += snprintf(buffer + offset, 4096 - offset, "  \"overrides\": %u\n", instance->override_count);
    offset += snprintf(buffer + offset, 4096 - offset, "}\n");
    
    return buffer;
}

// Property metadata generation
MaterialPropertyMetadata *material_get_metadata(MaterialMaster *master, u32 *out_count) {
    if (!master || !out_count) return NULL;
    
    // For now, return static metadata for PBR standard
    static MaterialPropertyMetadata pbr_metadata[] = {
        {
            .name = "BaseColor",
            .display_name = "Base Color",
            .tooltip = "The base diffuse color of the material",
            .type = MATERIAL_PARAM_VEC3,
            .min_value = 0.0f,
            .max_value = 1.0f,
            .is_color = true,
            .is_slider = false,
            .category = "Base"
        },
        {
            .name = "Metallic",
            .display_name = "Metallic",
            .tooltip = "How metallic the surface is (0=dielectric, 1=metal)",
            .type = MATERIAL_PARAM_FLOAT,
            .min_value = 0.0f,
            .max_value = 1.0f,
            .is_color = false,
            .is_slider = true,
            .category = "Base"
        },
        {
            .name = "Roughness",
            .display_name = "Roughness",
            .tooltip = "Surface roughness (0=smooth/shiny, 1=rough/matte)",
            .type = MATERIAL_PARAM_FLOAT,
            .min_value = 0.0f,
            .max_value = 1.0f,
            .is_color = false,
            .is_slider = true,
            .category = "Base"
        },
        {
            .name = "AO",
            .display_name = "Ambient Occlusion",
            .tooltip = "Ambient occlusion multiplier",
            .type = MATERIAL_PARAM_FLOAT,
            .min_value = 0.0f,
            .max_value = 1.0f,
            .is_color = false,
            .is_slider = true,
            .category = "Advanced"
        }
    };
    
    *out_count = sizeof(pbr_metadata) / sizeof(pbr_metadata[0]);
    return pbr_metadata;
}

bool material_master_save_to_file(MaterialMaster *master, const char *filepath) {
    if (!master || !filepath) return false;
    
    FILE *f = fopen(filepath, "w");
    if (!f) {
        LOG_ERROR("Failed to open %s for writing", filepath);
        return false;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"master\": {\n");
    fprintf(f, "    \"name\": \"%s\",\n", master->name);
    fprintf(f, "    \"shader\": \"%s\",\n", master->shader_base_path);
    fprintf(f, "    \"supported_features\": %u,\n", master->supported_features);
    fprintf(f, "    \"default_quality\": %d\n", master->default_quality);
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    
    fclose(f);
    
    LOG_INFO("Saved master material %s to %s", master->name, filepath);
    return true;
}

MaterialMaster *material_master_load_from_file(const char *filepath) {
    // Simplified implementation
    LOG_WARN("material_master_load_from_file not fully implemented");
    return NULL;
}
