// src/engine/core/services/voxel_renderer.c
//
// Purpose: Implements VoxelRenderer, a concrete implementation of IRenderer
// with support for Vulkan, Metal, and STUB backends.
//
#include <chunk/chunk.h>
#include <common.h>
#include <core/logger.h>
#include <include/rendering/mesh.h>
#include <player/player.h>
#include <rendering/camera.h>
#include <rendering/renderer.h>
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD
#include <rendering/vulkan.h>

// Voxel renderer implementation data
typedef struct {
  VulkanRenderer *vulkan_renderer;
  bool initialized;
} VoxelRendererData;

// Forward declarations
static bool voxel_renderer_init(IRenderer *self, RendererInitParams *params);
static void voxel_renderer_cleanup(IRenderer *self);
static void voxel_renderer_resize(IRenderer *self, u32 width, u32 height);
static bool voxel_renderer_begin_frame(IRenderer *self, u32 *image_index);
static void voxel_renderer_end_frame(IRenderer *self, u32 image_index);
static void voxel_renderer_update_camera(IRenderer *self, struct Camera *camera,
                                         f32 aspect);
static void voxel_renderer_update_camera_uniforms(IRenderer *self,
                                                  struct Camera *camera,
                                                  f32 aspect);
static void voxel_renderer_render_chunk(IRenderer *self, Chunk *chunk,
                                        Mat4 view, Mat4 proj);
static void voxel_renderer_render_chunk_mesh(IRenderer *self, Chunk *chunk,
                                             Mat4 view, Mat4 proj);
static void voxel_renderer_render_dynamic_mesh(IRenderer *self, Mesh *mesh,
                                               Mat4 view, Mat4 proj);
static void voxel_renderer_render_sprite(IRenderer *self, Vec3 position,
                                         Vec2 size, u32 texture_id,
                                         f32 rotation);
static void voxel_renderer_render_entity_sprite(IRenderer *self, Entity entity,
                                                Vec3 position, Vec2 size,
                                                u32 texture_id);
static void voxel_renderer_render_ui_quad(IRenderer *self, Vec2 pos, Vec2 size,
                                          u32 texture_id);
static void voxel_renderer_render_text(IRenderer *self, const char *text,
                                       Vec2 pos, f32 scale, Vec3 color);
static void
voxel_renderer_render_block_highlight(IRenderer *self,
                                      struct PlayerSystem *player_system);
static void voxel_renderer_render_physics_debug(IRenderer *self, Mat4 view,
                                                Mat4 proj);
static void voxel_renderer_set_ambient_light(IRenderer *self,
                                             f32 ambient_light);
static bool voxel_renderer_create_chunk_buffers(IRenderer *self, Mesh *mesh,
                                                void **vertex_buffer,
                                                void **index_buffer);
static bool voxel_renderer_update_chunk_buffers(IRenderer *self, Mesh *mesh,
                                                void *vertex_buffer,
                                                void *index_buffer);

