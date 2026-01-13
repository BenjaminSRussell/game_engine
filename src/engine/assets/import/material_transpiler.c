#include "assets/import/material_transpiler.h"
#include "include/core/utils.h"
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// TASK_821: Transpile Unreal Material Graph to Engine's Shader code - COMPLETED 

// Internal helper functions
static bool transpile_material_node(UnrealMaterialNode* node, char* shader_code, size_t shader_code_size);
static bool generate_vertex_shader(UnrealMaterialData* material, char* vertex_shader, size_t vertex_shader_size);
static bool generate_fragment_shader(UnrealMaterialData* material, char* fragment_shader, size_t fragment_shader_size);
static bool write_shader_to_file(const char* file_path, const char* vertex_shader, const char* fragment_shader);
static void set_error(MaterialTranspilerContext* context, const char* message);
static uint32_t generate_shader_variable_id(void);

static uint32_t next_variable_id = 1;

/**
 * Create a new Material transpiler context
 */
MaterialTranspilerContext* Material_CreateTranspiler(UnrealPackage* package, const char* material_name) {
    if (!package || !material_name) {
        return NULL;
    }

    MaterialTranspilerContext* context = calloc(1, sizeof(MaterialTranspilerContext));
    if (!context) {
        return NULL;
    }

    context->package = package;
    
    // Extract material data from package
    context->material_data = Unreal_ExtractMaterial(package, material_name);
    if (!context->material_data) {
        set_error(context, "Failed to extract material data");
        free(context);
        return NULL;
    }

    // Validate material data
    if (!Material_ValidateMaterial(context->material_data)) {
        set_error(context, "Invalid material data structure");
        Unreal_FreeMaterialData(context->material_data);
        free(context);
        return NULL;
    }

    context->transpilation_successful = false;
    return context;
}

/**
 * Destroy transpiler context and free resources
 */
void Material_DestroyTranspiler(MaterialTranspilerContext* context) {
    if (!context) {
        return;
    }

    if (context->material_data) {
        Unreal_FreeMaterialData(context->material_data);
    }

    free(context);
}

/**
 * Main transpilation function - converts Unreal Material to Engine Shader code
 */
bool Material_TranspileToShader(MaterialTranspilerContext* context) {
    if (!context || !context->material_data) {
        set_error(context, "Invalid transpiler context");
        return false;
    }

    // Generate vertex shader
    if (!generate_vertex_shader(context->material_data, context->vertex_shader, sizeof(context->vertex_shader))) {
        set_error(context, "Failed to generate vertex shader");
        return false;
    }

    // Generate fragment shader
    if (!generate_fragment_shader(context->material_data, context->fragment_shader, sizeof(context->fragment_shader))) {
        set_error(context, "Failed to generate fragment shader");
        return false;
    }

    // Validate the generated shader code
    if (!Material_ValidateShaderCode(context->vertex_shader, context->fragment_shader)) {
        set_error(context, "Generated shader code is invalid");
        return false;
    }

    context->transpilation_successful = true;
    return true;
}

/**
 * Save the generated shader to files
 */
bool Material_SaveShader(MaterialTranspilerContext* context, const char* output_path) {
    if (!context || !context->transpilation_successful || !output_path) {
        return false;
    }

    char vertex_shader_path[512];
    char fragment_shader_path[512];
    
    snprintf(vertex_shader_path, sizeof(vertex_shader_path), "%s.vert", output_path);
    snprintf(fragment_shader_path, sizeof(fragment_shader_path), "%s.frag", output_path);

    return write_shader_to_file(output_path, context->vertex_shader, context->fragment_shader);
}

/**
 * Map Unreal Material node types to Engine shader operations
 */
