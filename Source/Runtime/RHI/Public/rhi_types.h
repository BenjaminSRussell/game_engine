/**
 * @file rhi_types.h
 * @brief Render Hardware Interface type definitions
 */

#ifndef VOXELFORGE_RHI_TYPES_H
#define VOXELFORGE_RHI_TYPES_H

#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct RHI_Device RHI_Device;
typedef struct RHI_Queue RHI_Queue;
typedef struct RHI_Swapchain RHI_Swapchain;
typedef struct RHI_CommandPool RHI_CommandPool;
typedef struct RHI_CommandBuffer RHI_CommandBuffer;
typedef struct RHI_Buffer RHI_Buffer;
typedef struct RHI_Texture RHI_Texture;
typedef struct RHI_TextureView RHI_TextureView;
typedef struct RHI_Sampler RHI_Sampler;
typedef struct RHI_Shader RHI_Shader;
typedef struct RHI_Pipeline RHI_Pipeline;
typedef struct RHI_RenderPass RHI_RenderPass;
typedef struct RHI_Framebuffer RHI_Framebuffer;
typedef struct RHI_Fence RHI_Fence;
typedef struct RHI_Semaphore RHI_Semaphore;
typedef struct RHI_DescriptorSetLayout RHI_DescriptorSetLayout;
typedef struct RHI_DescriptorSet RHI_DescriptorSet;
typedef struct RHI_PipelineLayout RHI_PipelineLayout;

// ============================================================================
// Backend Type
// ============================================================================

typedef enum RHI_Backend {
  RHI_BACKEND_VULKAN = 0,
  RHI_BACKEND_METAL = 1,
  RHI_BACKEND_D3D12 = 2,
  RHI_BACKEND_COUNT
} RHI_Backend;

// ============================================================================
// Queue Types
// ============================================================================

typedef enum RHI_QueueType {
  RHI_QUEUE_GRAPHICS = 0,
  RHI_QUEUE_COMPUTE = 1,
  RHI_QUEUE_TRANSFER = 2,
  RHI_QUEUE_COUNT
} RHI_QueueType;

// ============================================================================
// Format Types
// ============================================================================

typedef enum RHI_Format {
  RHI_FORMAT_UNDEFINED = 0,

  // 8-bit
  RHI_FORMAT_R8_UNORM,
  RHI_FORMAT_R8_SNORM,
  RHI_FORMAT_R8_UINT,
  RHI_FORMAT_R8_SINT,

  // 16-bit
  RHI_FORMAT_R16_FLOAT,
  RHI_FORMAT_R16_UNORM,
  RHI_FORMAT_RG8_UNORM,

  // 32-bit
  RHI_FORMAT_R32_FLOAT,
  RHI_FORMAT_R32_UINT,
  RHI_FORMAT_R32_SINT,
  RHI_FORMAT_RG16_FLOAT,
  RHI_FORMAT_RGBA8_UNORM,
  RHI_FORMAT_RGBA8_SRGB,
  RHI_FORMAT_BGRA8_UNORM,
  RHI_FORMAT_BGRA8_SRGB,
  RHI_FORMAT_RGB10A2_UNORM,
  RHI_FORMAT_RG11B10_FLOAT,

  // 64-bit
  RHI_FORMAT_RG32_FLOAT,
  RHI_FORMAT_RGBA16_FLOAT,

  // 128-bit
  RHI_FORMAT_RGBA32_FLOAT,

  // Depth/Stencil
  RHI_FORMAT_D16_UNORM,
  RHI_FORMAT_D24_UNORM_S8_UINT,
  RHI_FORMAT_D32_FLOAT,
  RHI_FORMAT_D32_FLOAT_S8_UINT,

  // Compressed
  RHI_FORMAT_BC1_UNORM,
  RHI_FORMAT_BC1_SRGB,
  RHI_FORMAT_BC3_UNORM,
  RHI_FORMAT_BC3_SRGB,
  RHI_FORMAT_BC5_UNORM,
  RHI_FORMAT_BC7_UNORM,
  RHI_FORMAT_BC7_SRGB,

  RHI_FORMAT_COUNT
} RHI_Format;

// ============================================================================
// Buffer Usage
// ============================================================================

typedef enum RHI_BufferUsage {
  RHI_BUFFER_USAGE_VERTEX = (1 << 0),
  RHI_BUFFER_USAGE_INDEX = (1 << 1),
  RHI_BUFFER_USAGE_UNIFORM = (1 << 2),
  RHI_BUFFER_USAGE_STORAGE = (1 << 3),
  RHI_BUFFER_USAGE_INDIRECT = (1 << 4),
  RHI_BUFFER_USAGE_TRANSFER_SRC = (1 << 5),
  RHI_BUFFER_USAGE_TRANSFER_DST = (1 << 6),
} RHI_BufferUsage;

// ============================================================================
// Texture Usage
// ============================================================================

