// fluid_sim.m - Implementation
#include "include/physics/fluid_sim.h"
#include "include/core/logger.h"
#import <Metal/Metal.h>
#include <stdlib.h>

FluidGrid *fluid_grid_create(id<MTLDevice> device) {
  FluidGrid *fluid = calloc(1, sizeof(FluidGrid));
  fluid->viscosity = 0.001f;
  fluid->diffusion = 0.0001f;

  MTLTextureDescriptor *desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float
                                   width:FLUID_GRID_SIZE
                                  height:FLUID_GRID_SIZE
                               mipmapped:NO];
  desc.textureType = MTLTextureType3D;
  desc.depth = FLUID_GRID_SIZE;
  desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;

  fluid->density_texture = [device newTextureWithDescriptor:desc];
  fluid->velocity_texture =
      [device newTextureWithDescriptor:desc]; // Actually needs RGBA32Float

  LOG_INFO("Fluid grid created");
  return fluid;
}

void fluid_grid_update(FluidGrid *fluid, id<MTLCommandBuffer> cmd,
                       f32 delta_time) {
  id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];
  // Dispatch advect, diffuse, project
  [compute endEncoding];
}
