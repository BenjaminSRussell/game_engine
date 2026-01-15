#pragma once

#include <stdbool.h>
#include <stdint.h>

// RHI (Render Hardware Interface) - Platform-agnostic GPU abstraction

// Forward declarations
typedef struct RHIDevice RHIDevice;
typedef struct RHICommandList RHICommandList;
typedef struct RHIBuffer RHIBuffer;
typedef struct RHITexture RHITexture;
typedef struct RHIPipeline RHIPipeline;
typedef struct RHIShader RHIShader;

// Resource types
typedef enum {
  RHI_BUFFER_VERTEX,
  RHI_BUFFER_INDEX,
  RHI_BUFFER_UNIFORM,
  RHI_BUFFER_STORAGE
} RHIBufferType;

typedef enum {
  RHI_TEXTURE_2D,
  RHI_TEXTURE_3D,
  RHI_TEXTURE_CUBE,
  RHI_TEXTURE_2D_ARRAY
} RHITextureType;

// Usage flags
typedef enum {
  RHI_USAGE_STATIC = 1 << 0,  // GPU read-only, set once
  RHI_USAGE_DYNAMIC = 1 << 1, // CPU updates frequently
  RHI_USAGE_STAGING = 1 << 2, // CPU → GPU transfer
  RHI_USAGE_RENDER_TARGET = 1 << 3,
  RHI_USAGE_DEPTH_STENCIL = 1 << 4
} RHIUsageFlags;

// Shader stages
typedef enum {
  RHI_SHADER_VERTEX = 1 << 0,
  RHI_SHADER_FRAGMENT = 1 << 1,
  RHI_SHADER_COMPUTE = 1 << 2,
  RHI_SHADER_GEOMETRY = 1 << 3
} RHIShaderStage;

// Primitive topology
typedef enum {
  RHI_TOPOLOGY_TRIANGLES,
  RHI_TOPOLOGY_TRIANGLE_STRIP,
  RHI_TOPOLOGY_LINES,
  RHI_TOPOLOGY_POINTS
} RHITopology;

// Comparison functions
typedef enum {
  RHI_COMPARE_NEVER,
  RHI_COMPARE_LESS,
  RHI_COMPARE_EQUAL,
  RHI_COMPARE_LESS_EQUAL,
  RHI_COMPARE_GREATER,
  RHI_COMPARE_NOT_EQUAL,
  RHI_COMPARE_GREATER_EQUAL,
  RHI_COMPARE_ALWAYS
} RHICompareFunc;

// Blend factors
typedef enum {
  RHI_BLEND_ZERO,
  RHI_BLEND_ONE,
  RHI_BLEND_SRC_ALPHA,
  RHI_BLEND_ONE_MINUS_SRC_ALPHA,
  RHI_BLEND_DST_ALPHA,
  RHI_BLEND_ONE_MINUS_DST_ALPHA
} RHIBlendFactor;

// Clear values
typedef struct {
  float color[4];
  float depth;
  uint32_t stencil;
} RHIClearValue;

// Viewport
typedef struct {
  float x, y;
  float width, height;
  float min_depth, max_depth;
} RHIViewport;

// Scissor rect
typedef struct {
  int32_t x, y;
  uint32_t width, height;
} RHIScissor;

// Buffer descriptor
typedef struct {
  RHIBufferType type;
  uint32_t size;
  RHIUsageFlags usage;
  const void *initial_data; // Optional
} RHIBufferDesc;

// Texture descriptor
typedef struct {
  RHITextureType type;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t mip_levels;
  uint32_t array_layers;
  uint32_t format; // RenderFormat from unified_renderer.h
  RHIUsageFlags usage;
  const void *initial_data; // Optional
} RHITextureDesc;

// These are opaque handles - implementation in rhi_types_private.h
