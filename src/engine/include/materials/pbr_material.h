// materials/pbr_material.h
// Complete PBR (Physically Based Rendering) material system
#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include "include/common.h"
#include "include/math/vec3.h"
#include "include/math/vec4.h"
#import <Metal/Metal.h>

#define MAX_TEXTURE_SLOTS 16
#define MAX_PARAMETERS 64

// PBR texture types
typedef enum {
  PBR_TEXTURE_BASE_COLOR = 0,
  PBR_TEXTURE_NORMAL,
  PBR_TEXTURE_METALLIC,
  PBR_TEXTURE_ROUGHNESS,
  PBR_TEXTURE_AMBIENT_OCCLUSION,
  PBR_TEXTURE_EMISSIVE,
  PBR_TEXTURE_HEIGHT,
  PBR_TEXTURE_OPACITY,
  PBR_TEXTURE_SUBSURFACE_COLOR,
  PBR_TEXTURE_SUBSURFACE_RADIUS,
  PBR_TEXTURE_CLEARCOAT,
  PBR_TEXTURE_CLEARCOAT_ROUGHNESS,
  PBR_TEXTURE_ANISOTROPY,
  PBR_TEXTURE_SHEEN,
  PBR_TEXTURE_COUNT
} PBRTextureType;

// Material blend modes
typedef enum {
  BLEND_MODE_OPAQUE,
  BLEND_MODE_MASKED,
  BLEND_MODE_TRANSLUCENT,
  BLEND_MODE_ADDITIVE,
  BLEND_MODE_MODULATE
} BlendMode;

// Shading models
typedef enum {
  SHADING_MODEL_DEFAULT_LIT,
  SHADING_MODEL_SUBSURFACE,
  SHADING_MODEL_PREINTEGRATED_SKIN,
  SHADING_MODEL_CLEAR_COAT,
  SHADING_MODEL_SUBSURFACE_PROFILE,
  SHADING_MODEL_TWO_SIDED_FOLIAGE,
  SHADING_MODEL_HAIR,
  SHADING_MODEL_CLOTH,
  SHADING_MODEL_EYE,
  SHADING_MODEL_UNLIT
} ShadingModel;

// Material parameter
typedef struct {
  char name[64];
  Vec4 value;
  bool is_texture;
  u32 texture_index;
} MaterialParameter;

// PBR Material
typedef struct {
  char name[128];

  // Core PBR properties
  Vec3 base_color;
  f32 metallic;
  f32 roughness;
  f32 specular;
  f32 anisotropy;

  // Emissive
  Vec3 emissive_color;
  f32 emissive_strength;

  // Opacity
  f32 opacity;
  f32 opacity_mask_clip;

  // Subsurface scattering
  Vec3 subsurface_color;
  f32 subsurface_radius;

  // Clear coat
  f32 clear_coat;
  f32 clear_coat_roughness;

  // Advanced
  f32 sheen;
  f32 sheen_tint;
  Vec3 sheen_color;

  // Rendering
  ShadingModel shading_model;
  BlendMode blend_mode;
  bool two_sided;
  bool cast_shadow;
  bool receive_shadow;

  // Textures
  id<MTLTexture> textures[PBR_TEXTURE_COUNT];
  bool texture_enabled[PBR_TEXTURE_COUNT];

  // Parameters for dynamic materials
  MaterialParameter parameters[MAX_PARAMETERS];
  u32 parameter_count;

  // GPU resources
  id<MTLRenderPipelineState> pipeline_state;
  id<MTLDepthStencilState> depth_stencil_state;
  id<MTLBuffer> uniform_buffer;

} PBRMaterial;

// Material instance (inherits from material with parameter overrides)
typedef struct {
  PBRMaterial *parent;
  MaterialParameter overrides[MAX_PARAMETERS];
  u32 override_count;
} MaterialInstance;

#ifdef __cplusplus
extern "C" {
#endif

// Material creation
PBRMaterial *pbr_material_create(id<MTLDevice> device, const char *name);
void pbr_material_destroy(PBRMaterial *material);

// Texture assignment
void pbr_material_set_texture(PBRMaterial *material, PBRTextureType type,
                              id<MTLTexture> texture);
void pbr_material_enable_texture(PBRMaterial *material, PBRTextureType type,
                                 bool enabled);

// Property setters
void pbr_material_set_base_color(PBRMaterial *material, Vec3 color);
void pbr_material_set_metallic(PBRMaterial *material, f32 metallic);
void pbr_material_set_roughness(PBRMaterial *material, f32 roughness);
void pbr_material_set_emissive(PBRMaterial *material, Vec3 color, f32 strength);
void pbr_material_set_opacity(PBRMaterial *material, f32 opacity);
void pbr_material_set_subsurface(PBRMaterial *material, Vec3 color, f32 radius);
void pbr_material_set_clear_coat(PBRMaterial *material, f32 strength,
                                 f32 roughness);
void pbr_material_set_shading_model(PBRMaterial *material, ShadingModel model);
void pbr_material_set_blend_mode(PBRMaterial *material, BlendMode mode);

// Parameter system
void pbr_material_add_parameter(PBRMaterial *material, const char *name,
                                Vec4 value);
Vec4 pbr_material_get_parameter(const PBRMaterial *material, const char *name);
void pbr_material_set_parameter(PBRMaterial *material, const char *name,
                                Vec4 value);

// Compilation
bool pbr_material_compile(PBRMaterial *material, id<MTLDevice> device,
                          id<MTLLibrary> shader_lib);
void pbr_material_bind(PBRMaterial *material,
                       id<MTLRenderCommandEncoder> encoder);

// Material instances
MaterialInstance *material_instance_create(PBRMaterial *parent);
void material_instance_destroy(MaterialInstance *instance);
void material_instance_set_parameter(MaterialInstance *instance,
                                     const char *name, Vec4 value);
void material_instance_bind(MaterialInstance *instance,
                            id<MTLRenderCommandEncoder> encoder);

#ifdef __cplusplus
}
#endif

#endif // PBR_MATERIAL_H
