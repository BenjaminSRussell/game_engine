#pragma once

#include "rhi_types_private.h"

// Backend function pointer table
typedef struct {
  RHIDevice *(*create_device)(const RenderConfig *config);
  void (*destroy_device)(RHIDevice *device);
  RenderSwapchain *(*create_swapchain)(RHIDevice *device,
                                       const RenderConfig *config);
  void (*destroy_swapchain)(RenderSwapchain *swapchain);
  void (*begin_frame)(RenderContext *ctx);
  void (*end_frame)(RenderContext *ctx);
  void (*present)(RenderContext *ctx);
} RHIBackendAPI;

// Backend API getters
RHIBackendAPI *rhi_metal_get_api(void);
RHIBackendAPI *rhi_vulkan_get_api(void);
RHIBackendAPI *rhi_d3d12_get_api(void);
RHIBackendAPI *rhi_opengl_get_api(void);