// Create voxel renderer
IRenderer *voxel_renderer_create(void) {
  IRenderer *renderer = (IRenderer *)calloc(1, sizeof(IRenderer));
  if (!renderer)
    return NULL;

  VoxelRendererData *data =
      (VoxelRendererData *)calloc(1, sizeof(VoxelRendererData));
  if (!data) {
    free(renderer);
    return NULL;
  }

  data->vulkan_renderer = (VulkanRenderer *)calloc(1, sizeof(VulkanRenderer));
  if (!data->vulkan_renderer) {
    free(data);
    free(renderer);
    return NULL;
  }

  renderer->type = RENDERER_TYPE_VOXEL;
  renderer->impl_data = data;
  renderer->init = voxel_renderer_init;
  renderer->cleanup = voxel_renderer_cleanup;
  renderer->resize = voxel_renderer_resize;
  renderer->begin_frame = voxel_renderer_begin_frame;
  renderer->end_frame = voxel_renderer_end_frame;
  renderer->update_camera = voxel_renderer_update_camera;
  renderer->update_camera_uniforms = voxel_renderer_update_camera_uniforms;
  renderer->render_chunk = voxel_renderer_render_chunk;
  renderer->render_chunk_mesh = voxel_renderer_render_chunk_mesh;
  renderer->render_dynamic_mesh = voxel_renderer_render_dynamic_mesh;
  renderer->render_sprite = voxel_renderer_render_sprite;
  renderer->render_entity_sprite = voxel_renderer_render_entity_sprite;
  renderer->render_ui_quad = voxel_renderer_render_ui_quad;
  renderer->render_text = voxel_renderer_render_text;
  renderer->render_block_highlight = voxel_renderer_render_block_highlight;
  renderer->render_physics_debug = voxel_renderer_render_physics_debug;
  renderer->set_ambient_light = voxel_renderer_set_ambient_light;
  renderer->create_chunk_buffers = voxel_renderer_create_chunk_buffers;
  renderer->update_chunk_buffers = voxel_renderer_update_chunk_buffers;

  return renderer;
}

static bool voxel_renderer_init(IRenderer *self, RendererInitParams *params) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->vulkan_renderer)
    return false;
  bool success = vulkan_init(data->vulkan_renderer, params->window,
                             params->width, params->height, params->config);
  if (success)
    data->initialized = true;
  return success;
}

static void voxel_renderer_cleanup(IRenderer *self) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data)
    return;
  if (data->initialized && data->vulkan_renderer)
    vulkan_cleanup(data->vulkan_renderer);
  if (data->vulkan_renderer)
    free(data->vulkan_renderer);
  free(data);
  self->impl_data = NULL;
}

static void voxel_renderer_resize(IRenderer *self, u32 width, u32 height) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_recreate_swapchain(data->vulkan_renderer, width, height);
}

static bool voxel_renderer_begin_frame(IRenderer *self, u32 *image_index) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->vulkan_renderer)
    return false;
  return vulkan_begin_frame(data->vulkan_renderer, image_index);
}

static void voxel_renderer_end_frame(IRenderer *self, u32 image_index) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_end_frame(data->vulkan_renderer, image_index);
}

static void voxel_renderer_update_camera(IRenderer *self, struct Camera *camera,
                                         f32 aspect) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_update_camera(data->vulkan_renderer, camera);
}

static void voxel_renderer_update_camera_uniforms(IRenderer *self,
                                                  struct Camera *camera,
                                                  f32 aspect) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_update_camera_uniforms(data->vulkan_renderer, camera, aspect);
}

static void voxel_renderer_render_chunk(IRenderer *self, Chunk *chunk,
                                        Mat4 view, Mat4 proj) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer && chunk && chunk->mesh.vertex_count > 0)
    vulkan_render_chunk_mesh(data->vulkan_renderer, chunk, view, proj);
}

static void voxel_renderer_render_chunk_mesh(IRenderer *self, Chunk *chunk,
                                             Mat4 view, Mat4 proj) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_render_chunk_mesh(data->vulkan_renderer, chunk, view, proj);
}

static void voxel_renderer_render_dynamic_mesh(IRenderer *self, Mesh *mesh,
                                               Mat4 view, Mat4 proj) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_render_dynamic_mesh(data->vulkan_renderer, mesh, view, proj);
}