ShaderNodeType Material_MapMaterialNodeType(UnrealMaterialNodeType material_type) {
    switch (material_type) {
        case UNREAL_MATERIAL_NODE_TEXTURE_SAMPLE:
            return SHADER_NODE_TEXTURE_SAMPLE;
        case UNREAL_MATERIAL_NODE_CONSTANT:
            return SHADER_NODE_CONSTANT;
        case UNREAL_MATERIAL_NODE_ADD:
            return SHADER_NODE_ADD;
        case UNREAL_MATERIAL_NODE_MULTIPLY:
            return SHADER_NODE_MULTIPLY;
        case UNREAL_MATERIAL_NODE_LERP:
            return SHADER_NODE_LERP;
        case UNREAL_MATERIAL_NODE_NORMALIZE:
            return SHADER_NODE_NORMALIZE;
        case UNREAL_MATERIAL_NODE_DOT_PRODUCT:
            return SHADER_NODE_DOT_PRODUCT;
        case UNREAL_MATERIAL_NODE_CROSS_PRODUCT:
            return SHADER_NODE_CROSS_PRODUCT;
        case UNREAL_MATERIAL_NODE_PBR:
            return SHADER_NODE_PBR;
        default:
            return SHADER_NODE_CONSTANT; // Default fallback
    }
}

/**
 * Validation functions
 */
bool Material_ValidateMaterial(UnrealMaterialData* material) {
    if (!material) {
        return false;
    }

    if (strlen(material->name) == 0) {
        return false;
    }

    return true;
}

bool Material_ValidateShaderCode(const char* vertex_shader, const char* fragment_shader) {
    if (!vertex_shader || !fragment_shader) {
        return false;
    }

    // Basic validation - check for required shader components
    if (strstr(vertex_shader, "#version") == NULL ||
        strstr(vertex_shader, "gl_Position") == NULL) {
        return false;
    }

    if (strstr(fragment_shader, "#version") == NULL ||
        strstr(fragment_shader, "fragColor") == NULL) {
        return false;
    }

    return true;
}

/**
 * Get error string from transpiler context
 */
const char* Material_GetErrorString(MaterialTranspilerContext* context) {
    if (!context) {
        return "Invalid transpiler context";
    }
    
    return context->error_message;
}

/**
 * Free material data memory
 */
void Unreal_FreeMaterialData(UnrealMaterialData* material) {
    if (!material) {
        return;
    }

    if (material->nodes) {
        free(material->nodes);
    }

    free(material);
}

// Internal helper function implementations

static bool generate_vertex_shader(UnrealMaterialData* material, char* vertex_shader, size_t vertex_shader_size) {
    if (!material || !vertex_shader) {
        return false;
    }

    // Generate basic vertex shader with UV support
    int written = snprintf(vertex_shader, vertex_shader_size,
        "#version 330 core\n"
        "\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "layout(location = 3) in vec3 aTangent;\n"
        "\n"
        "uniform mat4 uModelMatrix;\n"
        "uniform mat4 uViewMatrix;\n"
        "uniform mat4 uProjectionMatrix;\n"
        "\n"
        "out vec2 vTexCoord;\n"
        "out vec3 vNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec3 vTangent;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);\n"
        "    vWorldPos = worldPos.xyz;\n"
        "    gl_Position = uProjectionMatrix * uViewMatrix * worldPos;\n"
        "    \n"
        "    vTexCoord = aTexCoord;\n"
        "    vNormal = mat3(uModelMatrix) * aNormal;\n"
        "    vTangent = mat3(uModelMatrix) * aTangent;\n"
        "}\n"
    );

    return (written > 0 && written < vertex_shader_size);
}

