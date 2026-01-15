#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct RenderContext RenderContext;
typedef struct ShaderProgram ShaderProgram;
typedef struct Material Material;
typedef struct MaterialInstance MaterialInstance;

// Material parameter types
typedef enum {
  MATERIAL_PARAM_FLOAT,
  MATERIAL_PARAM_VEC2,
  MATERIAL_PARAM_VEC3,
  MATERIAL_PARAM_VEC4,
  MATERIAL_PARAM_INT,
  MATERIAL_PARAM_BOOL,
  MATERIAL_PARAM_TEXTURE_2D,
  MATERIAL_PARAM_TEXTURE_CUBE
} MaterialParamType;

// Material parameter
typedef struct {
  char name[64];
  MaterialParamType type;
  union {
    float f;
    float vec2[2];
    float vec3[3];
    float vec4[4];
    int32_t i;
    bool b;
    void *texture; // Texture handle
  } value;
} MaterialParam;

// Material blend mode
typedef enum {
  MATERIAL_BLEND_OPAQUE,
  MATERIAL_BLEND_ALPHA,
  MATERIAL_BLEND_ADDITIVE,
  MATERIAL_BLEND_MULTIPLY
} MaterialBlendMode;

// Material cull mode
typedef enum {
  MATERIAL_CULL_NONE,
  MATERIAL_CULL_FRONT,
  MATERIAL_CULL_BACK
} MaterialCullMode;

// Material descriptor
typedef struct {
  const char *name;
  ShaderProgram *shader_program;

  // Render state
  MaterialBlendMode blend_mode;
  MaterialCullMode cull_mode;
  bool depth_test;
  bool depth_write;
  bool wireframe;

  // Default parameters
  MaterialParam *params;
  uint32_t param_count;
} MaterialDesc;

// PBR material parameters (convenience)
typedef struct {
  float base_color[4]; // RGBA
  float metallic;
  float roughness;
  float ao;          // Ambient occlusion
  float emissive[3]; // RGB

  void *albedo_texture;
  void *normal_texture;
  void *metallic_roughness_texture;
  void *ao_texture;
  void *emissive_texture;
} PBRMaterialParams;

// =============================================================================
// MATERIAL SYSTEM
// =============================================================================

// Initialize material system
bool material_system_init(RenderContext *renderer);
void material_system_shutdown(void);

// Create/destroy materials
Material *material_create(const MaterialDesc *desc);
void material_destroy(Material *material);

// Create/destroy material instances
MaterialInstance *material_instance_create(Material *base_material);
void material_instance_destroy(MaterialInstance *instance);

// Parameter management
bool material_set_param_float(Material *material, const char *name,
                              float value);
bool material_set_param_vec3(Material *material, const char *name,
                             const float *value);
bool material_set_param_vec4(Material *material, const char *name,
                             const float *value);
bool material_set_param_texture(Material *material, const char *name,
                                void *texture);

bool material_instance_set_param_float(MaterialInstance *instance,
                                       const char *name, float value);
bool material_instance_set_param_vec3(MaterialInstance *instance,
                                      const char *name, const float *value);
bool material_instance_set_param_vec4(MaterialInstance *instance,
                                      const char *name, const float *value);
bool material_instance_set_param_texture(MaterialInstance *instance,
                                         const char *name, void *texture);

// Get parameters
const MaterialParam *material_get_param(Material *material, const char *name);
const MaterialParam *material_instance_get_param(MaterialInstance *instance,
                                                 const char *name);

// Convenience: Create PBR material
Material *material_create_pbr(const char *name, ShaderProgram *shader,
                              const PBRMaterialParams *params);

// Material binding (for rendering)
void material_bind(Material *material);
void material_instance_bind(MaterialInstance *instance);

// Utilities
uint32_t material_get_param_size(MaterialParamType type);
const char *material_blend_mode_to_string(MaterialBlendMode mode);
const char *material_cull_mode_to_string(MaterialCullMode mode);
