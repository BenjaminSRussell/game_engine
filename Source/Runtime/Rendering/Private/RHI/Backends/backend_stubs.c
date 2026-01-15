#include "../rhi_backend_api.h"
#include "../rhi_types_private.h"
#include "unified_logger.h"

// Vulkan backend stub - returns NULL for now
RHIBackendAPI *rhi_vulkan_get_api(void) {
  LOG_WARN(LOG_CAT_RENDERER, "Vulkan backend not implemented yet");
  return NULL;
}

// D3D12 backend stub
RHIBackendAPI *rhi_d3d12_get_api(void) {
  LOG_WARN(LOG_CAT_RENDERER, "D3D12 backend not implemented yet");
  return NULL;
}

// OpenGL backend stub
RHIBackendAPI *rhi_opengl_get_api(void) {
  LOG_WARN(LOG_CAT_RENDERER, "OpenGL backend not implemented yet");
  return NULL;
}
