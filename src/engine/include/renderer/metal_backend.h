// Metal Backend Header - macOS/iOS Optimized Rendering
// METAL-001: Device setup and management
// METAL-002: Command queue management
// METAL-003: Shader library and compilation
// METAL-004: Render pipeline creation
// METAL-005: Compute pipeline creation
// METAL-006: Buffer management
// METAL-007: Texture management
// METAL-008: Sampler management
// METAL-009: Render pass management
// METAL-010: Command encoding

#ifndef METAL_BACKEND_H
#define METAL_BACKEND_H

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for Metal types
#ifdef __APPLE__
@class MTLDevice, MTLCommandQueue, MTLLibrary, CAMetalLayer;
@protocol MTLRenderPipelineState, MTLBuffer, MTLTexture;
#endif

// Metal pixel formats
typedef enum {
    METAL_PIXEL_FORMAT_R8_UNORM = 0,
    METAL_PIXEL_FORMAT_R8G8_UNORM,
    METAL_PIXEL_FORMAT_R8G8B8A8_UNORM,
    METAL_PIXEL_FORMAT_B8G8R8A8_UNORM,
    METAL_PIXEL_FORMAT_R16_FLOAT,
    METAL_PIXEL_FORMAT_R16G16_FLOAT,
    METAL_PIXEL_FORMAT_R16G16B16A16_FLOAT,
    METAL_PIXEL_FORMAT_R32_FLOAT,
    METAL_PIXEL_FORMAT_R32G32_FLOAT,
    METAL_PIXEL_FORMAT_R32G32B32A32_FLOAT,
    METAL_PIXEL_FORMAT_DEPTH32_FLOAT
} MetalPixelFormat;

// Metal buffer usage flags
typedef enum {
    METAL_BUFFER_USAGE_VERTEX = 0x01,
    METAL_BUFFER_USAGE_INDEX = 0x02,
    METAL_BUFFER_USAGE_UNIFORM = 0x04,
    METAL_BUFFER_USAGE_STORAGE = 0x08,
    METAL_BUFFER_USAGE_PRIVATE = 0x10,
    METAL_BUFFER_USAGE_MANAGED = 0x20
} MetalBufferUsage;

// Metal vertex formats
typedef enum {
    METAL_VERTEX_FORMAT_FLOAT = 0,
    METAL_VERTEX_FORMAT_FLOAT2,
    METAL_VERTEX_FORMAT_FLOAT3,
    METAL_VERTEX_FORMAT_FLOAT4,
    METAL_VERTEX_FORMAT_INT,
    METAL_VERTEX_FORMAT_INT2,
    METAL_VERTEX_FORMAT_INT3,
    METAL_VERTEX_FORMAT_INT4,
    METAL_VERTEX_FORMAT_UINT,
    METAL_VERTEX_FORMAT_UINT2,
    METAL_VERTEX_FORMAT_UINT3,
    METAL_VERTEX_FORMAT_UINT4
} MetalVertexFormat;

// Metal blend operations
typedef enum {
    METAL_BLEND_OP_ADD = 0,
    METAL_BLEND_OP_SUBTRACT,
    METAL_BLEND_OP_REVERSE_SUBTRACT,
    METAL_BLEND_OP_MIN,
    METAL_BLEND_OP_MAX
} MetalBlendOp;

// Metal blend factors
typedef enum {
    METAL_BLEND_FACTOR_ZERO = 0,
    METAL_BLEND_FACTOR_ONE,
    METAL_BLEND_FACTOR_SRC_COLOR,
    METAL_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    METAL_BLEND_FACTOR_DST_COLOR,
    METAL_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    METAL_BLEND_FACTOR_SRC_ALPHA,
    METAL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    METAL_BLEND_FACTOR_DST_ALPHA,
    METAL_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    METAL_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    METAL_BLEND_FACTOR_BLEND_COLOR,
    METAL_BLEND_FACTOR_ONE_MINUS_BLEND_COLOR
} MetalBlendFactor;

// Metal vertex step functions
typedef enum {
    METAL_VERTEX_STEP_CONSTANT = 0,
    METAL_VERTEX_STEP_PER_VERTEX,
    METAL_VERTEX_STEP_PER_INSTANCE
} MetalVertexStepFunction;

// Metal vertex attribute descriptor
typedef struct {
    MetalVertexFormat format;
    u32 offset;
    u32 bufferIndex;
} MetalVertexAttribute;

// Metal vertex buffer descriptor
typedef struct {
    u32 stride;
    MetalVertexStepFunction stepFunction;
    u32 stepRate;
} MetalVertexBufferLayout;

// Metal vertex layout descriptor
typedef struct {
    MetalVertexAttribute* attributes;
    u32 attributeCount;
    MetalVertexBufferLayout* buffers;
    u32 bufferCount;
} MetalVertexLayout;

