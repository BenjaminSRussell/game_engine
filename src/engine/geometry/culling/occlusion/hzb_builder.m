/*
 * hzb_builder.m
 * Hierarchical Z-Buffer Construction Implementation
 */

#import "hzb_builder.h"
#import <Metal/Metal.h>
#include <include/math/math.h>
#import <simd/simd.h>
#include <stdio.h>

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

static uint32_t calculate_mip_levels(uint32_t width, uint32_t height) {
  uint32_t max_dim = width > height ? width : height;
  return (uint32_t)floor(log2(max_dim)) + 1;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int hzb_builder_init(hzb_builder_t *hzb, id<MTLDevice> device, uint32_t width,
                     uint32_t height) {
  if (!hzb || !device)
    return -1;

  hzb->width = width;
  hzb->height = height;
  hzb->mip_levels = calculate_mip_levels(width, height);

  // Create HZB texture with mipmaps
  MTLTextureDescriptor *desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float
                                   width:width
                                  height:height
                               mipmapped:YES];
  desc.mipmapLevelCount = hzb->mip_levels;
  desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
  desc.storageMode = MTLStorageModePrivate;

  hzb->hzb_texture = [device newTextureWithDescriptor:desc];
  if (!hzb->hzb_texture) {
    printf("HZB: Failed to create HZB texture\n");
    return -2;
  }

  // Load shader and create pipeline
  NSError *error = nil;
  id<MTLLibrary> library = [device newDefaultLibrary];
  if (!library) {
    printf("HZB: Failed to load default library\n");
    return -3;
  }

  id<MTLFunction> reduceFunc = [library newFunctionWithName:@"hzb_reduce"];
  if (!reduceFunc) {
    printf("HZB: Failed to find hzb_reduce function\n");
    return -4;
  }

  hzb->reduce_pipeline = [device newComputePipelineStateWithFunction:reduceFunc
                                                               error:&error];
  if (error) {
    printf("HZB: Failed to create pipeline: %s\n",
           [[error localizedDescription] UTF8String]);
    return -5;
  }

  hzb->initialized = true;
  printf("HZB: Initialized %dx%d with %d mip levels\n", width, height,
         hzb->mip_levels);

  return 0;
}

void hzb_builder_shutdown(hzb_builder_t *hzb) {
  if (!hzb)
    return;

  hzb->hzb_texture = nil;
  hzb->reduce_pipeline = nil;
  hzb->initialized = false;
}

void hzb_builder_build(hzb_builder_t *hzb, id<MTLCommandBuffer> cmd,
                       id<MTLTexture> source_depth) {
  if (!hzb || !hzb->initialized || !cmd || !source_depth)
    return;

  id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
  [encoder setLabel:@"HZB Generation"];
  [encoder setComputePipelineState:hzb->reduce_pipeline];

  // Copy source depth to mip 0
  id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
  [blit copyFromTexture:source_depth
            sourceSlice:0
            sourceLevel:0
           sourceOrigin:MTLOriginMake(0, 0, 0)
             sourceSize:MTLSizeMake(hzb->width, hzb->height, 1)
              toTexture:hzb->hzb_texture
       destinationSlice:0
       destinationLevel:0
      destinationOrigin:MTLOriginMake(0, 0, 0)];
  [blit endEncoding];

  // Generate mip chain
  for (uint32_t mip = 1; mip < hzb->mip_levels; mip++) {
    uint32_t src_width = hzb->width >> (mip - 1);
    uint32_t src_height = hzb->height >> (mip - 1);
    uint32_t dst_width = src_width >> 1;
    uint32_t dst_height = src_height >> 1;

    if (dst_width == 0)
      dst_width = 1;
    if (dst_height == 0)
      dst_height = 1;

    // Create texture views for source and dest mip levels
    id<MTLTexture> src_view =
        [hzb->hzb_texture newTextureViewWithPixelFormat:MTLPixelFormatR32Float
                                            textureType:MTLTextureType2D
                                                 levels:NSMakeRange(mip - 1, 1)
                                                 slices:NSMakeRange(0, 1)];

    id<MTLTexture> dst_view =
        [hzb->hzb_texture newTextureViewWithPixelFormat:MTLPixelFormatR32Float
                                            textureType:MTLTextureType2D
                                                 levels:NSMakeRange(mip, 1)
                                                 slices:NSMakeRange(0, 1)];

    [encoder setTexture:src_view atIndex:0];
    [encoder setTexture:dst_view atIndex:1];

    MTLSize threadgroups =
        MTLSizeMake((dst_width + 7) / 8, (dst_height + 7) / 8, 1);
    MTLSize threadsPerGroup = MTLSizeMake(8, 8, 1);

    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];

    // Memory barrier between mip levels
    [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
  }

  [encoder endEncoding];
}

void hzb_builder_resize(hzb_builder_t *hzb, id<MTLDevice> device,
                        uint32_t width, uint32_t height) {
  if (!hzb)
    return;

  if (hzb->width == width && hzb->height == height)
    return;

  hzb_builder_shutdown(hzb);
  hzb_builder_init(hzb, device, width, height);
}

id<MTLTexture> hzb_builder_get_texture(hzb_builder_t *hzb) {
  return hzb ? hzb->hzb_texture : nil;
}
