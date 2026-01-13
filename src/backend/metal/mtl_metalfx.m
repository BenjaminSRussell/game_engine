// Complete MetalFX temporal upscaling implementation
#include "../../include/core/logger.h"
#include "../../include/rendering/metal_fx.h"
#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>
#include <stdlib.h>

struct MetalFXUpscaler {
  id<MTLDevice> device;
  id<MTLFXTemporalScaler> temporal_scaler;
  MetalFXConfig config;
  u32 frame_index;
  f32 jitter_sequence[8][2]; // Halton sequence for 8 frames
};

// Halton sequence generator for temporal jitter
static void generate_halton_sequence(f32 sequence[][2], u32 count) {
  for (u32 i = 0; i < count; i++) {
    // Halton base 2 (X)
    f32 x = 0.0f;
    f32 f = 0.5f;
    u32 index = i + 1;
    while (index > 0) {
      if (index & 1)
        x += f;
      index >>= 1;
      f *= 0.5f;
    }

    // Halton base 3 (Y)
    f32 y = 0.0f;
    f = 1.0f / 3.0f;
    index = i + 1;
    while (index > 0) {
      y += f * (index % 3);
      index /= 3;
      f /= 3.0f;
    }

    sequence[i][0] = x - 0.5f; // Center around 0
    sequence[i][1] = y - 0.5f;
  }
}

MetalFXUpscaler *metalfx_create_temporal_upscaler(id<MTLDevice> device,
                                                  const MetalFXConfig *config) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000 ||                                \
    __IPHONE_OS_VERSION_MAX_ALLOWED >= 160000
  if (@available(macOS 13.0, iOS 16.0, *)) {
    Class scalerClass = NSClassFromString(@"MTLFXTemporalScaler");
    if (!scalerClass ||
        ![scalerClass respondsToSelector:@selector(supportsDevice:)]) {
      LOG_WARN("MetalFX not available on this system");
      return NULL;
    }

    if (![scalerClass supportsDevice:device]) {
      LOG_WARN("MetalFX not supported on this device");
      return NULL;
    }

    MetalFXUpscaler *upscaler =
        (MetalFXUpscaler *)calloc(1, sizeof(MetalFXUpscaler));
    upscaler->device = device;
    upscaler->config = *config;
    upscaler->frame_index = 0;

    // Generate jitter sequence
    generate_halton_sequence(upscaler->jitter_sequence, 8);

    // Create MetalFX temporal scaler
    MTLFXTemporalScalerDescriptor *desc =
        [[MTLFXTemporalScalerDescriptor alloc] init];
    desc.inputWidth = config->input_width;
    desc.inputHeight = config->input_height;
    desc.outputWidth = config->output_width;
    desc.outputHeight = config->output_height;
    desc.colorTextureFormat = MTLPixelFormatRGBA16Float;
    desc.depthTextureFormat = MTLPixelFormatDepth32Float;
    desc.motionTextureFormat = MTLPixelFormatRG16Float;
    desc.outputTextureFormat = MTLPixelFormatRGBA16Float;
    desc.autoExposureEnabled = config->enable_auto_exposure;

    upscaler->temporal_scaler = [desc newTemporalScalerWithDevice:device];

    if (upscaler->temporal_scaler) {
      LOG_INFO("MetalFX temporal upscaler created: %ux%u -> %ux%u",
               config->input_width, config->input_height, config->output_width,
               config->output_height);
      return upscaler;
    } else {
      LOG_ERROR("Failed to create MetalFX temporal scaler");
      free(upscaler);
    }
  }
#endif

  LOG_ERROR("MetalFX requires macOS 13.0+ or iOS 16.0+");
  return NULL;
}

void metalfx_upscale(MetalFXUpscaler *upscaler, id<MTLCommandBuffer> cmd,
                     id<MTLTexture> input_color, id<MTLTexture> input_depth,
                     id<MTLTexture> input_motion, id<MTLTexture> output) {
  if (!upscaler || !upscaler->temporal_scaler)
    return;

  @autoreleasepool {
    upscaler->temporal_scaler.colorTexture = input_color;
    upscaler->temporal_scaler.depthTexture = input_depth;
    upscaler->temporal_scaler.motionTexture = input_motion;
    upscaler->temporal_scaler.outputTexture = output;

    // Set jitter offset for current frame
    f32 jitter_x = upscaler->jitter_sequence[upscaler->frame_index % 8][0];
    f32 jitter_y = upscaler->jitter_sequence[upscaler->frame_index % 8][1];

    upscaler->temporal_scaler.jitterOffsetX = jitter_x;
    upscaler->temporal_scaler.jitterOffsetY = jitter_y;

    // Encode upscaling operation
    [upscaler->temporal_scaler encodeToCommandBuffer:cmd];

    upscaler->frame_index++;
  }
}

void metalfx_get_jitter_offset(u32 frame_index, f32 *out_x, f32 *out_y) {
  f32 sequence[8][2];
  generate_halton_sequence(sequence, 8);

  u32 idx = frame_index % 8;
  *out_x = sequence[idx][0];
  *out_y = sequence[idx][1];
}

void metalfx_get_render_resolution(u32 output_width, u32 output_height,
                                   MetalFXQuality quality,
                                   u32 *out_render_width,
                                   u32 *out_render_height) {
  f32 scale = 1.0f;

  switch (quality) {
  case METALFX_QUALITY_PERFORMANCE:
    scale = 0.5f; // 50% resolution
    break;
  case METALFX_QUALITY_BALANCED:
    scale = 0.67f; // 67% resolution
    break;
  case METALFX_QUALITY_QUALITY:
    scale = 0.77f; // 77% resolution
    break;
  case METALFX_QUALITY_ULTRA_QUALITY:
    scale = 0.9f; // 90% resolution
    break;
  }

  *out_render_width = (u32)(output_width * scale);
  *out_render_height = (u32)(output_height * scale);

  // Ensure even dimensions
  *out_render_width = (*out_render_width + 1) & ~1;
  *out_render_height = (*out_render_height + 1) & ~1;
}

bool metalfx_is_supported(id<MTLDevice> device) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000 ||                                \
    __IPHONE_OS_VERSION_MAX_ALLOWED >= 160000
  if (@available(macOS 13.0, iOS 16.0, *)) {
    Class scalerClass = NSClassFromString(@"MTLFXTemporalScaler");
    if (scalerClass &&
        [scalerClass respondsToSelector:@selector(supportsDevice:)]) {
      return [scalerClass supportsDevice:device];
    }
  }
#endif
  return false;
}

void metalfx_destroy_upscaler(MetalFXUpscaler *upscaler) {
  if (!upscaler)
    return;

  upscaler->temporal_scaler = nil;
  free(upscaler);
}