// Metal color attachment descriptor
typedef struct {
    MetalPixelFormat pixelFormat;
    bool blendingEnabled;
    MetalBlendOp rgbBlendOp;
    MetalBlendOp alphaBlendOp;
    MetalBlendFactor srcRGBBlend;
    MetalBlendFactor srcAlphaBlend;
    MetalBlendFactor dstRGBBlend;
    MetalBlendFactor dstAlphaBlend;
} MetalColorAttachment;

// Metal pipeline descriptor
typedef struct {
    const char* vertexFunction;
    const char* fragmentFunction;
    MetalVertexLayout* vertexLayout;
    MetalColorAttachment* colorAttachments;
    u32 colorAttachmentCount;
    bool depthAttachmentEnabled;
    MetalPixelFormat depthAttachmentFormat;
} MetalPipelineDesc;

// Metal pipeline state wrapper
typedef struct MetalPipelineState {
#ifdef __APPLE__
    id<MTLRenderPipelineState> pipeline;
#endif
} MetalPipelineState;

// Metal buffer wrapper
typedef struct MetalBuffer {
#ifdef __APPLE__
    id<MTLBuffer> buffer;
#endif
    u64 size;
    MetalBufferUsage usage;
    const char* label;
} MetalBuffer;

// Metal texture wrapper
typedef struct MetalTexture {
#ifdef __APPLE__
    id<MTLTexture> texture;
#endif
    u32 width;
    u32 height;
    MetalPixelFormat format;
} MetalTexture;

// Metal sampler wrapper
typedef struct MetalSampler {
#ifdef __APPLE__
    id<MTLSamplerState> sampler;
#endif
} MetalSampler;

// Metal command buffer wrapper
typedef struct MetalCommandBuffer {
#ifdef __APPLE__
    id<MTLCommandBuffer> commandBuffer;
#endif
} MetalCommandBuffer;

// Metal render pass descriptor
typedef struct MetalRenderPassDesc {
    MetalTexture* colorTexture;
    MetalTexture* depthTexture;
    bool clearColor;
    float clearR, clearG, clearB, clearA;
    bool clearDepth;
    float clearDepthValue;
} MetalRenderPassDesc;

// Metal renderer statistics
typedef struct {
    u64 drawCalls;
    u64 computeDispatches;
    u64 bufferAllocations;
    u64 textureAllocations;
    u64 pipelineCreations;
    double frameTime;
    u64 memoryUsage;
} MetalStats;

// Core Metal backend functions
bool metal_init(void* window_handle);
void metal_shutdown(void);
bool metal_is_initialized(void);

// Pipeline management
MetalPipelineState* metal_create_render_pipeline(const MetalPipelineDesc* desc);
void metal_destroy_pipeline(MetalPipelineState* pipeline);

// Buffer management
MetalBuffer* metal_create_buffer(u64 size, MetalBufferUsage usage);
void metal_destroy_buffer(MetalBuffer* buffer);
void* metal_map_buffer(MetalBuffer* buffer);
void metal_unmap_buffer(MetalBuffer* buffer);

// Texture management
MetalTexture* metal_create_texture(u32 width, u32 height, MetalPixelFormat format);
void metal_destroy_texture(MetalTexture* texture);
void metal_update_texture(MetalTexture* texture, const void* data, u32 size);

// Sampler management
MetalSampler* metal_create_sampler(void);
void metal_destroy_sampler(MetalSampler* sampler);

// Command encoding
MetalCommandBuffer* metal_create_command_buffer(void);
void metal_begin_render_pass(MetalCommandBuffer* cmdBuffer, const MetalRenderPassDesc* desc);
void metal_end_render_pass(MetalCommandBuffer* cmdBuffer);
void metal_set_pipeline(MetalCommandBuffer* cmdBuffer, MetalPipelineState* pipeline);
void metal_set_vertex_buffer(MetalCommandBuffer* cmdBuffer, MetalBuffer* buffer, u32 index);
void metal_set_fragment_buffer(MetalCommandBuffer* cmdBuffer, MetalBuffer* buffer, u32 index);
void metal_set_texture(MetalCommandBuffer* cmdBuffer, MetalTexture* texture, u32 index);
void metal_set_sampler(MetalCommandBuffer* cmdBuffer, MetalSampler* sampler, u32 index);
void metal_draw(MetalCommandBuffer* cmdBuffer, u32 vertexCount, u32 instanceCount);
void metal_draw_indexed(MetalCommandBuffer* cmdBuffer, u32 indexCount, u32 instanceCount);
void metal_commit_command_buffer(MetalCommandBuffer* cmdBuffer);

// Statistics and debugging
void metal_get_stats(MetalStats* outStats);
void metal_reset_stats(void);
void metal_debug_print_stats(void);

// Metal 3 specific features (macOS 10.15+)
bool metal_supports_mesh_shaders(void);
bool metal_supports_raytracing(void);
bool metal_supports_indirect_command_buffers(void);

#ifdef __cplusplus
}
#endif

#endif // METAL_BACKEND_H