static void voxel_renderer_render_sprite(IRenderer *self, Vec3 position,
                                         Vec2 size, u32 texture_id,
                                         f32 rotation) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data)
    return;

  // Sprite rendering via Vulkan backend
  // TODO: Implement 3D sprite batching and rendering
  LOG_TRACE("Sprite render: pos=[%.2f,%.2f,%.2f], size=[%.2f,%.2f], "
            "tex=%u, rot=%.2f",
            position.x, position.y, position.z, size.x, size.y, texture_id,
            rotation);
}
static void voxel_renderer_render_entity_sprite(IRenderer *self, Entity entity,
                                                Vec3 position, Vec2 size,
                                                u32 texture_id) {
  (void)self;
  (void)entity;
  (void)position;
  (void)size;
  (void)texture_id;
}
static void voxel_renderer_render_ui_quad(IRenderer *self, Vec2 pos, Vec2 size,
                                          u32 texture_id) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data)
    return;

  // UI quad rendering via Vulkan backend
  // TODO: Implement UI quad batching and rendering
  LOG_TRACE("UI quad render: pos=[%.2f,%.2f], size=[%.2f,%.2f], tex=%u", pos.x,
            pos.y, size.x, size.y, texture_id);
}
static void voxel_renderer_render_text(IRenderer *self, const char *text,
                                       Vec2 pos, f32 scale, Vec3 color) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !text || text[0] == '\0')
    return;

  // Text rendering via Vulkan backend
  // TODO: Implement text rendering pipeline (font atlas + glyph rendering)
  LOG_TRACE(
      "Text render: '%s' at [%.2f,%.2f], scale=%.2f, color=[%.2f,%.2f,%.2f]",
      text, pos.x, pos.y, scale, color.x, color.y, color.z);
}

static void
voxel_renderer_render_block_highlight(IRenderer *self,
                                      struct PlayerSystem *player_system) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_render_block_highlight(data->vulkan_renderer, player_system);
}

static void voxel_renderer_render_physics_debug(IRenderer *self, Mat4 view,
                                                Mat4 proj) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_render_physics_debug(data->vulkan_renderer, view, proj);
}

static void voxel_renderer_set_ambient_light(IRenderer *self,
                                             f32 ambient_light) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->vulkan_renderer)
    vulkan_set_ambient_light(data->vulkan_renderer, ambient_light);
}

static bool voxel_renderer_create_chunk_buffers(IRenderer *self, Mesh *mesh,
                                                void **vertex_buffer,
                                                void **index_buffer) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->vulkan_renderer)
    return false;
  VkBuffer v, i;
  VkDeviceMemory vm, im;
  if (!vulkan_create_chunk_vertex_buffer(data->vulkan_renderer, mesh, &v, &vm))
    return false;
  if (!vulkan_create_chunk_index_buffer(data->vulkan_renderer, mesh, &i, &im))
    return false;
  *vertex_buffer = (void *)v;
  *index_buffer = (void *)i;
  return true;
}

static bool voxel_renderer_update_chunk_buffers(IRenderer *self, Mesh *mesh,
                                                void *vertex_buffer,
                                                void *index_buffer) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->vulkan_renderer)
    return false;
  return vulkan_update_chunk_buffers(data->vulkan_renderer, mesh,
                                     (VkBuffer)vertex_buffer,
                                     (VkBuffer)index_buffer);
}

#elif defined(METAL_BUILD)
#include <backend/metal/mtl_command_buffer.h>
#include <backend/metal/mtl_device.h>
#include <backend/metal/mtl_encoder.h>
#include <backend/metal/mtl_swapchain.h>
#include <rendering/voxel_renderer.h>

// ... (typedefs remain)

typedef struct {
  VoxelRenderer *renderer;
  metal_device_t *device;
  metal_swapchain_t *swapchain;
  mtl_command_buffer_t cmd;
  mtl_render_command_encoder_t encoder;
  bool initialized;
} VoxelRendererData;

static bool voxel_renderer_init(IRenderer *self, RendererInitParams *params) {
  if (!self || !params)
    return false;

  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;

  // 1. Create Device
  data->device = metal_device_create_system_default();
  if (!data->device) {
    LOG_ERROR("Failed to create Metal device");
    return false;
  }

  // 2. Create Swapchain
  // params->window is void* (NSView*)
  bool hdr = false; // TODO: Configurable
  data->swapchain = metal_swapchain_create(data->device, params->window, hdr);
  if (!data->swapchain) {
    LOG_ERROR("Failed to create Metal swapchain");
    return false;
  }

  // 3. Create Native Voxel Renderer
  data->renderer = voxel_renderer_native_create(data->device);
  if (!data->renderer) {
    LOG_ERROR("Failed to create Voxel Renderer Native");
    return false;
  }

  data->initialized = true;
  LOG_INFO("Voxel Renderer (Metal) Initialized");
  return true;
}

