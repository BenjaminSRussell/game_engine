/**
 * @file render_material.h
 * @brief Material system for 3D rendering
 * @details Manages materials, shaders, material parameters, and material properties
 */

#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Material blend mode
 */
typedef enum {
    BLEND_MODE_OPAQUE = 0,
    BLEND_MODE_MASKED = 1,
    BLEND_MODE_TRANSLUCENT = 2,
    BLEND_MODE_ADDITIVE = 3,
    BLEND_MODE_MODULATE = 4,
} BlendMode;

/**
 * @brief Shader stage
 */
typedef enum {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_FRAGMENT = 0x00000010,
    SHADER_STAGE_GEOMETRY = 0x00000020,
    SHADER_STAGE_COMPUTE = 0x00000040,
    SHADER_STAGE_TASK = 0x00000080,
    SHADER_STAGE_MESH = 0x00000100,
    SHADER_STAGE_RAYGEN = 0x00000200,
    SHADER_STAGE_ANY_HIT = 0x00000400,
    SHADER_STAGE_CLOSEST_HIT = 0x00000800,
    SHADER_STAGE_MISS = 0x00001000,
} ShaderStageFlags;

/**
 * @brief Rasterization state
 */
typedef enum {
    CULL_MODE_NONE = 0,
    CULL_MODE_FRONT = 1,
    CULL_MODE_BACK = 2,
    CULL_MODE_FRONT_AND_BACK = 3,
} CullMode;

/**
 * @brief Depth comparison function
 */
typedef enum {
    COMPARE_FUNC_NEVER = 0,
    COMPARE_FUNC_LESS = 1,
    COMPARE_FUNC_EQUAL = 2,
    COMPARE_FUNC_LESS_OR_EQUAL = 3,
    COMPARE_FUNC_GREATER = 4,
    COMPARE_FUNC_NOT_EQUAL = 5,
    COMPARE_FUNC_GREATER_OR_EQUAL = 6,
    COMPARE_FUNC_ALWAYS = 7,
} CompareFunc;

/**
 * @brief Shader source code
 */
typedef struct {
    const char* vertex_source;
    const char* fragment_source;
    const char* compute_source;
    const char* tessellation_control_source;
    const char* tessellation_evaluation_source;
    const char* geometry_source;
} ShaderSource;

/**
 * @brief Shader structure
 */
typedef struct {
    uint32_t id;
    ShaderStageFlags stages;
    void* compiled_bytecode;
    size_t bytecode_size;
    uint32_t descriptor_set_count;
} Shader;

/**
 * @brief Material structure
 */
typedef struct {
    uint32_t id;
    uint32_t shader_id;
    const char* name;
    BlendMode blend_mode;
    bool depth_test_enabled;
    bool depth_write_enabled;
    CompareFunc depth_compare;
    CullMode cull_mode;
    bool backface_winding_ccw;
} Material;

/**
 * @brief Material parameter value
 */
typedef union {
    float f32;
    int32_t i32;
    uint32_t u32;
    float vec2[2];
    float vec3[3];
    float vec4[4];
    float mat4[16];
} MaterialParameterValue;

/**
 * @brief Material parameter type
 */
typedef enum {
    PARAM_TYPE_FLOAT = 0,
    PARAM_TYPE_INT = 1,
    PARAM_TYPE_UINT = 2,
    PARAM_TYPE_VEC2 = 3,
    PARAM_TYPE_VEC3 = 4,
    PARAM_TYPE_VEC4 = 5,
    PARAM_TYPE_MAT4 = 6,
    PARAM_TYPE_SAMPLER = 7,
    PARAM_TYPE_IMAGE = 8,
} MaterialParameterType;

/**
 * @brief Material instance for per-object material data
 */
typedef struct {
    uint32_t id;
    uint32_t material_id;
    MaterialParameterValue* parameters;
    uint32_t parameter_count;
} MaterialInstance;

/**
 * @brief Shader compiler structure
 */
typedef struct RenderShaderCompiler RenderShaderCompiler;

/**
 * @brief Material manager structure
 */
typedef struct RenderMaterialManager RenderMaterialManager;

/**
 * @brief Create a shader compiler
 * @return Pointer to compiler, NULL on failure
 */
RenderShaderCompiler* render_shader_compiler_create(void);

/**
 * @brief Destroy shader compiler
 * @param[in] compiler Compiler to destroy
 */
void render_shader_compiler_destroy(RenderShaderCompiler* compiler);

/**
 * @brief Compile shader from source
 * @param[in] compiler Compiler instance
 * @param[in] source Source code
 * @return Pointer to compiled shader, NULL on failure
 */
Shader* render_shader_compile(RenderShaderCompiler* compiler, const ShaderSource* source);

/**
 * @brief Create material manager
 * @param[in] max_materials Maximum number of materials
 * @param[in] max_instances Maximum material instances
 * @return Pointer to manager, NULL on failure
 */
RenderMaterialManager* render_material_manager_create(uint32_t max_materials,
                                                      uint32_t max_instances);

/**
 * @brief Destroy material manager
 * @param[in] manager Manager to destroy
 */
void render_material_manager_destroy(RenderMaterialManager* manager);

/**
 * @brief Create a material
 * @param[in] manager Material manager
 * @param[in] material Material parameters
 * @return Material ID, 0 on failure
 */
uint32_t render_material_create(RenderMaterialManager* manager, const Material* material);

/**
 * @brief Get material by ID
 * @param[in] manager Material manager
 * @param[in] material_id Material ID
 * @return Pointer to material, NULL if not found
 */
const Material* render_material_get(RenderMaterialManager* manager, uint32_t material_id);

/**
 * @brief Create material instance
 * @param[in] manager Material manager
 * @param[in] material_id ID of base material
 * @return Instance ID, 0 on failure
 */
uint32_t render_material_instance_create(RenderMaterialManager* manager, uint32_t material_id);

/**
 * @brief Set material parameter
 * @param[in] manager Material manager
 * @param[in] instance_id Instance ID
 * @param[in] param_name Parameter name
 * @param[in] param_type Parameter type
 * @param[in] value Parameter value
 * @return true on success
 */
bool render_material_instance_set_parameter(RenderMaterialManager* manager,
                                            uint32_t instance_id,
                                            const char* param_name,
                                            MaterialParameterType param_type,
                                            const MaterialParameterValue* value);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_MATERIAL_H */
