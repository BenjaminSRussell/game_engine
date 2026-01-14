// include/rendering/metal_raytracing.h
//
// Metal Hardware Ray Tracing API
// Native acceleration structure support for Apple Silicon
//
#ifndef METAL_RAYTRACING_H
#define METAL_RAYTRACING_H

#include "../common.h"
#include "../math/mat4.h"
#include "../math/vec2.h"
#include "../math/vec3.h"
#include <Metal/Metal.h>

typedef struct MetalRTAccelStructure MetalRTAccelStructure;
typedef struct MetalRTPipeline MetalRTPipeline;

// Geometry types
typedef enum {
  MTL_RT_GEOMETRY_TRIANGLES,
  MTL_RT_GEOMETRY_BOUNDING_BOX,
  MTL_RT_GEOMETRY_CURVE
} MetalRTGeometryType;

// Ray intersection result
typedef struct {
  f32 distance;
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  u32 primitive_id;
  u32 instance_id;
  bool hit;
} MetalRTHitResult;

#ifdef __cplusplus
extern "C" {
#endif

// Capability check
bool metal_rt_is_supported(id<MTLDevice> device);

// Acceleration structure lifecycle
MetalRTAccelStructure *
metal_rt_create_triangle_accel(id<MTLDevice> device,
                               id<MTLBuffer> vertex_buffer,
                               id<MTLBuffer> index_buffer, u32 triangle_count);

MetalRTAccelStructure *
metal_rt_create_instance_accel(id<MTLDevice> device,
                               MetalRTAccelStructure **geometries,
                               Mat4 *transforms, u32 instance_count);

void metal_rt_rebuild_accel(MetalRTAccelStructure *accel,
                            id<MTLCommandBuffer> cmd);
void metal_rt_destroy_accel(MetalRTAccelStructure *accel);

// Ray tracing pipeline
MetalRTPipeline *metal_rt_create_pipeline(id<MTLDevice> device,
                                          id<MTLFunction> ray_gen,
                                          id<MTLFunction> closest_hit,
                                          id<MTLFunction> miss);

void metal_rt_trace_rays(MetalRTPipeline *pipeline, id<MTLCommandBuffer> cmd,
                         MetalRTAccelStructure *scene, id<MTLTexture> output,
                         u32 width, u32 height);

void metal_rt_destroy_pipeline(MetalRTPipeline *pipeline);

#ifdef __cplusplus
}
#endif

#endif // METAL_RAYTRACING_H
