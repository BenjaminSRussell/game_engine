/*
 * metallic_roughness.m
 * Metallic-roughness workflow implementation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/pbr/metallic_roughness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal definition of metal_device_t to access the device object
// This should match the definition in the backend implementation
typedef struct metal_device {
  id<MTLDevice> device;
  id<MTLCommandQueue> queue;
} metal_device_t;

material_t *material_create(metal_device_t *dev, const char *name) {
  if (!dev || !dev->device)
    return NULL;

  material_t *mat = calloc(1, sizeof(material_t));
  if (!mat)
    return NULL;

  if (name) {
    strncpy(mat->name, name, 63);
  } else {
    strcpy(mat->name, "Material");
  }

  // Default PBR values
  mat->params.albedo_factor = simd_make_float4(1, 1, 1, 1);
  mat->params.metallic_factor = 0.0f;
  mat->params.roughness_factor = 0.5f;
  mat->params.normal_scale = 1.0f;
  mat->params.occlusion_strength = 1.0f;
  mat->params.emissive_factor = simd_make_float3(0, 0, 0);
  mat->params.alpha_cutoff = 0.5f;

  // Create GPU buffer
  mat->param_buffer =
      [dev->device newBufferWithLength:sizeof(pbr_material_params_t)
                               options:MTLResourceStorageModeShared];
  if (name) {
    [mat->param_buffer setLabel:[NSString stringWithUTF8String:name]];
  }

  return mat;
}

void material_destroy(material_t *mat) {
  if (!mat)
    return;

  // ARC will handle the release of Objective-C objects (id<MTLBuffer>,
  // id<MTLTexture>) when the struct is freed, IF this was compiled as Obj-C++
  // or strict ARC. However, in C + ObjC mix, we might need explicit releases if
  // not using ARC for C structs. Assuming ARC is enabled for .m files, but
  // since this is allocated with calloc, ARC DOES NOT track fields inside a C
  // struct! We must manually release them. BUT, we can't call [obj release] in
  // ARC. Standard practice for C-structs holding ObjC objects in ARC is to use
  // __unsafe_unretained or CFBridgingRelease. Or simpler: assign nil to them.

  mat->param_buffer = nil;
  mat->albedo_texture = nil;
  mat->normal_texture = nil;
  mat->metallic_roughness_texture = nil;
  mat->occlusion_texture = nil;
  mat->emissive_texture = nil;

  free(mat);
}

void material_update_gpu(material_t *mat) {
  if (!mat || !mat->param_buffer)
    return;
  memcpy([mat->param_buffer contents], &mat -> params,
         sizeof(pbr_material_params_t));
}

void material_bind(material_t *mat, id<MTLRenderCommandEncoder> encoder,
                   material_defaults_t *defaults) {
  if (!mat || !encoder || !defaults)
    return;

  // Bind textures (use defaults if NULL)
  [encoder setFragmentTexture:(mat->albedo_texture ?: defaults->white_1x1)
                      atIndex:0];
  [encoder setFragmentTexture:(mat->normal_texture ?: defaults->normal_1x1)
                      atIndex:1];
  [encoder setFragmentTexture:(mat->metallic_roughness_texture
                                   ?: defaults->white_1x1)
                      atIndex:2];
  [encoder setFragmentTexture:(mat->occlusion_texture ?: defaults->white_1x1)
                      atIndex:3];
  [encoder setFragmentTexture:(mat->emissive_texture ?: defaults->black_1x1)
                      atIndex:4];

  // Bind parameters
  [encoder setFragmentBuffer:mat->param_buffer offset:0 atIndex:0];

  // Bind sampler
  [encoder setFragmentSamplerState:defaults->default_sampler atIndex:0];
}

void material_defaults_init(material_defaults_t *defaults,
                            metal_device_t *dev) {
  if (!defaults || !dev)
    return;

  // Helper to create 1x1 texture
  id<MTLTexture> (^createTexture)(uint8_t r, uint8_t g, uint8_t b, uint8_t a) =
      ^id<MTLTexture>(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    MTLTextureDescriptor *desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:1
                                    height:1
                                 mipmapped:NO];
    id<MTLTexture> tex = [dev->device newTextureWithDescriptor:desc];
    uint8_t pixel[] = {r, g, b, a};
    [tex replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
           mipmapLevel:0
             withBytes:pixel
           bytesPerRow:4];
    return tex;
  };

  defaults->white_1x1 = createTexture(255, 255, 255, 255);
  defaults->black_1x1 = createTexture(0, 0, 0, 255);
  defaults->normal_1x1 = createTexture(128, 128, 255, 255); // Flat normal

  MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
  samplerDesc.minFilter = MTLSamplerMinFilterLinear;
  samplerDesc.magFilter = MTLSamplerMagFilterLinear;
  samplerDesc.mipFilter = MTLSamplerMipFilterLinear;
  samplerDesc.sAddressMode = MTLSamplerAddressModeRepeat;
  samplerDesc.tAddressMode = MTLSamplerAddressModeRepeat;
  defaults->default_sampler =
      [dev->device newSamplerStateWithDescriptor:samplerDesc];
}

void material_defaults_cleanup(material_defaults_t *defaults) {
  if (!defaults)
    return;
  defaults->white_1x1 = nil;
  defaults->black_1x1 = nil;
  defaults->normal_1x1 = nil;
  defaults->default_sampler = nil;
}