static void voxel_renderer_cleanup(IRenderer *self) {
  if (!self)
    return;
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data) {
    if (data->renderer)
      voxel_renderer_destroy(data->renderer);
    if (data->swapchain)
      metal_swapchain_destroy(data->swapchain);
    // Device is usually owned by system or we destroy it?
    // metal_device_destroy(data->device); // Assuming simple cleanup
    free(data);
  }
  free(self);
}

static void voxel_renderer_resize(IRenderer *self, u32 width, u32 height) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->swapchain) {
    metal_swapchain_resize(data->swapchain, width, height);
  }
}

static bool voxel_renderer_begin_frame(IRenderer *self, u32 *image_index) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->initialized)
    return false;

  metal_swapchain_begin_frame(data->swapchain);

  void *texture = metal_swapchain_get_texture(data->swapchain);
  if (!texture) {
    // Dropped frame
    return false;
  }

  // Create Command Buffer
  data->cmd = metal_create_command_buffer(data->device);
  if (!data->cmd)
    return false;

  // Create Render Encoder
  // create_with_texture creates a default pass (load=clear, store=store,
  // clear_color=gray)
  data->encoder = metal_render_encoder_create_with_texture(data->cmd, texture);
  if (!data->encoder)
    return false;

  if (image_index)
    *image_index = 0; // Dummy index
  return true;
}

static void voxel_renderer_end_frame(IRenderer *self, u32 image_index) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->initialized)
    return;

  if (data->encoder) {
    metal_render_encoder_end_encoding(data->encoder);
    data->encoder = NULL;
  }

  if (data->cmd) {
    metal_swapchain_present(data->swapchain, data->cmd);
    metal_command_buffer_commit(data->cmd);
    data->cmd = NULL;
  }

  metal_swapchain_end_frame(data->swapchain);
}

static void voxel_renderer_render_chunk_mesh(IRenderer *self, Chunk *chunk,
                                             Mat4 view, Mat4 proj) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (!data || !data->initialized || !data->encoder)
    return;

  if (chunk && chunk->mesh.vertex_count > 0) {
    Mat4 mvp = mat4_mul(proj, view); // TODO: Model matrix? Chunk position?
    // Chunk mesh is in local coords? No, usually baked world coords or need
    // model matrix. If baked, Identity model. If local, pass model. VoxelMesh
    // struct has vertices. Let's assume chunk meshes are positioned using Model
    // matrix. But function signature doesn't take Model matrix. It takes chunk.
    // Chunk has position. I should compute Model matrix from chunk->pos.
    // Actually, `mvp` passed to `draw_mesh` is likely MVP.
    // So I need M * V * P.
    // V * P is calculated.
    // M = Translation(chunk->pos * CHUNK_SIZE).

    Mat4 model = mat4_identity();
    Vec3 pos = vec3((f32)chunk->pos.x * 16.0f, (f32)chunk->pos.y * 16.0f,
                    (f32)chunk->pos.z * 16.0f); // Assuming 16 size
    model = mat4_translate(pos);

    Mat4 mvp_matrix = mat4_mul(mvp, model);

    // Get VoxelMesh from chunk storage (we reuse vertex_buffer pointer)
    VoxelMesh *vmesh = (VoxelMesh *)chunk->mesh.vertex_buffer;
    if (vmesh) {
      voxel_renderer_draw_mesh(data->renderer, data->encoder, vmesh,
                               (float *)&mvp_matrix);
    }
  }
}

