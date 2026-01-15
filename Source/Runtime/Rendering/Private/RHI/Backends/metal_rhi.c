#include "../rhi_backend_api.h"
#include "../rhi_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"

// Metal RHI Backend (macOS/iOS)
// This is a stub implementation - will be filled in next

typedef struct {
  void *mtl_device;
  void *mtl_command_queue;
} MetalBackendData;

static RHIDevice *metal_create_device(const RenderConfig *config) {
  LOG_INFO(LOG_CAT_RENDERER, "Creating Metal device (stub)");

  RHIDevice *device = (RHIDevice *)UNIFIED_ALLOC(sizeof(RHIDevice));
  if (!device)
    return NULL;

  device->backend = RENDER_BACKEND_METAL;
  device->native_device = NULL; // TODO: Create MTLDevice
  device->native_queue = NULL;  // TODO: Create command queue

  // Stub capabilities
  device->capabilities.ray_tracing = false;
  device->capabilities.mesh_shaders = false;
  device->capabilities.variable_rate_shading = false;
  device->capabilities.max_texture_size = 16384;

  return device;
}

static void metal_destroy_device(RHIDevice *device) {
  if (!device)
    return;
  LOG_INFO(LOG_CAT_RENDERER, "Destroying Metal device");

  // TODO: Release Metal objects

  UNIFIED_FREE(device);
}

static RenderSwapchain *metal_create_swapchain(RHIDevice *device,
                                               const RenderConfig *config) {
  if (!device)
    return NULL;

  LOG_INFO(LOG_CAT_RENDERER, "Creating Metal swapchain (stub)");

  RenderSwapchain *swapchain =
      (RenderSwapchain *)UNIFIED_ALLOC(sizeof(RenderSwapchain));
  if (!swapchain)
    return NULL;

  swapchain->device = device;
  swapchain->width = config->width;
  swapchain->height = config->height;
  swapchain->format = config->format;
  swapchain->present_mode = config->present_mode;
  swapchain->image_count = config->swapchain_image_count;
  swapchain->current_image = 0;

  // TODO: Create CAMetalLayer and drawable textures

  return swapchain;
}

static void metal_destroy_swapchain(RenderSwapchain *swapchain) {
  if (!swapchain)
    return;
  LOG_INFO(LOG_CAT_RENDERER, "Destroying Metal swapchain");

  // TODO: Release Metal swapchain resources

  UNIFIED_FREE(swapchain);
}

static void metal_begin_frame(RenderContext *ctx) {
  if (!ctx)
    return;
  // TODO: Acquire next drawable
}

static void metal_end_frame(RenderContext *ctx) {
  if (!ctx)
    return;
  // TODO: Submit command buffer
}

static void metal_present(RenderContext *ctx) {
  if (!ctx)
    return;
  // TODO: Present drawable
}

// Export Metal backend API
static RHIBackendAPI metal_api = {.create_device = metal_create_device,
                                  .destroy_device = metal_destroy_device,
                                  .create_swapchain = metal_create_swapchain,
                                  .destroy_swapchain = metal_destroy_swapchain,
                                  .begin_frame = metal_begin_frame,
                                  .end_frame = metal_end_frame,
                                  .present = metal_present};

RHIBackendAPI *rhi_metal_get_api(void) { return &metal_api; }
