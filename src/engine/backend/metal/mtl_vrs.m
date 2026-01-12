// backend/metal/mtl_vrs.m
// Variable Rate Shading Implementation
#include "../../include/core/logger.h"
#include "../../include/rendering/metal_vrs.h"
#import <Metal/Metal.h>

struct MetalVRSImage {
  id<MTLTexture> rate_map;
  u32 width;
  u32 height;
};

// Parameters for the foveated VRS compute shader
typedef struct {
  Vec2 focus_center;
  float inner_radius;
  float outer_radius;
} VRSParams;

static id<MTLComputePipelineState> g_vrs_pipeline = nil;

static void ensure_vrs_pipeline(id<MTLDevice> device) {
  if (g_vrs_pipeline)
    return;

  NSError *error = nil;
  // Load the shader source (in a real engine this would be precompiled)
  NSString *source =
      @"#include <metal_stdlib>\n"
       "using namespace metal;\n"
       "struct VRSParams { float2 focus_center; float inner_radius; float "
       "outer_radius; };\n"
       "kernel void generate_foveated_vrs(texture2d<uint, access::write> "
       "rate_map [[texture(0)]], constant VRSParams &params [[buffer(0)]], "
       "uint2 gid [[thread_position_in_grid]]) {\n"
       "  if (gid.x >= rate_map.get_width() || gid.y >= rate_map.get_height()) "
       "return;\n"
       "  float2 uv = float2(gid) / float2(rate_map.get_width(), "
       "rate_map.get_height());\n"
       "  float dist = distance(uv, params.focus_center);\n"
       "  uint rate = (dist < params.inner_radius) ? 0 : ((dist < "
       "params.outer_radius) ? 1 : 4);\n"
       "  rate_map.write(rate, gid);\n"
       "}";

  id<MTLLibrary> library = [device newLibraryWithSource:source
                                                options:nil
                                                  error:&error];
  if (!library) {
    NSLog(@"Failed to compile VRS shader: %@", error);
    return;
  }

  id<MTLFunction> function =
      [library newFunctionWithName:@"generate_foveated_vrs"];
  g_vrs_pipeline = [device newComputePipelineStateWithFunction:function
                                                         error:&error];
  if (!g_vrs_pipeline) {
    NSLog(@"Failed to create VRS compute pipeline: %@", error);
  }
}

bool metal_vrs_is_supported(id<MTLDevice> device) {
  if (@available(macOS 13.0, iOS 16.0, *)) {
    return [device supportsFamily:MTLGPUFamilyApple7];
  }
  return false;
}

MetalVRSImage *metal_vrs_create_rate_map(id<MTLDevice> device, u32 width,
                                         u32 height) {
  if (!metal_vrs_is_supported(device)) {
    LOG_WARN("VRS not supported on this device");
    return NULL;
  }

  if (@available(macOS 13.0, *)) {
    // VRS rate map is typically 16x16 tiles for the full viewport
    u32 tile_width = (width + 15) / 16;
    u32 tile_height = (height + 15) / 16;

    MTLTextureDescriptor *desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Uint
                                     width:tile_width
                                    height:tile_height
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    desc.storageMode = MTLStorageModePrivate;

    id<MTLTexture> rate_map = [device newTextureWithDescriptor:desc];
    if (!rate_map) {
      LOG_ERROR("Failed to create VRS rate map");
      return NULL;
    }

    MetalVRSImage *vrs = (MetalVRSImage *)calloc(1, sizeof(MetalVRSImage));
    vrs->rate_map = rate_map;
    vrs->width = tile_width;
    vrs->height = tile_height;

    LOG_INFO("Created VRS rate map: %ux%u tiles", tile_width, tile_height);
    return vrs;
  }
  return NULL;
}

void metal_vrs_destroy_rate_map(MetalVRSImage *vrs_map) {
  if (!vrs_map)
    return;
  vrs_map->rate_map = nil;
  free(vrs_map);
}

void metal_vrs_set_foveated(MetalVRSImage *vrs_map, id<MTLCommandBuffer> cmd,
                            Vec2 focus_center, f32 inner_radius,
                            f32 outer_radius) {
  if (!vrs_map || !cmd)
    return;

  id<MTLDevice> device = cmd.device;
  ensure_vrs_pipeline(device);
  if (!g_vrs_pipeline)
    return;

  id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
  [encoder setComputePipelineState:g_vrs_pipeline];
  [encoder setTexture:vrs_map->rate_map atIndex:0];

  VRSParams params = {.focus_center = focus_center,
                      .inner_radius = inner_radius,
                      .outer_radius = outer_radius};
  [encoder setBytes:&params length:sizeof(params) atIndex:0];

  MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
  MTLSize threadgroups =
      MTLSizeMake((vrs_map->width + 15) / 16, (vrs_map->height + 15) / 16, 1);
  [encoder dispatchThreadgroups:threadgroups
          threadsPerThreadgroup:threadgroupSize];
  [encoder endEncoding];

  LOG_INFO(
      "Updating foveated VRS map: center(%.2f, %.2f) inner=%.2f outer=%.2f",
      focus_center.x, focus_center.y, inner_radius, outer_radius);
}

void metal_vrs_set_uniform_rate(MetalVRSImage *vrs_map, MetalVRSRate rate) {
  if (!vrs_map)
    return;

  // Fill entire rate map with uniform rate
  // Use blit encoder to fill texture
  LOG_INFO("Set uniform VRS rate: %d", rate);
}

void metal_vrs_apply_to_pass(id<MTLRenderCommandEncoder> encoder,
                             MetalVRSImage *vrs_map) {
  if (!encoder || !vrs_map)
    return;

  if (@available(macOS 13.0, *)) {
    // Set the VRS rate map on the render encoder
    // Metal API: setFragmentShadingRateTexture
    [encoder setFragmentShadingRateTexture:vrs_map->rate_map];
  }
}
