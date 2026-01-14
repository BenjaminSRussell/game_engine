#include <core/logger.h>
#include <rendering/renderer.h>
#include <rendering/voxel_renderer.h>
#include <stdlib.h>
#include <string.h>

#ifdef METAL_BUILD
#include "backend/metal/mtl_device.h"

typedef struct {
  VoxelRenderer *renderer;
  bool initialized;
} MetalVoxelRendererData;

static bool metal_voxel_renderer_init(IRenderer *self,
                                      RendererInitParams *params) {
  if (!self || !params)
    return false;

  MetalVoxelRendererData *data = (MetalVoxelRendererData *)self->impl_data;

  // Create Metal device (or use the one from params if possible, but here we
  // create one)
  metal_device_t *device = metal_device_create_system_default();
  if (!device) {
    LOG_ERROR("Failed to create Metal device for Voxel Renderer");
    return false;
  }

  data->renderer = voxel_renderer_create(device);
  if (!data->renderer) {
    LOG_ERROR("Failed to create Metal Voxel Renderer");
    metal_device_destroy(device);
    return false;
  }

  data->initialized = true;
  return true;
}

static void metal_voxel_renderer_cleanup(IRenderer *self) {
  if (!self)
    return;
  MetalVoxelRendererData *data = (MetalVoxelRendererData *)self->impl_data;
  if (data) {
    if (data->renderer) {
      voxel_renderer_destroy(data->renderer);
    }
    free(data);
  }
  free(self);
}

static void metal_voxel_renderer_resize(IRenderer *self, u32 width,
                                        u32 height) {
  // Metal resize typically handled by layer, but could update viewport here if
  // needed
  (void)self;
  (void)width;
  (void)height;
}

static bool metal_voxel_renderer_begin_frame(IRenderer *self,
                                             u32 *image_index) {
  MetalVoxelRendererData *data = (MetalVoxelRendererData *)self->impl_data;
  if (!data || !data->initialized)
    return false;

  // Remove recursive call - just stub implementation
  if (image_index)
    *image_index = 0; // Metal uses different indexing
  return true;
}

static void metal_voxel_renderer_end_frame(IRenderer *self, u32 image_index) {
  MetalVoxelRendererData *data = (MetalVoxelRendererData *)self->impl_data;
  if (data && data->initialized) {
    // Remove recursive call - just stub implementation
    (void)image_index; // Suppress unused parameter warning
  }
}

static void metal_voxel_renderer_render_chunk_mesh(IRenderer *self,
                                                   struct Chunk *chunk,
                                                   Mat4 view, Mat4 proj) {
  MetalVoxelRendererData *data = (MetalVoxelRendererData *)self->impl_data;
  if (!data || !data->initialized)
    return;

  // Note: We need a command encoder here. In the current engine structure,
  // the encoder might be stored in the renderer state or passed via unified
  // core. For now, this is a bridge. voxel_renderer_draw_mesh(data->renderer,
  // ???, chunk->mesh, (float*)&mvp);
}

static const char *metal_voxel_renderer_get_backend_name(IRenderer *self) {
  return "Metal";
}

VoxelRenderer *voxel_renderer_create(metal_device_t *device) {
  (void)device; // Suppress unused parameter warning
  // This is a stub implementation for now
  return NULL;
}

#endif // METAL_BUILD
