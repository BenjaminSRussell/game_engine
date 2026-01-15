#pragma once

#include "../Public/unified_renderer.h"
#include "rhi_types.h"

// Internal RHI device structure
struct RHIDevice {
  RenderBackend backend;
  void *native_device; // Backend-specific device handle
  void *native_queue;  // Command queue/context

  // Capabilities
  struct {
    bool ray_tracing;
    bool mesh_shaders;
    bool variable_rate_shading;
    uint32_t max_texture_size;
    uint32_t max_compute_workgroup_size;
  } capabilities;

  // Memory tracking
  uint64_t total_memory;
  uint64_t used_memory;
};

// Internal swapchain structure
struct RenderSwapchain {
  RHIDevice *device;
  void *native_swapchain; // Backend-specific swapchain

  uint32_t width;
  uint32_t height;
  RenderFormat format;
  RenderPresentMode present_mode;

  uint32_t image_count;
  void **images; // Swapchain images (backend-specific)
  uint32_t current_image;
};

// Internal render context
struct RenderContext {
  RHIDevice *device;
  RenderSwapchain *swapchain;

  RenderConfig config;
  RenderStats stats;

  uint64_t frame_index;
  bool is_initialized;

  // Backend-specific data
  void *backend_data;
};

// Buffer implementation
struct RHIBuffer {
  RHIDevice *device;
  RHIBufferType type;
  uint32_t size;
  RHIUsageFlags usage;
  void *native_buffer; // Backend-specific
  void *mapped_memory; // For dynamic buffers
};

// Texture implementation
struct RHITexture {
  RHIDevice *device;
  RHITextureType type;
  uint32_t width, height, depth;
  uint32_t mip_levels;
  uint32_t format;
  RHIUsageFlags usage;
  void *native_texture; // Backend-specific
  void *native_view;    // Texture view
};

// Command list implementation
struct RHICommandList {
  RHIDevice *device;
  void *native_command_buffer;
  bool is_recording;
};

// Pipeline state
struct RHIPipeline {
  RHIDevice *device;
  void *native_pipeline;
  RHIShader *vertex_shader;
  RHIShader *fragment_shader;
};

// Shader
struct RHIShader {
  RHIDevice *device;
  RHIShaderStage stage;
  void *native_shader;
  uint32_t code_size;
  void *bytecode;
};