typedef enum RHI_TextureUsage {
  RHI_TEXTURE_USAGE_SAMPLED = (1 << 0),
  RHI_TEXTURE_USAGE_STORAGE = (1 << 1),
  RHI_TEXTURE_USAGE_COLOR_ATTACHMENT = (1 << 2),
  RHI_TEXTURE_USAGE_DEPTH_STENCIL = (1 << 3),
  RHI_TEXTURE_USAGE_TRANSFER_SRC = (1 << 4),
  RHI_TEXTURE_USAGE_TRANSFER_DST = (1 << 5),
} RHI_TextureUsage;

// ============================================================================
// Memory Type
// ============================================================================

typedef enum RHI_MemoryType {
  RHI_MEMORY_DEVICE = 0, // GPU-only, fastest
  RHI_MEMORY_HOST = 1,   // CPU-accessible staging
  RHI_MEMORY_SHARED = 2, // Unified/shared memory
} RHI_MemoryType;

// ============================================================================
// Shader Stage
// ============================================================================

typedef enum RHI_ShaderStage {
  RHI_SHADER_STAGE_VERTEX = (1 << 0),
  RHI_SHADER_STAGE_FRAGMENT = (1 << 1),
  RHI_SHADER_STAGE_COMPUTE = (1 << 2),
  RHI_SHADER_STAGE_GEOMETRY = (1 << 3),
  RHI_SHADER_STAGE_TESSELLATION_CONTROL = (1 << 4),
  RHI_SHADER_STAGE_TESSELLATION_EVAL = (1 << 5),
  RHI_SHADER_STAGE_RAYGEN = (1 << 6),
  RHI_SHADER_STAGE_MISS = (1 << 7),
  RHI_SHADER_STAGE_CLOSEST_HIT = (1 << 8),
  RHI_SHADER_STAGE_ANY_HIT = (1 << 9),
  RHI_SHADER_STAGE_INTERSECTION = (1 << 10),
} RHI_ShaderStage;

// ============================================================================
// Primitive Topology
// ============================================================================

typedef enum RHI_PrimitiveTopology {
  RHI_PRIMITIVE_TRIANGLES = 0,
  RHI_PRIMITIVE_TRIANGLE_STRIP,
  RHI_PRIMITIVE_LINES,
  RHI_PRIMITIVE_LINE_STRIP,
  RHI_PRIMITIVE_POINTS,
} RHI_PrimitiveTopology;

// ============================================================================
// Cull Mode
// ============================================================================

typedef enum RHI_CullMode {
  RHI_CULL_NONE = 0,
  RHI_CULL_FRONT = 1,
  RHI_CULL_BACK = 2,
} RHI_CullMode;

// ============================================================================
// Compare Op
// ============================================================================

typedef enum RHI_CompareOp {
  RHI_COMPARE_NEVER = 0,
  RHI_COMPARE_LESS,
  RHI_COMPARE_EQUAL,
  RHI_COMPARE_LESS_EQUAL,
  RHI_COMPARE_GREATER,
  RHI_COMPARE_NOT_EQUAL,
  RHI_COMPARE_GREATER_EQUAL,
  RHI_COMPARE_ALWAYS,
} RHI_CompareOp;

// ============================================================================
// Blend Factor
// ============================================================================

typedef enum RHI_BlendFactor {
  RHI_BLEND_ZERO = 0,
  RHI_BLEND_ONE,
  RHI_BLEND_SRC_COLOR,
  RHI_BLEND_ONE_MINUS_SRC_COLOR,
  RHI_BLEND_SRC_ALPHA,
  RHI_BLEND_ONE_MINUS_SRC_ALPHA,
  RHI_BLEND_DST_COLOR,
  RHI_BLEND_ONE_MINUS_DST_COLOR,
  RHI_BLEND_DST_ALPHA,
  RHI_BLEND_ONE_MINUS_DST_ALPHA,
} RHI_BlendFactor;

// ============================================================================
// Load/Store Op
// ============================================================================

typedef enum RHI_LoadOp {
  RHI_LOAD_OP_LOAD = 0,
  RHI_LOAD_OP_CLEAR,
  RHI_LOAD_OP_DONT_CARE,
} RHI_LoadOp;

typedef enum RHI_StoreOp {
  RHI_STORE_OP_STORE = 0,
  RHI_STORE_OP_DONT_CARE,
} RHI_StoreOp;

// ============================================================================
// Filter Mode
// ============================================================================

typedef enum RHI_FilterMode {
  RHI_FILTER_NEAREST = 0,
  RHI_FILTER_LINEAR,
} RHI_FilterMode;

// ============================================================================
// Address Mode
// ============================================================================

typedef enum RHI_AddressMode {
  RHI_ADDRESS_REPEAT = 0,
  RHI_ADDRESS_MIRRORED_REPEAT,
  RHI_ADDRESS_CLAMP_TO_EDGE,
  RHI_ADDRESS_CLAMP_TO_BORDER,
} RHI_AddressMode;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_RHI_TYPES_H