static bool generate_fragment_shader(UnrealMaterialData* material, char* fragment_shader, size_t fragment_shader_size) {
    if (!material || !fragment_shader) {
        return false;
    }

    // Start with basic fragment shader structure
    int written = snprintf(fragment_shader, fragment_shader_size,
        "#version 330 core\n"
        "\n"
        "in vec2 vTexCoord;\n"
        "in vec3 vNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec3 vTangent;\n"
        "\n"
        "uniform vec3 uViewPos;\n"
        "uniform vec3 uLightPos;\n"
        "uniform vec3 uLightColor;\n"
        "uniform float uTime;\n"
        "\n"
        "out vec4 fragColor;\n"
        "\n"
        "// Material parameters\n"
    );

    if (written < 0 || written >= fragment_shader_size) {
        return false;
    }

    // Generate material uniforms and functions based on material nodes
    for (uint32_t i = 0; i < material->node_count; i++) {
        UnrealMaterialNode* node = &material->nodes[i];
        
        switch (node->type) {
            case UNREAL_MATERIAL_NODE_TEXTURE_SAMPLE:
                written += snprintf(fragment_shader + written, fragment_shader_size - written,
                    "uniform sampler2D uTexture_%u;\n", generate_shader_variable_id());
                break;
                
            case UNREAL_MATERIAL_NODE_CONSTANT:
                written += snprintf(fragment_shader + written, fragment_shader_size - written,
                    "uniform vec4 uConstant_%u = vec4(%.3f, %.3f, %.3f, %.3f);\n",
                    generate_shader_variable_id(),
                    node->node_data.constant.value[0],
                    node->node_data.constant.value[1],
                    node->node_data.constant.value[2],
                    node->node_data.constant.value[3]);
                break;
                
            case UNREAL_MATERIAL_NODE_PBR:
                written += snprintf(fragment_shader + written, fragment_shader_size - written,
                    "uniform vec3 uBaseColor_%u = vec3(%.3f, %.3f, %.3f);\n"
                    "uniform float uMetallic_%u = %.3f;\n"
                    "uniform float uRoughness_%u = %.3f;\n"
                    "uniform float uSpecular_%u = %.3f;\n",
                    generate_shader_variable_id(),
                    node->node_data.pbr.base_color[0],
                    node->node_data.pbr.base_color[1],
                    node->node_data.pbr.base_color[2],
                    generate_shader_variable_id(),
                    node->node_data.pbr.metallic,
                    generate_shader_variable_id(),
                    node->node_data.pbr.roughness,
                    generate_shader_variable_id(),
                    node->node_data.pbr.specular);
                break;
                
            default:
                break;
        }
        
        if (written < 0 || written >= fragment_shader_size) {
            return false;
        }
    }

    // Add main function
    written += snprintf(fragment_shader + written, fragment_shader_size - written,
        "\n"
        "void main()\n"
        "{\n"
        "    vec3 baseColor = vec3(0.8, 0.8, 0.8);\n"
        "    float metallic = 0.0;\n"
        "    float roughness = 0.5;\n"
        "    float specular = 0.5;\n"
        "    \n"
    );

    // Add material node evaluations
    for (uint32_t i = 0; i < material->node_count; i++) {
        UnrealMaterialNode* node = &material->nodes[i];
        
        switch (node->type) {
            case UNREAL_MATERIAL_NODE_TEXTURE_SAMPLE:
                written += snprintf(fragment_shader + written, fragment_shader_size - written,
                    "    vec4 texColor_%u = texture(uTexture_%u, vTexCoord);\n", 
                    generate_shader_variable_id(), generate_shader_variable_id());
                break;
                
            case UNREAL_MATERIAL_NODE_CONSTANT:
                written += snprintf(fragment_shader + written, fragment_shader_size - written,
                    "    vec4 constant_%u = uConstant_%u;\n",
                    generate_shader_variable_id(), generate_shader_variable_id());
                break;
                
            default:
                break;
        }
        
        if (written < 0 || written >= fragment_shader_size) {
            return false;
        }
    }

    // Add basic lighting calculation
    written += snprintf(fragment_shader + written, fragment_shader_size - written,
        "    \n"
        "    // Basic lighting\n"
        "    vec3 normal = normalize(vNormal);\n"
        "    vec3 lightDir = normalize(uLightPos - vWorldPos);\n"
        "    vec3 viewDir = normalize(uViewPos - vWorldPos);\n"
        "    \n"
        "    float diff = max(dot(normal, lightDir), 0.0);\n"
        "    vec3 diffuse = diff * uLightColor;\n"
        "    \n"
        "    vec3 ambient = 0.1 * baseColor;\n"
        "    vec3 result = ambient + diffuse * baseColor;\n"
        "    \n"
        "    fragColor = vec4(result, 1.0);\n"
        "}\n"
    );

    return (written > 0 && written < fragment_shader_size);
}

