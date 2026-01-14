/**
 * @file rhi_descriptors.h
 * @brief RHI resource descriptor structures
 */

#ifndef VOXELFORGE_RHI_DESCRIPTORS_H
#define VOXELFORGE_RHI_DESCRIPTORS_H

#include "RHI/Public/rhi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Device Descriptor
// ============================================================================

typedef struct RHI_DeviceDesc {
  RHI_Backend backend;
  b8 enable_validation;
  b8 enable_gpu_capture;
  const char *app_name;
  u32 app_version;
} RHI_DeviceDesc;

// ============================================================================
// Swapchain Descriptor
// ============================================================================

typedef struct RHI_SwapchainDesc {
  void *window_handle; // Platform window handle
  u32 width;
  u32 height;
  RHI_Format format;
  u32 buffer_count; // 2 or 3 for double/triple buffering
  b8 vsync;
} RHI_SwapchainDesc;

// ============================================================================
// Buffer Descriptor
// ============================================================================

typedef struct RHI_BufferDesc {
  usize size;
  u32 usage; // RHI_BufferUsage flags
  RHI_MemoryType memory_type;
  const char *debug_name;
} RHI_BufferDesc;

// ============================================================================
// Texture Descriptor
// ============================================================================

typedef enum RHI_TextureType {
  RHI_TEXTURE_1D = 0,
  RHI_TEXTURE_2D = 1,
  RHI_TEXTURE_3D = 2,
  RHI_TEXTURE_CUBE = 3,
  RHI_TEXTURE_2D_ARRAY = 4,
  RHI_TEXTURE_CUBE_ARRAY = 5,
} RHI_TextureType;

typedef struct RHI_TextureDesc {
  RHI_TextureType type;
  RHI_Format format;
  u32 width;
  u32 height;
  u32 depth; // 1 for 2D textures
  u32 mip_levels;
  u32 array_layers;
  u32 sample_count;
  u32 usage; // RHI_TextureUsage flags
  const char *debug_name;
} RHI_TextureDesc;

// ============================================================================
// Texture View Descriptor
// ============================================================================

typedef struct RHI_TextureViewDesc {
  RHI_Texture *texture;
  RHI_TextureType view_type;
  RHI_Format format;
  u32 base_mip;
  u32 mip_count;
  u32 base_layer;
  u32 layer_count;
} RHI_TextureViewDesc;

// ============================================================================
// Sampler Descriptor
// ============================================================================

typedef struct RHI_SamplerDesc {
  RHI_FilterMode min_filter;
  RHI_FilterMode mag_filter;
  RHI_FilterMode mip_filter;
  RHI_AddressMode address_u;
  RHI_AddressMode address_v;
  RHI_AddressMode address_w;
  f32 mip_lod_bias;
  b8 anisotropy_enable;
  f32 max_anisotropy;
  b8 compare_enable;
  RHI_CompareOp compare_op;
  f32 min_lod;
  f32 max_lod;
} RHI_SamplerDesc;

// ============================================================================
// Shader Descriptor
// ============================================================================

typedef struct RHI_ShaderDesc {
  RHI_ShaderStage stage;
  const void *code; // SPIR-V, DXIL, or MSL bytecode
  usize code_size;
  const char *entry_point;
  const char *debug_name;
} RHI_ShaderDesc;

// ============================================================================
// Vertex Input Descriptor
// ============================================================================

typedef enum RHI_VertexFormat {
  RHI_VERTEX_FORMAT_FLOAT = 0,
  RHI_VERTEX_FORMAT_FLOAT2,
  RHI_VERTEX_FORMAT_FLOAT3,
  RHI_VERTEX_FORMAT_FLOAT4,
  RHI_VERTEX_FORMAT_INT,
  RHI_VERTEX_FORMAT_INT2,
  RHI_VERTEX_FORMAT_INT3,
  RHI_VERTEX_FORMAT_INT4,
  RHI_VERTEX_FORMAT_UINT,
  RHI_VERTEX_FORMAT_UINT2,
  RHI_VERTEX_FORMAT_UINT3,
  RHI_VERTEX_FORMAT_UINT4,
  RHI_VERTEX_FORMAT_UNORM8X4,
} RHI_VertexFormat;

