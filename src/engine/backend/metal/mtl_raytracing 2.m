// backend/metal/mtl_raytracing.m
// Metal Hardware Ray Tracing Implementation
#include "../../include/core/logger.h"
#include "../../include/rendering/metal_raytracing.h"
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

struct MetalRTAccelStructure {
  id<MTLAccelerationStructure> accel_structure;
  MPSTriangleAccelerationStructure *mps_accel; // For MPS-based RT
  id<MTLBuffer> scratch_buffer;
};

struct MetalRTPipeline {
  id<MTLComputePipelineState> ray_gen_pipeline;
  id<MTLIntersectionFunctionTable> intersection_table;
};

bool metal_rt_is_supported(id<MTLDevice> device) {
  if (@available(macOS 11.0, iOS 14.0, *)) {
    return [device supportsRaytracing];
  }
  return false;
}

MetalRTAccelStructure *
metal_rt_create_triangle_accel(id<MTLDevice> device,
                               id<MTLBuffer> vertex_buffer,
                               id<MTLBuffer> index_buffer, u32 triangle_count) {
  if (@available(macOS 12.0, *)) {
    MPSTriangleAccelerationStructure *accel =
        [[MPSTriangleAccelerationStructure alloc] initWithDevice:device];
    accel.vertexBuffer = vertex_buffer;
    accel.vertexStride = sizeof(f32) * 8; // pos(3) + normal(3) + uv(2)
    accel.indexBuffer = index_buffer;
    accel.indexType = MPSDataTypeUInt32;
    accel.triangleCount = triangle_count;
    accel.usage = MPSAccelerationStructureUsageRefit |
                  MPSAccelerationStructureUsagePreferGPUBuild;

    [accel rebuild];

    MetalRTAccelStructure *rt_accel =
        (MetalRTAccelStructure *)calloc(1, sizeof(MetalRTAccelStructure));
    rt_accel->mps_accel = accel;

    LOG_INFO("Created RT acceleration structure with %u triangles",
             triangle_count);
    return rt_accel;
  }
  return NULL;
}

MetalRTAccelStructure *
metal_rt_create_instance_accel(id<MTLDevice> device,
                               MetalRTAccelStructure **geometries,
                               Mat4 *transforms, u32 instance_count) {
  if (@available(macOS 12.0, *)) {
    MPSInstanceAccelerationStructure *instance_accel =
        [[MPSInstanceAccelerationStructure alloc] initWithDevice:device];

    NSMutableArray *accel_array =
        [NSMutableArray arrayWithCapacity:instance_count];
    for (u32 i = 0; i < instance_count; i++) {
      if (geometries[i] && geometries[i]->mps_accel) {
        [accel_array addObject:geometries[i]->mps_accel];
      }
    }

    instance_accel.accelerationStructures = accel_array;
    instance_accel.instanceCount = instance_count;

    // Set transforms from Mat4 array
    if (transforms && instance_count > 0) {
      // Create a buffer for the 4x3 transforms (MPS expects float4x3)
      // Each float4x3 is 12 floats (3 rows, 4 columns or 4 columns, 3 rows
      // depending on layout) MPS expects column-major 4x3: [c0.xyz, c1.xyz,
      // c2.xyz, c3.xyz]
      u32 transform_size = sizeof(float) * 12 * instance_count;
      id<MTLBuffer> transform_buffer =
          [device newBufferWithLength:transform_size
                              options:MTLResourceStorageModeShared];
      float *dst = (float *)transform_buffer.contents;
      for (u32 i = 0; i < instance_count; i++) {
        // Copy 4 columns, 3 rows each
        Mat4 m = transforms[i];
        dst[i * 12 + 0] = m.m00;
        dst[i * 12 + 1] = m.m10;
        dst[i * 12 + 2] = m.m20;
        dst[i * 12 + 3] = m.m01;
        dst[i * 12 + 4] = m.m11;
        dst[i * 12 + 5] = m.m21;
        dst[i * 12 + 6] = m.m02;
        dst[i * 12 + 7] = m.m12;
        dst[i * 12 + 8] = m.m22;
        dst[i * 12 + 9] = m.m03;
        dst[i * 12 + 10] = m.m13;
        dst[i * 12 + 11] = m.m23;
      }
      instance_accel.instanceBuffer = transform_buffer;
    }

    [instance_accel rebuild];

    MetalRTAccelStructure *rt_accel =
        (MetalRTAccelStructure *)calloc(1, sizeof(MetalRTAccelStructure));
    rt_accel->mps_accel = instance_accel;

    return rt_accel;
  }
  return NULL;
}

void metal_rt_rebuild_accel(MetalRTAccelStructure *accel,
                            id<MTLCommandBuffer> cmd) {
  if (!accel)
    return;
  if (@available(macOS 12.0, *)) {
    if (accel->mps_accel) {
      [accel->mps_accel rebuild];
    }
  }
}

void metal_rt_destroy_accel(MetalRTAccelStructure *accel) {
  if (!accel)
    return;
  accel->mps_accel = nil;
  accel->accel_structure = nil;
  accel->scratch_buffer = nil;
  free(accel);
}

MetalRTPipeline *metal_rt_create_pipeline(id<MTLDevice> device,
                                          id<MTLFunction> ray_gen,
                                          id<MTLFunction> closest_hit,
                                          id<MTLFunction> miss) {
  if (@available(macOS 12.0, *)) {
    NSError *error = nil;

    id<MTLComputePipelineState> pipeline =
        [device newComputePipelineStateWithFunction:ray_gen error:&error];
    if (!pipeline) {
      LOG_ERROR("Failed to create RT pipeline: %s",
                [[error localizedDescription] UTF8String]);
      return NULL;
    }

    MetalRTPipeline *rt_pipeline =
        (MetalRTPipeline *)calloc(1, sizeof(MetalRTPipeline));
    rt_pipeline->ray_gen_pipeline = pipeline;

    return rt_pipeline;
  }
  return NULL;
}

void metal_rt_trace_rays(MetalRTPipeline *pipeline, id<MTLCommandBuffer> cmd,
                         MetalRTAccelStructure *scene, id<MTLTexture> output,
                         u32 width, u32 height) {
  if (!pipeline || !scene)
    return;

  if (@available(macOS 12.0, *)) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setComputePipelineState:pipeline->ray_gen_pipeline];
    [encoder setTexture:output atIndex:0];

    // Bind acceleration structure
    if (scene->accel_structure) {
      [encoder setAccelerationStructure:scene->accel_structure atBufferIndex:0];
    }

    MTLSize grid_size = MTLSizeMake(width, height, 1);
    NSUInteger w = pipeline->ray_gen_pipeline.threadExecutionWidth;
    NSUInteger h = pipeline->ray_gen_pipeline.maxTotalThreadsPerThreadgroup / w;
    MTLSize thread_group_size = MTLSizeMake(w, h, 1);

    [encoder dispatchThreads:grid_size threadsPerThreadgroup:thread_group_size];
    [encoder endEncoding];
  }
}

void metal_rt_destroy_pipeline(MetalRTPipeline *pipeline) {
  if (!pipeline)
    return;
  pipeline->ray_gen_pipeline = nil;
  pipeline->intersection_table = nil;
  free(pipeline);
}