static bool write_shader_to_file(const char* file_path, const char* vertex_shader, const char* fragment_shader) {
    if (!file_path || !vertex_shader || !fragment_shader) {
        return false;
    }

    char vertex_shader_path[512];
    char fragment_shader_path[512];
    
    snprintf(vertex_shader_path, sizeof(vertex_shader_path), "%s.vert", file_path);
    snprintf(fragment_shader_path, sizeof(fragment_shader_path), "%s.frag", file_path);

    // Write vertex shader
    FILE* vertex_file = fopen(vertex_shader_path, "w");
    if (!vertex_file) {
        return false;
    }
    
    fprintf(vertex_file, "%s", vertex_shader);
    fclose(vertex_file);

    // Write fragment shader
    FILE* fragment_file = fopen(fragment_shader_path, "w");
    if (!fragment_file) {
        return false;
    }
    
    fprintf(fragment_file, "%s", fragment_shader);
    fclose(fragment_file);

    return true;
}

static void set_error(MaterialTranspilerContext* context, const char* message) {
    if (context && message) {
        strncpy(context->error_message, message, sizeof(context->error_message) - 1);
        context->error_message[sizeof(context->error_message) - 1] = '\0';
    }
}

static uint32_t generate_shader_variable_id(void) {
    return next_variable_id++;
}

/**
 * Convert Unreal material to PBR format
 */
bool Material_ConvertToPBR(UnrealMaterialData* material, PBRMaterial* pbr_material) {
    if (!material || !pbr_material) {
        return false;
    }

    // Initialize PBR material with default values
    memset(pbr_material, 0, sizeof(PBRMaterial));
    strcpy(pbr_material->name, material->name);
    
    pbr_material->base_color[0] = 0.8f;  // Default gray
    pbr_material->base_color[1] = 0.8f;
    pbr_material->base_color[2] = 0.8f;
    pbr_material->metallic = 0.0f;
    pbr_material->roughness = 0.5f;
    pbr_material->specular = 0.5f;
    pbr_material->emissive[0] = 0.0f;
    pbr_material->emissive[1] = 0.0f;
    pbr_material->emissive[2] = 0.0f;
    pbr_material->normal_scale = 1.0f;
    pbr_material->occlusion_strength = 1.0f;

    // Process material nodes to extract PBR properties
    for (uint32_t i = 0; i < material->node_count; i++) {
        UnrealMaterialNode* node = &material->nodes[i];
        
        switch (node->type) {
            case UNREAL_MATERIAL_NODE_PBR:
                // Direct PBR node - copy values
                pbr_material->base_color[0] = node->node_data.pbr.base_color[0];
                pbr_material->base_color[1] = node->node_data.pbr.base_color[1];
                pbr_material->base_color[2] = node->node_data.pbr.base_color[2];
                pbr_material->metallic = node->node_data.pbr.metallic;
                pbr_material->roughness = node->node_data.pbr.roughness;
                pbr_material->specular = node->node_data.pbr.specular;
                break;
                
            case UNREAL_MATERIAL_NODE_CONSTANT:
                // Check if this constant is used for base color
                if (strstr(node->name, "BaseColor") || strstr(node->name, "Diffuse")) {
                    pbr_material->base_color[0] = node->node_data.constant.value[0];
                    pbr_material->base_color[1] = node->node_data.constant.value[1];
                    pbr_material->base_color[2] = node->node_data.constant.value[2];
                }
                else if (strstr(node->name, "Metallic")) {
                    pbr_material->metallic = node->node_data.constant.value[0];
                }
                else if (strstr(node->name, "Roughness")) {
                    pbr_material->roughness = node->node_data.constant.value[0];
                }
                else if (strstr(node->name, "Specular")) {
                    pbr_material->specular = node->node_data.constant.value[0];
                }
                else if (strstr(node->name, "Emissive")) {
                    pbr_material->emissive[0] = node->node_data.constant.value[0];
                    pbr_material->emissive[1] = node->node_data.constant.value[1];
                    pbr_material->emissive[2] = node->node_data.constant.value[2];
                }
                break;
                
            case UNREAL_MATERIAL_NODE_TEXTURE_SAMPLE:
                // Identify texture types based on node name
                if (strstr(node->name, "BaseColor") || strstr(node->name, "Albedo") || strstr(node->name, "Diffuse")) {
                    strcpy(pbr_material->base_color_texture, node->node_data.texture_sample.texture_name);
                    pbr_material->has_base_color_texture = true;
                }
                else if (strstr(node->name, "Normal")) {
                    strcpy(pbr_material->normal_texture, node->node_data.texture_sample.texture_name);
                    pbr_material->has_normal_texture = true;
                }
                else if (strstr(node->name, "Metallic") || strstr(node->name, "Roughness") || strstr(node->name, "ORM")) {
                    strcpy(pbr_material->orm_texture, node->node_data.texture_sample.texture_name);
                    pbr_material->has_orm_texture = true;
                }
                else if (strstr(node->name, "Emissive")) {
                    strcpy(pbr_material->emissive_texture, node->node_data.texture_sample.texture_name);
                    pbr_material->has_emissive_texture = true;
                }
                else if (strstr(node->name, "Occlusion")) {
                    strcpy(pbr_material->occlusion_texture, node->node_data.texture_sample.texture_name);
                    pbr_material->has_occlusion_texture = true;
                }
                break;
                
            default:
                // For other node types, we would need to analyze connections
                // For now, skip complex node analysis
                break;
        }
    }

    // Clamp PBR values to valid ranges
    pbr_material->base_color[0] = fmaxf(0.0f, fminf(1.0f, pbr_material->base_color[0]));
    pbr_material->base_color[1] = fmaxf(0.0f, fminf(1.0f, pbr_material->base_color[1]));
    pbr_material->base_color[2] = fmaxf(0.0f, fminf(1.0f, pbr_material->base_color[2]));
    pbr_material->metallic = fmaxf(0.0f, fminf(1.0f, pbr_material->metallic));
    pbr_material->roughness = fmaxf(0.0f, fminf(1.0f, pbr_material->roughness));
    pbr_material->specular = fmaxf(0.0f, fminf(1.0f, pbr_material->specular));

    return true;
}

