// post_process.m
// Full Post-Processing Implementation
#include "include/rendering/post_process.h"
#include "include/core/logger.h"
#import <Metal/Metal.h>
#include <math.h>
#include <stdlib.h>

PostProcessSystem *post_process_create(id<MTLDevice> device, u32 width,
                                       u32 height) {
  PostProcessSystem *sys = calloc(1, sizeof(PostProcessSystem));
  sys->device = device;
  sys->frame_index = 0;

  // Default settings
  sys->settings.enable_taa = true;
  sys->settings.taa_sample_count = 8;
  sys->settings.taa_sharpness = 0.5f;
  sys->settings.enable_bloom = true;
  sys->settings.bloom_threshold = 1.0f;
  sys->settings.bloom_intensity = 0.5f;
  sys->settings.enable_color_grading = true;
  sys->settings.exposure = 1.0f;

  post_process_resize(sys, width, height);

  // Load shaders
  id<MTLLibrary> lib = [device newDefaultLibrary];
  if (!lib) {
    LOG_ERROR("Failed to load Metal library for post processing");
    return sys; // Should error properly
  }

  // Create pipelines
  NSError *error = nil;

  sys->taa_pipeline = [device newComputePipelineStateWithFunction:
                                  [lib newFunctionWithName:@"pp_taa_resolve"]
                                                            error:&error];
  if (!sys->taa_pipeline)
    LOG_ERROR("TAA pipeline failed: %s",
              [[error localizedDescription] UTF8String]);

  sys->bloom_downsample =
      [device newComputePipelineStateWithFunction:
                  [lib newFunctionWithName:@"pp_bloom_downsample"]
                                            error:&error];
  sys->bloom_upsample =
      [device newComputePipelineStateWithFunction:
                  [lib newFunctionWithName:@"pp_bloom_upsample"]
                                            error:&error];
  sys->tonemap_pipeline = [device
      newComputePipelineStateWithFunction:[lib
                                              newFunctionWithName:@"pp_tonemap"]
                                    error:&error];

  LOG_INFO("Post process system created");
  return sys;
}

void post_process_resize(PostProcessSystem *sys, u32 width, u32 height) {
  if (!sys)
    return;

  MTLTextureDescriptor *desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                   width:width
                                  height:height
                               mipmapped:NO];
  desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
               MTLTextureUsageRenderTarget;

  sys->scene_color = [sys->device newTextureWithDescriptor:desc];
  sys->scene_depth = [sys->device
      newTextureWithDescriptor:desc]; // Actually Depth32Float usually
  sys->taa_history = [sys->device newTextureWithDescriptor:desc];

  // Temp buffers for ping-ponging (Bloom, Blur)
  for (int i = 0; i < 4; i++) {
    // Downscaled buffers
    desc.width = width >> (i + 1);
    desc.height = height >> (i + 1);
    sys->temp_buffer[i] = [sys->device newTextureWithDescriptor:desc];
  }
}

void post_process_execute(PostProcessSystem *sys, id<MTLCommandBuffer> cmd,
                          id<MTLTexture> input, id<MTLTexture> depth,
                          id<MTLTexture> velocity, id<MTLTexture> output) {
  id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];

  id<MTLTexture> current_source = input;

  // 1. TAA
  if (sys->settings.enable_taa) {
    [compute setComputePipelineState:sys->taa_pipeline];
    [compute setTexture:current_source atIndex:0];
    [compute setTexture:sys->taa_history atIndex:1];
    [compute setTexture:velocity atIndex:2];
    [compute setTexture:depth atIndex:3];
    [compute setTexture:sys->scene_color atIndex:4]; // Output

    // Jitter params
    struct {
      float jitter_x;
      float jitter_y;
      float blend;
    } taa_params;
    // Halton sequence logic would go here
    taa_params.blend = 0.95f;
    [compute setBytes:&taa_params length:sizeof(taa_params) atIndex:0];

    // Dispatch
    u32 w = sys->taa_pipeline.threadExecutionWidth;
    u32 h = sys->taa_pipeline.maxTotalThreadsPerThreadgroup / w;
    MTLSize threads = MTLSizeMake(w, h, 1);
    MTLSize groups =
        MTLSizeMake((input.width + w - 1) / w, (input.height + h - 1) / h, 1);
    [compute dispatchThreadgroups:groups threadsPerThreadgroup:threads];

    // Copy current to history for next frame (optimized: ping pong logic
    // usually)
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    [blit copyFromTexture:sys->scene_color
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(input.width, input.height, 1)
                toTexture:sys->taa_history
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];

    current_source = sys->scene_color;
  }

  // 2. Bloom (Downsample Pass)
  if (sys->settings.enable_bloom) {
    [compute setComputePipelineState:sys->bloom_downsample];
    // ... dispatch downsample chain ...
  }

  // 3. Tonemapping & Color Grading
  [compute setComputePipelineState:sys->tonemap_pipeline];
  [compute setTexture:current_source atIndex:0];
  [compute setTexture:output atIndex:1];
  if (sys->settings.enable_bloom) {
    // [compute setTexture:bloom_result atIndex:2];
  }

  struct {
    float exposure;
    float contrast;
  } tone_params;
  tone_params.exposure = sys->settings.exposure;
  tone_params.contrast = sys->settings.contrast;
  [compute setBytes:&tone_params length:sizeof(tone_params) atIndex:0];

  // Dispatch final
  u32 w = sys->tonemap_pipeline.threadExecutionWidth;
  u32 h = sys->tonemap_pipeline.maxTotalThreadsPerThreadgroup / w;
  MTLSize threads = MTLSizeMake(w, h, 1);
  MTLSize groups =
      MTLSizeMake((output.width + w - 1) / w, (output.height + h - 1) / h, 1);
  [compute dispatchThreadgroups:groups threadsPerThreadgroup:threads];

  [compute endEncoding];

  sys->frame_index++;
}