typedef enum RHI_VertexInputRate {
  RHI_VERTEX_INPUT_RATE_VERTEX = 0,
  RHI_VERTEX_INPUT_RATE_INSTANCE = 1,
} RHI_VertexInputRate;

typedef struct RHI_VertexAttribute {
  u32 location;
  u32 binding;
  RHI_VertexFormat format;
  u32 offset;
} RHI_VertexAttribute;

typedef struct RHI_VertexBinding {
  u32 binding;
  u32 stride;
  RHI_VertexInputRate input_rate;
} RHI_VertexBinding;

typedef struct RHI_VertexInputDesc {
  RHI_VertexAttribute *attributes;
  u32 attribute_count;
  RHI_VertexBinding *bindings;
  u32 binding_count;
} RHI_VertexInputDesc;

// ============================================================================
// Blend State Descriptor
// ============================================================================

typedef enum RHI_BlendOp {
  RHI_BLEND_OP_ADD = 0,
  RHI_BLEND_OP_SUBTRACT,
  RHI_BLEND_OP_REVERSE_SUBTRACT,
  RHI_BLEND_OP_MIN,
  RHI_BLEND_OP_MAX,
} RHI_BlendOp;

typedef struct RHI_BlendAttachment {
  b8 blend_enable;
  RHI_BlendFactor src_color;
  RHI_BlendFactor dst_color;
  RHI_BlendOp color_op;
  RHI_BlendFactor src_alpha;
  RHI_BlendFactor dst_alpha;
  RHI_BlendOp alpha_op;
  u8 write_mask; // RGBA bits
} RHI_BlendAttachment;

typedef struct RHI_BlendStateDesc {
  RHI_BlendAttachment *attachments;
  u32 attachment_count;
} RHI_BlendStateDesc;

// ============================================================================
// Depth Stencil Descriptor
// ============================================================================

typedef struct RHI_DepthStencilDesc {
  b8 depth_test_enable;
  b8 depth_write_enable;
  RHI_CompareOp depth_compare_op;
  b8 stencil_test_enable;
  // Stencil ops omitted for brevity
} RHI_DepthStencilDesc;

// ============================================================================
// Rasterizer Descriptor
// ============================================================================

typedef enum RHI_FillMode {
  RHI_FILL_SOLID = 0,
  RHI_FILL_WIREFRAME = 1,
} RHI_FillMode;

typedef enum RHI_FrontFace {
  RHI_FRONT_FACE_CCW = 0,
  RHI_FRONT_FACE_CW = 1,
} RHI_FrontFace;

typedef struct RHI_RasterizerDesc {
  RHI_FillMode fill_mode;
  RHI_CullMode cull_mode;
  RHI_FrontFace front_face;
  b8 depth_bias_enable;
  f32 depth_bias_constant;
  f32 depth_bias_slope;
  f32 depth_bias_clamp;
} RHI_RasterizerDesc;

// ============================================================================
// Render Pass Descriptor
// ============================================================================

typedef struct RHI_AttachmentDesc {
  RHI_Format format;
  u32 sample_count;
  RHI_LoadOp load_op;
  RHI_StoreOp store_op;
  RHI_LoadOp stencil_load_op;
  RHI_StoreOp stencil_store_op;
} RHI_AttachmentDesc;

typedef struct RHI_RenderPassDesc {
  RHI_AttachmentDesc *color_attachments;
  u32 color_attachment_count;
  RHI_AttachmentDesc *depth_attachment; // Optional
  const char *debug_name;
} RHI_RenderPassDesc;

// ============================================================================
// Pipeline Descriptor
// ============================================================================

typedef struct RHI_GraphicsPipelineDesc {
  RHI_Shader *vertex_shader;
  RHI_Shader *fragment_shader;
  RHI_VertexInputDesc vertex_input;
  RHI_PrimitiveTopology primitive_topology;
  RHI_RasterizerDesc rasterizer;
  RHI_DepthStencilDesc depth_stencil;
  RHI_BlendStateDesc blend_state;
  RHI_RenderPass *render_pass;
  RHI_PipelineLayout *layout;
  const char *debug_name;
} RHI_GraphicsPipelineDesc;

typedef struct RHI_ComputePipelineDesc {
  RHI_Shader *compute_shader;
  RHI_PipelineLayout *layout;
  const char *debug_name;
} RHI_ComputePipelineDesc;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_RHI_DESCRIPTORS_H
