#include <core/logger.h>
#include <rendering/renderer.h>
#include <stdlib.h>

// Forward declarations for renderer creators
extern IRenderer *voxel_renderer_create(void);
extern IRenderer *sprite_3d_renderer_create(void);

IRenderer *renderer_create(RendererType type) {
  switch (type) {
  case RENDERER_TYPE_VOXEL:
    return voxel_renderer_create();
  case RENDERER_TYPE_SPRITE_3D:
    // return sprite_3d_renderer_create();
    LOG_ERROR("Sprite 3D renderer not yet implemented");
    return NULL;
  default:
    LOG_ERROR("Unknown renderer type: %d", type);
    return NULL;
  }
}

IRenderer *renderer_create_with_backend(RendererType type, GPUBackend backend) {
  if (type == RENDERER_TYPE_VOXEL) {
    // The voxel_renderer_create function in services will handle backend
    // dispatch via #ifdefs or we can pass the backend if we refactor it. For
    // now, let it handle it.
    return voxel_renderer_create();
  }

  LOG_ERROR("Renderer type %d with backend %d not supported", type, backend);
  return NULL;
}

void renderer_destroy(IRenderer *renderer) {
  if (renderer && renderer->cleanup) {
    renderer->cleanup(renderer);
  }
}
