// materials/pbr_material.m
// PBR Material System Implementation
#include "include/materials/pbr_material.h"
#include "include/core/logger.h"
#import <Metal/Metal.h>
#include <stdlib.h>
#include <string.h>

PBRMaterial *pbr_material_create(id<MTLDevice> device, const char *name) {
  PBRMaterial *mat = (PBRMaterial *)calloc(1, sizeof(PBRMaterial));

  strncpy(mat->name, name, sizeof(mat->name) - 1);

  // Default PBR values
  mat->base_color = vec3(0.8f, 0.8f, 0.8f);
  mat->metallic = 0.0f;
  mat->roughness = 0.5f;
  mat->specular = 0.5f;
  mat->opacity = 1.0f;
  mat->opacity_mask_clip = 0.33f;

  mat->shading_model = SHADING_MODEL_DEFAULT_LIT;
  mat->blend_mode = BLEND_MODE_OPAQUE;
  mat->two_sided = false;
  mat->cast_shadow = true;
  mat->receive_shadow = true;

  // Create uniform buffer
  mat->uniform_buffer =
      [device newBufferWithLength:4096 options:MTLResourceStorageModeShared];

  LOG_INFO("Created PBR material: %s", name);
  return mat;
}

void pbr_material_destroy(PBRMaterial *material) {
  if (!material)
    return;

  material->uniform_buffer = nil;
  material->pipeline_state = nil;
  material->depth_stencil_state = nil;

  for (u32 i = 0; i < PBR_TEXTURE_COUNT; i++) {
    material->textures[i] = nil;
  }

  free(material);
}

void pbr_material_set_texture(PBRMaterial *material, PBRTextureType type,
                              id<MTLTexture> texture) {
  if (!material || type >= PBR_TEXTURE_COUNT)
    return;
  material->textures[type] = texture;
  material->texture_enabled[type] = (texture != nil);
}

void pbr_material_set_base_color(PBRMaterial *material, Vec3 color) {
  if (!material)
    return;
  material->base_color = color;
}

void pbr_material_set_metallic(PBRMaterial *material, f32 metallic) {
  if (!material)
    return;
  material->metallic = fmaxf(0.0f, fminf(1.0f, metallic));
}

void pbr_material_set_roughness(PBRMaterial *material, f32 roughness) {
  if (!material)
    return;
  material->roughness = fmaxf(0.01f, fminf(1.0f, roughness));
}

void pbr_material_set_emissive(PBRMaterial *material, Vec3 color,
                               f32 strength) {
  if (!material)
    return;
  material->emissive_color = color;
  material->emissive_strength = strength;
}

void pbr_material_set_subsurface(PBRMaterial *material, Vec3 color,
                                 f32 radius) {
  if (!material)
    return;
  material->subsurface_color = color;
  material->subsurface_radius = radius;
  material->shading_model = SHADING_MODEL_SUBSURFACE;
}

void pbr_material_set_clear_coat(PBRMaterial *material, f32 strength,
                                 f32 roughness) {
  if (!material)
    return;
  material->clear_coat = fmaxf(0.0f, fminf(1.0f, strength));
  material->clear_coat_roughness = fmaxf(0.0f, fminf(1.0f, roughness));
  material->shading_model = SHADING_MODEL_CLEAR_COAT;
}

void pbr_material_add_parameter(PBRMaterial *material, const char *name,
                                Vec4 value) {
  if (!material || material->parameter_count >= MAX_PARAMETERS)
    return;

  MaterialParameter *param = &material->parameters[material->parameter_count++];
  strncpy(param->name, name, sizeof(param->name) - 1);
  param->value = value;
  param->is_texture = false;
}

Vec4 pbr_material_get_parameter(const PBRMaterial *material, const char *name) {
  if (!material)
    return vec4(0, 0, 0, 0);

  for (u32 i = 0; i < material->parameter_count; i++) {
    if (strcmp(material->parameters[i].name, name) == 0) {
      return material->parameters[i].value;
    }
  }

  return vec4(0, 0, 0, 0);
}

