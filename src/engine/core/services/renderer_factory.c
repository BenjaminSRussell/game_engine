#include "../../../include/core/logger.h"
#include "../../include/core/logger.h"
#include "../../include/rendering/renderer.h"
#include <stdlib.h>

// Forward declarations of concrete renderer factories
IRenderer *voxel_renderer_service_create(void);
// IRenderer *forward_renderer_create(void);
// IRenderer *deferred_renderer_create(void);

IRenderer *renderer_create_with_backend(RendererType type, GPUBackend backend) {
  IRenderer *renderer = NULL;

  switch (type) {
  case RENDERER_TYPE_VOXEL:
    renderer = voxel_renderer_service_create();
    break;
  case RENDERER_TYPE_FORWARD:
    // renderer = forward_renderer_create();
    LOG_WARN("Forward renderer not implemented, falling back to Voxel");
    renderer = voxel_renderer_service_create();
    break;
  case RENDERER_TYPE_DEFERRED:
    // renderer = deferred_renderer_create();
    LOG_WARN("Deferred renderer not implemented, falling back to Voxel");
    renderer = voxel_renderer_service_create();
    break;
  default:
    LOG_ERROR("Unknown renderer type: %d", type);
    return NULL;
  }

  if (renderer) {
    LOG_INFO("Renderer created (Type: %d, Backend: %d)", type, backend);
  } else {
    LOG_ERROR("Failed to create renderer");
  }

  return renderer;
}
