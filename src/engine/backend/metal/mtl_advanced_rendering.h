// Metal Advanced Rendering Features
// Provides advanced Metal rendering capabilities for the engine

#ifndef MTL_ADVANCED_RENDERING_H
#define MTL_ADVANCED_RENDERING_H

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Advanced Metal rendering features
typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLDepthStencilState> depthStencilState;
    id<MTLTexture> depthTexture;
    id<MTLTexture> colorTexture;
    MTLRenderPassDescriptor* renderPassDescriptor;
    bool isInitialized;
} MTLAdvancedRenderer;

// Ray tracing support
typedef struct {
    id<MTLAccelerationStructure> accelerationStructure;
    id<MTLBuffer> raygenBuffer;
    id<MTLBuffer> missBuffer;
    id<MTLBuffer> hitGroupBuffer;
    id<MTLComputePipelineState> raytracingPipeline;
    bool isRayTracingEnabled;
} MTLRayTracingContext;

// Variable Rate Shading
typedef struct {
    id<MTLTexture> shadingRateTexture;
    MTLShadingRate shadingRate;
    bool isVREnabled;
} MTLVariableRateShading;

// Function declarations
bool mtl_advanced_renderer_init(MTLAdvancedRenderer* renderer, id<MTLDevice> device);
void mtl_advanced_renderer_cleanup(MTLAdvancedRenderer* renderer);
void mtl_advanced_renderer_begin_frame(MTLAdvancedRenderer* renderer);
void mtl_advanced_renderer_end_frame(MTLAdvancedRenderer* renderer);
void mtl_advanced_renderer_draw(MTLAdvancedRenderer* renderer, id<MTLBuffer> vertexBuffer, uint32_t vertexCount);

// Ray tracing functions
bool mtl_raytracing_init(MTLRayTracingContext* context, id<MTLDevice> device);
void mtl_raytracing_cleanup(MTLRayTracingContext* context);
void mtl_raytracing_trace_rays(MTLRayTracingContext* context, id<MTLCommandBuffer> commandBuffer);

// Variable Rate Shading functions
bool mtl_vrs_init(MTLVariableRateShading* vrs, id<MTLDevice> device);
void mtl_vrs_cleanup(MTLVariableRateShading* vrs);
void mtl_vrs_set_shading_rate(MTLVariableRateShading* vrs, MTLShadingRate rate);

// Performance monitoring
typedef struct {
    uint64_t frameTime;
    uint64_t gpuTime;
    uint32_t drawCalls;
    uint32_t triangles;
    float frameRate;
} MTLPerformanceStats;

void mtl_get_performance_stats(MTLPerformanceStats* stats);
void mtl_reset_performance_counters(void);

#ifdef __cplusplus
}
#endif

#endif // MTL_ADVANCED_RENDERING_H