bool pbr_material_compile(PBRMaterial *material, id<MTLDevice> device,
                          id<MTLLibrary> shader_lib) {
  if (!material || !device || !shader_lib)
    return false;

  @autoreleasepool {
    NSError *error = nil;

    MTLRenderPipelineDescriptor *desc =
        [[MTLRenderPipelineDescriptor alloc] init];

    // Select vertex shader
    desc.vertexFunction = [shader_lib newFunctionWithName:@"pbr_vertex"];

    // Select fragment shader based on shading model
    NSString *fragmentName;
    switch (material->shading_model) {
    case SHADING_MODEL_SUBSURFACE:
      fragmentName = @"pbr_fragment_subsurface";
      break;
    case SHADING_MODEL_CLEAR_COAT:
      fragmentName = @"pbr_fragment_clearcoat";
      break;
    case SHADING_MODEL_UNLIT:
      fragmentName = @"pbr_fragment_unlit";
      break;
    default:
      fragmentName = @"pbr_fragment_default";
      break;
    }

    desc.fragmentFunction = [shader_lib newFunctionWithName:fragmentName];

    // Configure pixel format
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    // Configure blending
    MTLRenderPipelineColorAttachmentDescriptor *colorAttach =
        desc.colorAttachments[0];

    switch (material->blend_mode) {
    case BLEND_MODE_TRANSLUCENT:
      colorAttach.blendingEnabled = YES;
      colorAttach.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
      colorAttach.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
      colorAttach.rgbBlendOperation = MTLBlendOperationAdd;
      break;
    case BLEND_MODE_ADDITIVE:
      colorAttach.blendingEnabled = YES;
      colorAttach.sourceRGBBlendFactor = MTLBlendFactorOne;
      colorAttach.destinationRGBBlendFactor = MTLBlendFactorOne;
      colorAttach.rgbBlendOperation = MTLBlendOperationAdd;
      break;
    case BLEND_MODE_MASKED:
    case BLEND_MODE_OPAQUE:
    default:
      colorAttach.blendingEnabled = NO;
      break;
    }

    material->pipeline_state =
        [device newRenderPipelineStateWithDescriptor:desc error:&error];

    if (!material->pipeline_state) {
      LOG_ERROR("Failed to create material pipeline: %s",
                [[error localizedDescription] UTF8String]);
      return false;
    }

    // Create depth stencil state
    MTLDepthStencilDescriptor *depthDesc =
        [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthDesc.depthWriteEnabled = (material->blend_mode == BLEND_MODE_OPAQUE ||
                                   material->blend_mode == BLEND_MODE_MASKED);

    material->depth_stencil_state =
        [device newDepthStencilStateWithDescriptor:depthDesc];

    LOG_INFO("Compiled material: %s", material->name);
    return true;
  }
}

void pbr_material_bind(PBRMaterial *material,
                       id<MTLRenderCommandEncoder> encoder) {
  if (!material || !encoder)
    return;

  @autoreleasepool {
    [encoder setRenderPipelineState:material->pipeline_state];
    [encoder setDepthStencilState:material->depth_stencil_state];
    [encoder
        setCullMode:material->two_sided ? MTLCullModeNone : MTLCullModeBack];

    // Update uniform buffer
    typedef struct {
      float base_color[3];
      float metallic;
      float roughness;
      float specular;
      float emissive[3];
      float emissive_strength;
      float opacity;
      float subsurface_color[3];
      float subsurface_radius;
      float clear_coat;
      float clear_coat_roughness;
    } MaterialUniforms;

    MaterialUniforms *uniforms =
        (MaterialUniforms *)[material->uniform_buffer contents];
    uniforms->base_color[0] = material->base_color.x;
    uniforms->base_color[1] = material->base_color.y;
    uniforms->base_color[2] = material->base_color.z;
    uniforms->metallic = material->metallic;
    uniforms->roughness = material->roughness;
    uniforms->specular = material->specular;
    uniforms->emissive[0] = material->emissive_color.x;
    uniforms->emissive[1] = material->emissive_color.y;
    uniforms->emissive[2] = material->emissive_color.z;
    uniforms->emissive_strength = material->emissive_strength;
    uniforms->opacity = material->opacity;
    uniforms->subsurface_color[0] = material->subsurface_color.x;
    uniforms->subsurface_color[1] = material->subsurface_color.y;
    uniforms->subsurface_color[2] = material->subsurface_color.z;
    uniforms->subsurface_radius = material->subsurface_radius;
    uniforms->clear_coat = material->clear_coat;
    uniforms->clear_coat_roughness = material->clear_coat_roughness;

    [encoder setFragmentBuffer:material->uniform_buffer offset:0 atIndex:1];

    // Bind textures
    for (u32 i = 0; i < PBR_TEXTURE_COUNT; i++) {
      if (material->texture_enabled[i] && material->textures[i]) {
        [encoder setFragmentTexture:material->textures[i] atIndex:i];
      }
    }
  }
}

MaterialInstance *material_instance_create(PBRMaterial *parent) {
  MaterialInstance *instance =
      (MaterialInstance *)calloc(1, sizeof(MaterialInstance));
  instance->parent = parent;
  return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
  if (!instance)
    return;
  free(instance);
}

void material_instance_set_parameter(MaterialInstance *instance,
                                     const char *name, Vec4 value) {
  if (!instance || instance->override_count >= MAX_PARAMETERS)
    return;

  // Check if parameter already exists in overrides
  for (u32 i = 0; i < instance->override_count; i++) {
    if (strcmp(instance->overrides[i].name, name) == 0) {
      instance->overrides[i].value = value;
      return;
    }
  }

  // Add new override
  MaterialParameter *param = &instance->overrides[instance->override_count++];
  strncpy(param->name, name, sizeof(param->name) - 1);
  param->value = value;
}

void material_instance_bind(MaterialInstance *instance,
                            id<MTLRenderCommandEncoder> encoder) {
  if (!instance || !instance->parent)
    return;

  // Bind parent first
  pbr_material_bind(instance->parent, encoder);

  // Then apply overrides
  // TODO: Update uniform buffer with overridden values
}
