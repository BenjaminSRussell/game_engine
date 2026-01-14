#ifndef MATERIAL_TRANSPILER_H
#define MATERIAL_TRANSPILER_H

#include "include/assets/import/unreal_asset_importer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// PBR Material Structure
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    float base_color[3];
    float metallic;
    float roughness;
    float specular;
    float emissive[3];
    float normal_scale;
    float occlusion_strength;
    
    // Texture references
    char base_color_texture[MAX_UNREAL_NAME_LENGTH];
    char normal_texture[MAX_UNREAL_NAME_LENGTH];
    char orm_texture[MAX_UNREAL_NAME_LENGTH];  // Occlusion-Roughness-Metallic
    char emissive_texture[MAX_UNREAL_NAME_LENGTH];
    char occlusion_texture[MAX_UNREAL_NAME_LENGTH];
    
    bool has_base_color_texture;
    bool has_normal_texture;
    bool has_orm_texture;
    bool has_emissive_texture;
    bool has_occlusion_texture;
} PBRMaterial;

// Shader node types for engine shader generation
typedef enum {
    SHADER_NODE_TEXTURE_SAMPLE = 1,
    SHADER_NODE_CONSTANT = 2,
    SHADER_NODE_ADD = 3,
    SHADER_NODE_MULTIPLY = 4,
    SHADER_NODE_LERP = 5,
    SHADER_NODE_NORMALIZE = 6,
    SHADER_NODE_DOT_PRODUCT = 7,
    SHADER_NODE_CROSS_PRODUCT = 8,
    SHADER_NODE_PBR = 9
} ShaderNodeType;

/**
 * Material Transpiler Context
 * Main structure for managing material to shader transpilation
 */
typedef struct {
    UnrealPackage* package;
    UnrealMaterialData* material_data;
    char vertex_shader[4096];
    char fragment_shader[8192];
    char error_message[512];
    bool transpilation_successful;
} MaterialTranspilerContext;

// Core API Functions
MaterialTranspilerContext* Material_CreateTranspiler(UnrealPackage* package, const char* material_name);
void Material_DestroyTranspiler(MaterialTranspilerContext* context);
bool Material_TranspileToShader(MaterialTranspilerContext* context);
bool Material_SaveShader(MaterialTranspilerContext* context, const char* output_path);

// PBR Conversion Functions
bool Material_ConvertToPBR(UnrealMaterialData* material, PBRMaterial* pbr_material);
bool Material_SavePBRMaterial(PBRMaterial* pbr_material, const char* output_path);

// Material Node Mapping
ShaderNodeType Material_MapMaterialNodeType(UnrealMaterialNodeType material_type);

// Utility Functions
bool Material_ValidateMaterial(UnrealMaterialData* material);
bool Material_ValidateShaderCode(const char* vertex_shader, const char* fragment_shader);
const char* Material_GetErrorString(MaterialTranspilerContext* context);

// Memory Management
void Unreal_FreeMaterialData(UnrealMaterialData* material);

#endif // MATERIAL_TRANSPILER_H
