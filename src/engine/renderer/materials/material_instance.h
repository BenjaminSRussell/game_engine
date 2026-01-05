// Material Instance System
// Master materials define shader + defaults, instances override specific parameters

#ifndef MATERIAL_INSTANCE_H
#define MATERIAL_INSTANCE_H

#include "shader_permutations.h"
#include "core/types.h"
#include "renderer/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// Material parameter types
typedef enum MaterialParameterType {
    MATERIAL_PARAM_FLOAT,
    MATERIAL_PARAM_VEC2,
    MATERIAL_PARAM_VEC3,
    MATERIAL_PARAM_VEC4,
    MATERIAL_PARAM_COLOR,      // Vec4 with color picker UI hint
    MATERIAL_PARAM_TEXTURE,
    MATERIAL_PARAM_INT,
    MATERIAL_PARAM_BOOL
} MaterialParameterType;

// Material parameter value
typedef struct MaterialParameter {
    char name[64];
    MaterialParameterType type;
    
    union {
        f32 float_val;
        f32 vec2_val[2];
        f32 vec3_val[3];
        f32 vec4_val[4];
        TextureID texture_val;
        i32 int_val;
        bool bool_val;
    } value;
    
    bool is_overridden;  // For instances
} MaterialParameter;

// Master material (base template)
typedef struct MaterialMaster {
    char name[128];
    char shader_base_path[256];  // e.g., "assets/shaders/pbr_standard"
    
    // Shader variant cache
    ShaderVariantCache *variant_cache;
    
    // Supported features (what variants can this master create?)
    u32 supported_features;
    ShaderQualityLevel default_quality;
    
    // Default parameters
    MaterialParameter *default_params;
    u32 param_count;
    u32 param_capacity;
    
    // Render state
    bool two_sided;
    bool alpha_blend;
    bool alpha_test;
    
} MaterialMaster;

// Material instance (derived from master)
typedef struct MaterialInstance {
    char name[128];
    MaterialMaster *parent;
    
    // Parameter overrides
    MaterialParameter *overrides;
    u32 override_count;
    u32 override_capacity;
    
    // Active shader variant (cached)
    ShaderVariant *active_variant;
    ShaderPermutation active_permutation;
    bool needs_recompile;
    
} MaterialInstance;

// === Master Material API ===

// Create master material from shader
MaterialMaster *material_master_create(const char *name, const char *shader_path);

// Destroy master
void material_master_destroy(MaterialMaster *master);

// Add default parameter to master
void material_master_add_param(MaterialMaster *master, const char *name, 
                                MaterialParameterType type, const void *default_value);

// Add texture slot to master
void material_master_add_texture(MaterialMaster *master, const char *name, TextureID default_tex);

// Set supported features
void material_master_set_features(MaterialMaster *master, u32 feature_mask);

// === Instance API ===

// Create instance from master
MaterialInstance *material_instance_create(MaterialMaster *parent, const char *name);

// Destroy instance
void material_instance_destroy(MaterialInstance *instance);

// Set parameter override
void material_instance_set_float(MaterialInstance *instance, const char *name, f32 value);
void material_instance_set_vec3(MaterialInstance *instance, const char *name, const f32 *value);
void material_instance_set_vec4(MaterialInstance *instance, const char *name, const f32 *value);
void material_instance_set_texture(MaterialInstance *instance, const char *name, TextureID texture);
void material_instance_set_bool(MaterialInstance *instance, const char *name, bool value);

// Get parameter (checks overrides first, then parent defaults)
MaterialParameter *material_instance_get_param(MaterialInstance *instance, const char *name);

// Compile shader variant for instance based on active features
bool material_instance_compile(MaterialInstance *instance);

// Bind material for rendering
void material_instance_bind(MaterialInstance *instance);

// Upload parameters to GPU
void material_instance_upload_params(MaterialInstance *instance);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_INSTANCE_H
