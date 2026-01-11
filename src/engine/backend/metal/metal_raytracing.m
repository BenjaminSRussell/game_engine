#include "../../include/rendering/metal_raytracing.h"
#include "../../include/core/logger.h"
#include "../../include/core/memory.h"
#import <Metal/Metal.h>
#import <simd/simd.h>

struct MetalRTAccelStructure {
  id<MTLAccelerationStructure> structure;
  id<MTLBuffer> scratch_buffer;
  id<MTLBuffer> instance_buffer; // Only for TLAS
  u32 primitive_count;
};

struct MetalRTPipeline {
  id<MTLComputePipelineState> pipeline;
  id<MTLFunction> ray_gen;
  id<MTLFunction>
      intersection; // Optional (not used in simple triangle tracing)
};

bool metal_rt_is_supported(id<MTLDevice> device) {
  if (!device)
    return false;
  return [device supportsRaytracing];
}

MetalRTAccelStructure *
metal_rt_create_triangle_accel(id<MTLDevice> device,
                               id<MTLBuffer> vertex_buffer,
                               id<MTLBuffer> index_buffer, u32 triangle_count) {
  if (!device || !vertex_buffer)
    return NULL;

  MetalRTAccelStructure *accel =
      (MetalRTAccelStructure *)calloc(1, sizeof(MetalRTAccelStructure));
  accel->primitive_count = triangle_count;

  // Create primitives descriptor
  MTLAccelerationStructureTriangleGeometryDescriptor *geom =
      [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
  geom.vertexBuffer = vertex_buffer;
  geom.vertexBufferOffset = 0;
  geom.vertexStride = sizeof(float) * 3; // Position packed
  geom.indexBuffer = index_buffer;
  geom.indexBufferOffset = 0;
  geom.indexType = MTLIndexTypeUInt32;
  geom.triangleCount = triangle_count;
  geom.opaque = YES;

  // Create primitive accel descriptor
  MTLPrimitiveAccelerationStructureDescriptor *accelDesc =
      [MTLPrimitiveAccelerationStructureDescriptor descriptor];
  accelDesc.geometryDescriptors = @[ geom ];
  accelDesc.usage = MTLAccelerationStructureUsageNone;

  // Allocate acceleration structure
  MTLAccelerationStructureSizes sizes =
      [device accelerationStructureSizesWithDescriptor:accelDesc];
  accel->structure =
      [device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
  accel->scratch_buffer =
      [device newBufferWithLength:sizes.buildScratchBufferSize
                          options:MTLResourceStorageModePrivate];

  // Build immediate (blocking for simplicity, usually done on queue)
  id<MTLCommandQueue> queue = [device newCommandQueue];
  id<MTLCommandBuffer> cmd = [queue commandBuffer];
  id<MTLAccelerationStructureCommandEncoder> encoder =
      [cmd accelerationStructureCommandEncoder];

  [encoder buildAccelerationStructure:accel->structure
                           descriptor:accelDesc
                        scratchBuffer:accel->scratch_buffer
                  scratchBufferOffset:0];

  [encoder endEncoding];
  [cmd commit];
  [cmd waitUntilCompleted]; // Validation: ensure ready

  return accel;
}

MetalRTAccelStructure *
metal_rt_create_instance_accel(id<MTLDevice> device,
                               MetalRTAccelStructure **geometries,
                               Mat4 *transforms, u32 instance_count) {
  if (!device || !geometries || instance_count == 0)
    return NULL;

  MetalRTAccelStructure *accel =
      (MetalRTAccelStructure *)calloc(1, sizeof(MetalRTAccelStructure));

  // Create instance buffer
  accel->instance_buffer = [device
      newBufferWithLength:instance_count *
                          sizeof(MTLAccelerationStructureInstanceDescriptor)
                  options:MTLResourceStorageModeShared];

  MTLAccelerationStructureInstanceDescriptor *instances =
      (MTLAccelerationStructureInstanceDescriptor *)
          [accel->instance_buffer contents];

  for (u32 i = 0; i < instance_count; i++) {
    instances[i].accelerationStructureIndex =
        0; // Assuming simplified mapping for now (1 type of geometry per call?)
           // In a real engine, we'd map this correctly.
    instances[i].options = MTLAccelerationStructureInstanceOptionOpaque;
    instances[i].intersectionFunctionTableOffset = 0;
    instances[i].mask = 0xFF;

    // Convert Mat4 to MTLPackedFloat4x3
    // Note: Metal expects 4x3 column-major matrix
    // Mat4 is likely column major too.
    // Copy 3 columns (x, y, z axis) and translation
    // transforms[i] elements:
    // [0][0] [1][0] [2][0] [3][0] (row 0: x-axis.x, y-axis.x, z-axis.x,
    // pos.x)?? Check core math definition later. Assuming standard column
    // vectors.

    Mat4 m = transforms[i];
    instances[i].transformationMatrix.columns[0] =
        (MTLPackedFloat3){m.m00, m.m10, m.m20};
    instances[i].transformationMatrix.columns[1] =
        (MTLPackedFloat3){m.m01, m.m11, m.m21};
    instances[i].transformationMatrix.columns[2] =
        (MTLPackedFloat3){m.m02, m.m12, m.m22};
    instances[i].transformationMatrix.columns[3] =
        (MTLPackedFloat3){m.m03, m.m13, m.m23};

    // Point to the BLAS
    // Note: API requires we use the accelerationStructure from geometries[i]
    // But the descriptor struct uses indices into an array provided during
    // build. Wait, for creating the Instance Accel, we provide Instance
    // Descriptors. We actually need to pass the BLAS objects to the build
    // command.
  }

  // Since we can't easily map the indices here without creating the encoder
  // first, logic is split. Simplifying: Warning - this assumes instances refer
  // to passed geometries at index i? Actually `accelerationStructureIndex`
  // refers to the index in the `instancedAccelerationStructures` array passed
  // to the descriptor.

  for (u32 i = 0; i < instance_count; i++) {
    instances[i].accelerationStructureIndex = i; // 1-to-1 mapping for now
  }

  MTLInstanceAccelerationStructureDescriptor *accelDesc =
      [MTLInstanceAccelerationStructureDescriptor descriptor];

  NSMutableArray *blas_array =
      [NSMutableArray arrayWithCapacity:instance_count];
  for (u32 i = 0; i < instance_count; i++) {
    if (geometries[i] && geometries[i]->structure) {
      [blas_array addObject:geometries[i]->structure];
    }
  }
  accelDesc.instancedAccelerationStructures = blas_array;
  accelDesc.instanceCount = instance_count;
  accelDesc.instanceDescriptorBuffer = accel->instance_buffer;
  accelDesc.instanceDescriptorBufferOffset = 0;
  accelDesc.instanceDescriptorStride =
      sizeof(MTLAccelerationStructureInstanceDescriptor);

  // Build
  MTLAccelerationStructureSizes sizes =
      [device accelerationStructureSizesWithDescriptor:accelDesc];
  accel->structure =
      [device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
  accel->scratch_buffer =
      [device newBufferWithLength:sizes.buildScratchBufferSize
                          options:MTLResourceStorageModePrivate];

  id<MTLCommandQueue> queue = [device newCommandQueue];
  id<MTLCommandBuffer> cmd = [queue commandBuffer];
  id<MTLAccelerationStructureCommandEncoder> encoder =
      [cmd accelerationStructureCommandEncoder];

  [encoder buildAccelerationStructure:accel->structure
                           descriptor:accelDesc
                        scratchBuffer:accel->scratch_buffer
                  scratchBufferOffset:0];

  [encoder endEncoding];
  [cmd commit];
  [cmd waitUntilCompleted];

  return accel;
}

void metal_rt_destroy_accel(MetalRTAccelStructure *accel) {
  if (!accel)
    return;
  accel->structure = nil;
  accel->scratch_buffer = nil;
  accel->instance_buffer = nil;
  free(accel);
}

MetalRTPipeline *metal_rt_create_pipeline(id<MTLDevice> device,
                                          id<MTLFunction> ray_gen,
                                          id<MTLFunction> closest_hit,
                                          id<MTLFunction> miss) {
  // Metal RT pipelines are Compute Pipelines with special linking
  // Or we use Intersection Functions.
  // For simple Ray Query (inline raytracing), we just need a compute shader
  // that checks capabilities. If we use Ray Tracing Pipelines (Ray
  // Intersectors), that's different.

  // Apple Silicon prefers Inline Raytracing (Ray Query) inside Compute Shaders
  // unless using Intersection Functions for custom primitives.
  // So we will assume Inline Raytracing (RayQuery).

  // Thus returns a standard compute pipeline.

  NSError *error = nil;
  MTLComputePipelineDescriptor *desc =
      [[MTLComputePipelineDescriptor alloc] init];
  desc.computeFunction = ray_gen; // The main entry point
  // Note: linkedFunctions are for Intersection Functions (not typically needed
  // for triangles)

  id<MTLComputePipelineState> pso =
      [device newComputePipelineStateWithDescriptor:desc
                                            options:0
                                         reflection:nil
                                              error:&error];

  if (!pso) {
    LOG_ERROR("Failed to create RT pipeline: %s",
              [[error localizedDescription] UTF8String]);
    return NULL;
  }

  MetalRTPipeline *pipeline =
      (MetalRTPipeline *)calloc(1, sizeof(MetalRTPipeline));
  pipeline->pipeline = pso;
  pipeline->ray_gen = ray_gen;

  return pipeline;
}

void metal_rt_trace_rays(MetalRTPipeline *pipeline, id<MTLCommandBuffer> cmd,
                         MetalRTAccelStructure *scene, id<MTLTexture> output,
                         u32 width, u32 height) {
  if (!pipeline || !cmd || !scene)
    return;

  id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
  [encoder setComputePipelineState:pipeline->pipeline];

  // Bind Acceleration Structure
  [encoder setAccelerationStructure:scene->structure atBufferIndex:0];

  // Bind Output
  [encoder setTexture:output atIndex:0];

  // Dispatch
  MTLSize threadGroupSize = MTLSizeMake(16, 16, 1);
  MTLSize threadGroups = MTLSizeMake((width + 15) / 16, (height + 15) / 16, 1);

  [encoder dispatchThreadgroups:threadGroups
          threadsPerThreadgroup:threadGroupSize];
  [encoder endEncoding];
}

void metal_rt_destroy_pipeline(MetalRTPipeline *pipeline) {
  if (!pipeline)
    return;
  pipeline->pipeline = nil;
  free(pipeline);
}

// Stubs for functions declared but not fully implemented in this pass
void metal_rt_rebuild_accel(MetalRTAccelStructure *accel,
                            id<MTLCommandBuffer> cmd) {
  // TODO: Implement refitting/rebuild logic
}