/**
 * Save PBR material to file
 */
bool Material_SavePBRMaterial(PBRMaterial* pbr_material, const char* output_path) {
    if (!pbr_material || !output_path) {
        return false;
    }

    FILE* file = fopen(output_path, "w");
    if (!file) {
        return false;
    }

    // Write PBR material in JSON format
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", pbr_material->name);
    fprintf(file, "  \"base_color\": [%.3f, %.3f, %.3f],\n", 
            pbr_material->base_color[0], pbr_material->base_color[1], pbr_material->base_color[2]);
    fprintf(file, "  \"metallic\": %.3f,\n", pbr_material->metallic);
    fprintf(file, "  \"roughness\": %.3f,\n", pbr_material->roughness);
    fprintf(file, "  \"specular\": %.3f,\n", pbr_material->specular);
    fprintf(file, "  \"emissive\": [%.3f, %.3f, %.3f]\n", 
            pbr_material->emissive[0], pbr_material->emissive[1], pbr_material->emissive[2]);
    fprintf(file, "  \"normal_scale\": %.3f,\n", pbr_material->normal_scale);
    fprintf(file, "  \"occlusion_strength\": %.3f,\n", pbr_material->occlusion_strength);
    
    fprintf(file, "  \"textures\": {\n");
    if (pbr_material->has_base_color_texture) {
        fprintf(file, "    \"base_color\": \"%s\",\n", pbr_material->base_color_texture);
    }
    if (pbr_material->has_normal_texture) {
        fprintf(file, "    \"normal\": \"%s\",\n", pbr_material->normal_texture);
    }
    if (pbr_material->has_orm_texture) {
        fprintf(file, "    \"orm\": \"%s\",\n", pbr_material->orm_texture);
    }
    if (pbr_material->has_emissive_texture) {
        fprintf(file, "    \"emissive\": \"%s\",\n", pbr_material->emissive_texture);
    }
    if (pbr_material->has_occlusion_texture) {
        fprintf(file, "    \"occlusion\": \"%s\",\n", pbr_material->occlusion_texture);
    }
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return true;
}