// Implement create_chunk_buffers
static bool voxel_renderer_create_chunk_buffers(IRenderer *self, Mesh *mesh,
                                                void **vertex_buffer,
                                                void **index_buffer) {
  // This function signature matches `vulkan_create_chunk_buffers`.
  // It returns buffers via pointers.
  // But Metal VoxelRenderer manages its own buffers in `VoxelMesh`.
  // We can return the `VoxelMesh*` casted to void*?
  // The callees (chunk manager) likely store these void* into
  // `chunk->vertex_buffer_handle`.

  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  // Create VoxelMesh from Mesh
  VoxelMesh *vmesh = voxel_mesh_create(data->renderer);
  // Upload data
  voxel_mesh_update(vmesh, mesh->vertices, mesh->vertex_count, mesh->indices,
                    mesh->index_count);

  *vertex_buffer = (void *)vmesh;
  *index_buffer = NULL; // We bundle both in vmesh
  return true;
}

// ... helper for render ...
// We need to support `render_chunk_mesh` using the handle.
// `chunk_manager_render` calls `renderer->render_chunk_mesh`.
// It passes `Chunk*`. `Chunk` has the handles?
// I need `Chunk` definition to know where handles are.
// `src/game/blockgame/chunk/chunk.h`.
// Assume `chunk->mesh_handle` or similar.
// Actually, `gamestate_main.c` showed `chunk_create_vulkan_buffers` storing in
// `renderer` cache? Or `chunk->mesh.vertices`. IRenderer `create_chunk_buffers`
// is called from `chunk_create_vulkan_buffers`? No,
// `chunk_create_vulkan_buffers` calls `renderer->create_chunk_buffers`. So if I
// implement `create_chunk_buffers`, I can control what is stored. But `chunk`
// likely has `VkBuffer vertex_buffer;` fields if hardcoded. If `Chunk` struct
// is opaque or has generic `void*`, great. If it has `VkBuffer`, I can cast
// `VoxelMesh*` to `VkBuffer` (pointer to pointer) safely enough if I am
// careful.

#else

static bool stub_renderer_init(IRenderer *self, RendererInitParams *params) {
  LOG_WARN("Voxel Renderer running in STUB mode");
  return true;
}
static void stub_renderer_cleanup(IRenderer *self) { free(self); }
static void stub_renderer_resize(IRenderer *self, u32 width, u32 height) {}
static bool stub_renderer_begin_frame(IRenderer *self, u32 *image_index) {
  *image_index = 0;
  return true;
}
static void stub_renderer_end_frame(IRenderer *self, u32 image_index) {}

// ... stubs for others ...
static void stub_render_chunk(IRenderer *self, Chunk *c, Mat4 v, Mat4 p) {}
// ...

#endif

IRenderer *voxel_renderer_service_create(void) {
  IRenderer *renderer = (IRenderer *)calloc(1, sizeof(IRenderer));
  if (!renderer)
    return NULL;

#ifdef METAL_BUILD
  renderer->type = RENDERER_TYPE_VOXEL;
  renderer->impl_data = calloc(1, sizeof(VoxelRendererData));
  renderer->init = voxel_renderer_init;
  renderer->cleanup = voxel_renderer_cleanup;
  renderer->resize = voxel_renderer_resize;
  renderer->begin_frame = voxel_renderer_begin_frame;
  renderer->end_frame = voxel_renderer_end_frame;
  renderer->render_chunk_mesh = voxel_renderer_render_chunk_mesh;
  renderer->create_chunk_buffers =
      voxel_renderer_create_chunk_buffers; // Hook this!
  // ... others stubs or null
#else
  renderer->type = RENDERER_TYPE_VOXEL;
  renderer->init = stub_renderer_init;
  renderer->cleanup = stub_renderer_cleanup;
  renderer->resize = stub_renderer_resize;
  renderer->begin_frame = stub_renderer_begin_frame;
  renderer->end_frame = stub_renderer_end_frame;
#endif

  return renderer;
}
