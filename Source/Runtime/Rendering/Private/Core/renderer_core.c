#include "RHI/rhi_backend_api.h"
#include "RHI/rhi_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// Get backend API based on selected backend
static RHIBackendAPI *get_backend_api(RenderBackend backend);

// ============================================================================
// RENDERER CORE - Initialization & Lifecycle
// ============================================================================

RenderContext *render_init(const RenderConfig *config) {
  if (!config) {
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid render config");
    return NULL;
  }

  RenderContext *ctx = (RenderContext *)UNIFIED_ALLOC(sizeof(RenderContext));
  if (!ctx) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate render context");
    return NULL;
  }
  memset(ctx, 0, sizeof(RenderContext));

  // Copy config
  ctx->config = *config;
  ctx->frame_index = 0;

  // Auto-select backend if needed
  if (ctx->config.backend == RENDER_BACKEND_AUTO) {
#ifdef __APPLE__
    ctx->config.backend = RENDER_BACKEND_METAL;
#elif defined(_WIN32)
    ctx->config.backend = RENDER_BACKEND_D3D12;
#else
    ctx->config.backend = RENDER_BACKEND_VULKAN;
#endif
  }

  LOG_INFO(LOG_CAT_RENDERER, "Initializing renderer with backend: %d",
           ctx->config.backend);

  // Get backend API
  RHIBackendAPI *api = get_backend_api(ctx->config.backend);
  if (!api || !api->create_device) {
    LOG_ERROR(LOG_CAT_RENDERER, "Backend not implemented: %d",
              ctx->config.backend);
    UNIFIED_FREE(ctx);
    return NULL;
  }

  // Create device
  ctx->device = api->create_device(config);
  if (!ctx->device) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to create RHI device");
    UNIFIED_FREE(ctx);
    return NULL;
  }

  // Create swapchain
  if (config->window_handle) {
    ctx->swapchain = api->create_swapchain(ctx->device, config);
    if (!ctx->swapchain) {
      LOG_ERROR(LOG_CAT_RENDERER, "Failed to create swapchain");
      api->destroy_device(ctx->device);
      UNIFIED_FREE(ctx);
      return NULL;
    }
  }

  ctx->is_initialized = true;
  LOG_INFO(LOG_CAT_RENDERER, "Renderer initialized successfully");

  return ctx;
}

void render_shutdown(RenderContext *ctx) {
  if (!ctx)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Shutting down renderer");

  RHIBackendAPI *api = get_backend_api(ctx->config.backend);
  if (api) {
    if (ctx->swapchain) {
      api->destroy_swapchain(ctx->swapchain);
    }
    if (ctx->device) {
      api->destroy_device(ctx->device);
    }
  }

  UNIFIED_FREE(ctx);
}

// ============================================================================
// FRAME LIFECYCLE
// ============================================================================

void render_begin_frame(RenderContext *ctx) {
  if (!ctx || !ctx->is_initialized)
    return;

  RHIBackendAPI *api = get_backend_api(ctx->config.backend);
  if (api && api->begin_frame) {
    api->begin_frame(ctx);
  }
}

void render_end_frame(RenderContext *ctx) {
  if (!ctx || !ctx->is_initialized)
    return;

  RHIBackendAPI *api = get_backend_api(ctx->config.backend);
  if (api && api->end_frame) {
    api->end_frame(ctx);
  }

  ctx->frame_index++;
  ctx->stats.frame_count = ctx->frame_index;
}

void render_present(RenderContext *ctx) {
  if (!ctx || !ctx->is_initialized || !ctx->swapchain)
    return;

  RHIBackendAPI *api = get_backend_api(ctx->config.backend);
  if (api && api->present) {
    api->present(ctx);
  }
}

// ============================================================================
// STATE QUERIES
// ============================================================================

RenderDevice *render_get_device(RenderContext *ctx) {
  return ctx ? (RenderDevice *)ctx->device : NULL;
}

RenderSwapchain *render_get_swapchain(RenderContext *ctx) {
  return ctx ? (RenderSwapchain *)ctx->swapchain : NULL;
}

void render_get_stats(RenderContext *ctx, RenderStats *out_stats) {
  if (!ctx || !out_stats)
    return;
  *out_stats = ctx->stats;
}

bool render_is_initialized(RenderContext *ctx) {
  return ctx && ctx->is_initialized;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void render_resize(RenderContext *ctx, u32 width, u32 height) {
  if (!ctx || !ctx->swapchain)
    return;

  ctx->config.width = width;
  ctx->config.height = height;

  // TODO: Recreate swapchain
  LOG_WARN(LOG_CAT_RENDERER, "Resize not fully implemented yet");
}

void render_set_vsync(RenderContext *ctx, bool enabled) {
  if (!ctx)
    return;
  ctx->config.enable_vsync = enabled;
}

// ============================================================================
// BACKEND API DISPATCH
// ============================================================================

static RHIBackendAPI *get_backend_api(RenderBackend backend) {
  switch (backend) {
  case RENDER_BACKEND_METAL:
    return rhi_metal_get_api();
  case RENDER_BACKEND_VULKAN:
    return rhi_vulkan_get_api();
  case RENDER_BACKEND_D3D12:
    return rhi_d3d12_get_api();
  case RENDER_BACKEND_OPENGL:
    return rhi_opengl_get_api();
  default:
    return NULL;
  }
}
