#include <core/logger.h>
#include <renderer/renderer.h>
#include <stdlib.h>

// TODO: Add renderer factory unit tests for all renderer types
// TODO: Add renderer lifecycle management unit tests
// TODO: Add renderer error handling and validation tests

// Forward declarations
IRenderer *voxel_renderer_create(void);
IRenderer *voxel_renderer_create(void);
IRenderer *sprite_3d_renderer_create(void);
#include "../metal/metal_backend.h"

IRenderer *voxel_renderer_create(void);
IRenderer *sprite_3d_renderer_create(void);
IRenderer *graph_renderer_create(void);

IRenderer *renderer_create(RendererType type) {
  // If backend is specified in params, we might want to pass it here, 
  // but renderer_create only takes type.
  // We need to change renderer_create signature OR handle it inside specific creates.
  // However, engine.c calls renderer_create BEFORE init.
  // So we default to Voxel for now, but if we want Metal, we might need a dedicated type or config.
  
  // HACK: For now, if we are on macOS and want Metal, let's assume Voxel maps to Metal if configured
  // But strictly, renderer_create doesn't know config yet.
  
  // Better approach: config is passed in init. But we create BEFORE init.
  // We should create a generic 'renderer_create_with_backend' or just expose metal_renderer_create?
  
  return voxel_renderer_create(); // Default
}

// New creation function that respects backend preference
IRenderer *renderer_create_with_backend(RendererType type, GPUBackend backend) {
    if (backend == GPU_BACKEND_METAL) {
        return metal_renderer_create();
    }
    // Fallback or Vulkan
    return voxel_renderer_create();
}

void renderer_destroy(IRenderer *renderer) {
  // TODO: Add renderer cleanup unit tests
  // TODO: Add null pointer handling tests
  // TODO: Add double-free protection tests
  if (renderer) {
    if (renderer->cleanup) {
      renderer->cleanup(renderer);
    }
    free(renderer);
  }
}
