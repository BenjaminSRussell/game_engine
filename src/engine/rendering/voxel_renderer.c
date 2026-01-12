// src/engine/rendering/voxel_renderer.c
// Voxel Renderer - Hardware-accelerated voxel rendering system

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#include "backend/metal/mtl_pipeline.h"
#include "backend/metal/mtl_shader_library.h"
#include <rendering/camera.h>
#include <rendering/renderer.h>

// ============================================================================
// Voxel Renderer Types
// ============================================================================

typedef struct {
  float x, y, z;     // Position
  float nx, ny, nz;  // Normal
  float u, v;        // Texture coordinates
  uint8_t ao;        // Ambient occlusion
  uint8_t light;     // Light level
  uint16_t block_id; // Block type
} VoxelVertex;

typedef struct {
  metal_buffer_t *vertex_buffer;
  metal_buffer_t *index_buffer;
  uint32_t vertex_count;
  uint32_t index_count;
  bool uploaded;
} VoxelMesh;

typedef struct {
  uint32_t draw_calls;
  uint32_t triangles_drawn;
  uint32_t chunks_rendered;
  float frame_time_ms;
} VoxelRenderStats;

typedef struct VoxelRenderer {
  metal_device_t *device;
  metal_render_pipeline_t *pipeline;
  metal_depth_stencil_state_t *depth_state;
  metal_shader_library_t *library;
  void *texture_atlas; // Block texture atlas

  VoxelMesh *mesh_pool;
  uint32_t mesh_pool_size;
  uint32_t mesh_pool_capacity;

  VoxelRenderStats stats;

  // Configuration
  bool enable_ao;
  bool enable_shadows;
  bool enable_reflections;
  float render_distance;
  int max_chunks_per_frame;
} VoxelRenderer;

// ============================================================================
// Voxel Renderer API
// ============================================================================

// Internal helper to initialize pipeline
static bool voxel_renderer_init_pipeline(VoxelRenderer *renderer) {
  if (!renderer || !renderer->device)
    return false;

  // Load shader library using the shader library manager
  // This handles both .metal source files and compiled .metallib files
  metal_shader_library_manager_t *shader_manager =
      metal_shader_library_manager_create(metal_get_device(renderer->device));
  if (!shader_manager) {
    LOG_ERROR("Failed to create shader library manager");
    return false;
  }

  MTLLibraryRef library_ref = metal_library_manager_load_file(
      shader_manager, "src/engine/rendering/shaders/voxel.metal");
  if (!library_ref) {
    LOG_ERROR("Failed to load voxel shader library");
    metal_shader_library_manager_destroy(shader_manager);
    return false;
  }

  // Create wrapper structure for consistency
  renderer->library =
      (metal_shader_library_t *)calloc(1, sizeof(metal_shader_library_t));
  if (!renderer->library) {
    LOG_ERROR("Failed to allocate shader library structure");
    metal_shader_library_manager_destroy(shader_manager);
    return false;
  }
  renderer->library->library = library_ref;
  strncpy(renderer->library->name, "voxel.metal",
          sizeof(renderer->library->name) - 1);

  metal_shader_library_manager_destroy(shader_manager);

  // Create vertex descriptor
  metal_vertex_descriptor_t v_desc;
  metal_vertex_descriptor_init(&v_desc);

  // Position (Float3)
  metal_vertex_descriptor_add_attribute(&v_desc, 0, METAL_VERTEX_FORMAT_FLOAT3,
                                        0, 0);
  // Normal (Float3)
  metal_vertex_descriptor_add_attribute(&v_desc, 1, METAL_VERTEX_FORMAT_FLOAT3,
                                        12, 0);
  // UV (Float2)
  metal_vertex_descriptor_add_attribute(&v_desc, 2, METAL_VERTEX_FORMAT_FLOAT2,
                                        24, 0);
  // AO (UChar - Normalized) -> Float
  metal_vertex_descriptor_add_attribute(
      &v_desc, 3, METAL_VERTEX_FORMAT_UCHAR4_NORMALIZED, 32, 0);
  // Light (UChar - Normalized) -> Float (using UChar4 for alignment/padding
  // simplified)
  metal_vertex_descriptor_add_attribute(
      &v_desc, 4, METAL_VERTEX_FORMAT_UCHAR4_NORMALIZED, 36, 0);
  // BlockID (UShort) -> UInt (Simplification: passing as Float/UInt via
  // padding) Note: struct alignment in C vs Metal packed can be tricky. Our C
  // struct: 3*4 + 3*4 + 2*4 + 1 + 1 + 2 = 12+12+8+4 = 36 bytes roughly? Let's
  // re-verify struct layout. float x,y,z (12) float nx,ny,nz (12) float u,v (8)
  // uint8 ao (1)
  // uint8 light (1)
  // uint16 block_id (2)
  // Total: 36 bytes.
  // Metal alignment usually 4 bytes.

  // Layout
  metal_vertex_descriptor_set_layout(&v_desc, 0, sizeof(VoxelVertex), 0);

  // Depth Stencil State
  metal_depth_stencil_desc_t ds_desc = {0};
  ds_desc.depth_compare = METAL_COMPARE_LESS;
  ds_desc.depth_write_enabled = true;
  renderer->depth_state = metal_create_depth_stencil_state(
      metal_get_device(renderer->device), &ds_desc);

  // Pipeline State
  metal_render_pipeline_desc_t p_desc = {0};
  p_desc.vertex_function =
      metal_get_function(renderer->library, "voxel_vertex_main");
  p_desc.fragment_function =
      metal_get_function(renderer->library, "voxel_fragment_main");
  p_desc.color_format =
      METAL_PIXEL_FORMAT_BGRA8_UNORM; // Assuming swapchain format
  p_desc.depth_format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT;
  p_desc.vertex_descriptor = v_desc;

  // Defaults for blend, etc.

  renderer->pipeline =
      metal_create_render_pipeline(metal_get_device(renderer->device), &p_desc);

  return (renderer->pipeline != NULL);
}

VoxelRenderer *voxel_renderer_create(metal_device_t *device) {
  if (!device) {
    device = metal_device_create_system_default();
  }

  VoxelRenderer *renderer = calloc(1, sizeof(VoxelRenderer));
  if (!renderer) {
    LOG_ERROR("Failed to allocate voxel renderer");
    return NULL;
  }

  renderer->device = device;
  renderer->mesh_pool_capacity = 1024;
  renderer->mesh_pool = calloc(renderer->mesh_pool_capacity, sizeof(VoxelMesh));

  // Initialize pipeline
  if (!voxel_renderer_init_pipeline(renderer)) {
    LOG_ERROR("Failed to initialize voxel pipeline");
    // Continue anyway for now to allow partial usage
  }

  // Default configuration
  renderer->enable_ao = true;
  renderer->enable_shadows = false;
  renderer->enable_reflections = false;
  renderer->render_distance = 256.0f;
  renderer->max_chunks_per_frame = 32;

  LOG_INFO("Voxel renderer created with %u mesh capacity",
           renderer->mesh_pool_capacity);
  return renderer;
}

void voxel_renderer_destroy(VoxelRenderer *renderer) {
  if (!renderer)
    return;

  if (renderer->depth_state) {
    metal_destroy_depth_stencil_state(renderer->depth_state);
  }
  if (renderer->pipeline) {
    metal_destroy_render_pipeline(renderer->pipeline);
  }
  if (renderer->library) {
    metal_destroy_shader_library(renderer->library);
  }

  // Free mesh pool
  if (renderer->mesh_pool) {
    for (uint32_t i = 0; i < renderer->mesh_pool_size; i++) {
      if (renderer->mesh_pool[i].vertex_buffer) {
        metal_buffer_destroy(renderer->mesh_pool[i].vertex_buffer);
      }
      if (renderer->mesh_pool[i].index_buffer) {
        metal_buffer_destroy(renderer->mesh_pool[i].index_buffer);
      }
    }
    free(renderer->mesh_pool);
  }

  free(renderer);
  LOG_INFO("Voxel renderer destroyed");
}

// ============================================================================
// Mesh Generation
// ============================================================================

static void add_face_vertices(VoxelVertex *vertices, uint32_t *indices,
                              uint32_t *v_count, uint32_t *i_count, float x,
                              float y, float z, int face, uint16_t block_id,
                              uint8_t light) {
  // Face normals
  static const float normals[6][3] = {
      {1, 0, 0}, {-1, 0, 0}, // +X, -X
      {0, 1, 0}, {0, -1, 0}, // +Y, -Y
      {0, 0, 1}, {0, 0, -1}  // +Z, -Z
  };

  // Face vertex offsets
  static const float offsets[6][4][3] = {
      // +X
      {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}},
      // -X
      {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
      // +Y
      {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
      // -Y
      {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
      // +Z
      {{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}},
      // -Z
      {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}};

  // UV coordinates
  static const float uvs[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};

  uint32_t base_v = *v_count;

  // Add 4 vertices for the face
  for (int i = 0; i < 4; i++) {
    VoxelVertex *v = &vertices[*v_count];
    v->x = x + offsets[face][i][0];
    v->y = y + offsets[face][i][1];
    v->z = z + offsets[face][i][2];
    v->nx = normals[face][0];
    v->ny = normals[face][1];
    v->nz = normals[face][2];
    v->u = uvs[i][0];
    v->v = uvs[i][1];
    v->ao = 255;
    v->light = light;
    v->block_id = block_id;
    (*v_count)++;
  }

  // Add 6 indices for 2 triangles
  indices[(*i_count)++] = base_v + 0;
  indices[(*i_count)++] = base_v + 1;
  indices[(*i_count)++] = base_v + 2;
  indices[(*i_count)++] = base_v + 0;
  indices[(*i_count)++] = base_v + 2;
  indices[(*i_count)++] = base_v + 3;
}

VoxelMesh *voxel_mesh_generate(VoxelRenderer *renderer, const uint8_t *blocks,
                               int size_x, int size_y, int size_z) {
  if (!renderer || !blocks)
    return NULL;
  if (renderer->mesh_pool_size >= renderer->mesh_pool_capacity) {
    LOG_WARN("Mesh pool full");
    return NULL;
  }

  // Allocate temporary buffers
  uint32_t max_vertices = size_x * size_y * size_z * 24; // 6 faces * 4 verts
  uint32_t max_indices = size_x * size_y * size_z * 36;  // 6 faces * 6 indices

  VoxelVertex *vertices = malloc(max_vertices * sizeof(VoxelVertex));
  uint32_t *indices = malloc(max_indices * sizeof(uint32_t));

  if (!vertices || !indices) {
    free(vertices);
    free(indices);
    return NULL;
  }

  uint32_t v_count = 0;
  uint32_t i_count = 0;

  // Greedy meshing - check each block
  for (int y = 0; y < size_y; y++) {
    for (int z = 0; z < size_z; z++) {
      for (int x = 0; x < size_x; x++) {
        int idx = y * size_x * size_z + z * size_x + x;
        uint8_t block = blocks[idx];

        if (block == 0)
          continue; // Air

        // Check each face
        // +X face
        if (x == size_x - 1 || blocks[idx + 1] == 0) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 0,
                            block, 255);
        }
        // -X face
        if (x == 0 || blocks[idx - 1] == 0) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 1,
                            block, 255);
        }
        // +Y face
        if (y == size_y - 1 ||
            (y < size_y - 1 && blocks[idx + size_x * size_z] == 0)) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 2,
                            block, 255);
        }
        // -Y face
        if (y == 0 || (y > 0 && blocks[idx - size_x * size_z] == 0)) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 3,
                            block, 255);
        }
        // +Z face
        if (z == size_z - 1 || (z < size_z - 1 && blocks[idx + size_x] == 0)) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 4,
                            block, 255);
        }
        // -Z face
        if (z == 0 || (z > 0 && blocks[idx - size_x] == 0)) {
          add_face_vertices(vertices, indices, &v_count, &i_count, x, y, z, 5,
                            block, 255);
        }
      }
    }
  }

  // Create Metal buffers
  VoxelMesh *mesh = &renderer->mesh_pool[renderer->mesh_pool_size++];

  metal_buffer_desc_t v_desc = {.size = v_count * sizeof(VoxelVertex),
                                .storage_mode = METAL_STORAGE_SHARED,
                                .usage = METAL_BUFFER_USAGE_VERTEX,
                                .initial_data = vertices,
                                .label = "Voxel Vertex Buffer"};
  mesh->vertex_buffer = metal_buffer_create(renderer->device, &v_desc);

  metal_buffer_desc_t i_desc = {.size = i_count * sizeof(uint32_t),
                                .storage_mode = METAL_STORAGE_SHARED,
                                .usage = METAL_BUFFER_USAGE_INDEX,
                                .initial_data = indices,
                                .label = "Voxel Index Buffer"};
  mesh->index_buffer = metal_buffer_create(renderer->device, &i_desc);

  mesh->vertex_count = v_count;
  mesh->index_count = i_count;
  mesh->uploaded = true;

  free(vertices);
  free(indices);

  return mesh;
}

void voxel_mesh_free(VoxelRenderer *renderer, VoxelMesh *mesh) {
  if (!mesh)
    return;

  if (mesh->vertex_buffer) {
    metal_buffer_destroy(mesh->vertex_buffer);
    mesh->vertex_buffer = NULL;
  }
  if (mesh->index_buffer) {
    metal_buffer_destroy(mesh->index_buffer);
    mesh->index_buffer = NULL;
  }
  mesh->vertex_count = 0;
  mesh->index_count = 0;
  mesh->uploaded = false;
}

// ============================================================================
// Rendering
// ============================================================================

void voxel_renderer_begin_frame(VoxelRenderer *renderer) {
  if (!renderer)
    return;

  // Reset stats
  renderer->stats.draw_calls = 0;
  renderer->stats.triangles_drawn = 0;
  renderer->stats.chunks_rendered = 0;
}

void voxel_renderer_draw_mesh(VoxelRenderer *renderer, VoxelMesh *mesh,
                              mtl_render_command_encoder_t encoder,
                              float *mvp_matrix) {
  if (!renderer || !mesh || !mesh->uploaded || !encoder)
    return;
  if (mesh->vertex_count == 0)
    return;

  // Set pipeline state
  if (renderer->pipeline && renderer->pipeline->state) {
    metal_render_encoder_set_render_pipeline_state(encoder,
                                                   renderer->pipeline->state);
  }

  // Set depth stencil
  if (renderer->depth_state && renderer->depth_state->state) {
    metal_render_encoder_set_depth_stencil_state(encoder,
                                                 renderer->depth_state->state);
  }

  // Set winding and cull mode
  metal_render_encoder_set_front_facing_winding(
      encoder, METAL_WINDING_COUNTER_CLOCKWISE);
  metal_render_encoder_set_cull_mode(encoder, METAL_CULL_MODE_BACK);

  // Set buffers
  if (mesh->vertex_buffer) {
    metal_render_encoder_set_vertex_buffer(encoder, mesh->vertex_buffer->buffer,
                                           0, 0);
  }

  // Set uniforms (MVP)
  if (mvp_matrix) {
    metal_render_encoder_set_vertex_bytes(encoder, mvp_matrix,
                                          16 * sizeof(float), 1);
  }

  // Draw indexed primitives
  mtl_draw_indexed_primitives_args_t args = {
      .primitiveType = MTL_PRIMITIVE_TYPE_TRIANGLE,
      .indexCount = mesh->index_count,
      .indexType = MTL_INDEX_TYPE_UINT32,
      .indexBuffer = mesh->index_buffer->buffer,
      .indexBufferOffset = 0,
      .instanceCount = 1,
      .baseVertex = 0,
      .baseInstance = 0};

  metal_render_encoder_draw_indexed_primitives(encoder, args);

  // Statistics
  renderer->stats.draw_calls++;
  renderer->stats.triangles_drawn += mesh->index_count / 3;
  renderer->stats.chunks_rendered++;
}

void voxel_renderer_end_frame(VoxelRenderer *renderer) {
  if (!renderer)
    return;

  // Present frame - handled by Metal/Vulkan layer
}

VoxelRenderStats voxel_renderer_get_stats(VoxelRenderer *renderer) {
  if (!renderer) {
    return (VoxelRenderStats){0};
  }
  return renderer->stats;
}

// ============================================================================
// Configuration
// ============================================================================

void voxel_renderer_set_render_distance(VoxelRenderer *renderer,
                                        float distance) {
  if (!renderer)
    return;
  renderer->render_distance = distance;
}

void voxel_renderer_enable_ao(VoxelRenderer *renderer, bool enable) {
  if (!renderer)
    return;
  renderer->enable_ao = enable;
}

void voxel_renderer_enable_shadows(VoxelRenderer *renderer, bool enable) {
  if (!renderer)
    return;
  renderer->enable_shadows = enable;
}

void voxel_renderer_enable_reflections(VoxelRenderer *renderer, bool enable) {
  if (!renderer)
    return;
  renderer->enable_reflections = enable;
}

// ============================================================================
// IRenderer Interface Implementation
// ============================================================================

static bool voxel_renderer_init_impl(IRenderer *self,
                                     RendererInitParams *params) {
  if (!self || !params)
    return false;

  // Create new VoxelRenderer if one doesn't exist or use impl_data
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;
  if (!vr) {
    // Assume device is passed or create default
    vr = voxel_renderer_create(NULL);
    if (!vr)
      return false;
    self->impl_data = vr;
  }

  // Configure based on params
  voxel_renderer_set_render_distance(
      vr, params->config ? params->config->render_distance : 256.0f);

  return true;
}

static void voxel_renderer_cleanup_impl(IRenderer *self) {
  if (!self)
    return;
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;
  if (vr) {
    voxel_renderer_destroy(vr);
    self->impl_data = NULL;
  }
}

static void voxel_renderer_resize_impl(IRenderer *self, u32 width, u32 height) {
  // Metal handles resize via layer, but we might need to update depth texture
  // etc. For now, no-op or handled internally
  (void)self;
  (void)width;
  (void)height;
}

static bool voxel_renderer_begin_frame_impl(IRenderer *self, u32 *image_index) {
  if (!self)
    return false;
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;
  voxel_renderer_begin_frame(vr);
  *image_index = 0; // Dummy index for Metal
  return true;
}

static void voxel_renderer_end_frame_impl(IRenderer *self, u32 image_index) {
  if (!self)
    return;
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;
  voxel_renderer_end_frame(vr);
}

static void voxel_renderer_render_chunk_mesh_impl(IRenderer *self, Chunk *chunk,
                                                  Mat4 view, Mat4 proj) {
  if (!self || !chunk)
    return;
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;

  // We need a way to get the mesh from the chunk and the command encoder
  // This part requires integration with the upper layer providing the encoder
  // For IRenderer abstraction, we might need to store the current encoder in
  // VoxelRenderer during begin_frame or pass it differently. For now, assume
  // VoxelRenderer manages its own encoder internally if possible, or we are
  // just verifying compilation.

  // TODO: Metal encoding integration
}

static const char *voxel_renderer_get_backend_name_impl(IRenderer *self) {
  return "Metal";
}

// Stubs for other interface methods
static void voxel_renderer_update_camera_impl(IRenderer *self, Camera *camera,
                                              f32 aspect) {
  (void)self;
  (void)camera;
  (void)aspect;
}
static void voxel_renderer_update_camera_uniforms_impl(IRenderer *self,
                                                       Camera *camera,
                                                       f32 aspect) {
  (void)self;
  (void)camera;
  (void)aspect;
}
static void voxel_renderer_render_chunk_impl(IRenderer *self, Chunk *chunk,
                                             Mat4 view, Mat4 proj) {
  (void)self;
  (void)chunk;
  (void)view;
  (void)proj;
}
static void voxel_renderer_render_dynamic_mesh_impl(IRenderer *self, Mesh *mesh,
                                                    Mat4 view, Mat4 proj) {
  (void)self;
  (void)mesh;
  (void)view;
  (void)proj;
}
static void voxel_renderer_render_sprite_impl(IRenderer *self, Vec3 position,
                                              Vec2 size, u32 texture_id,
                                              f32 rotation) {
  (void)self;
  (void)position;
  (void)size;
  (void)texture_id;
  (void)rotation;
}
static void voxel_renderer_render_entity_sprite_impl(IRenderer *self,
                                                     Entity entity,
                                                     Vec3 position, Vec2 size,
                                                     u32 texture_id) {
  (void)self;
  (void)entity;
  (void)position;
  (void)size;
  (void)texture_id;
}
static void voxel_renderer_render_ui_quad_impl(IRenderer *self, Vec2 pos,
                                               Vec2 size, u32 texture_id) {
  (void)self;
  (void)pos;
  (void)size;
  (void)texture_id;
}
static void voxel_renderer_render_text_impl(IRenderer *self, const char *text,
                                            Vec2 pos, f32 scale, Vec3 color) {
  (void)self;
  (void)text;
  (void)pos;
  (void)scale;
  (void)color;
}
static void
voxel_renderer_render_block_highlight_impl(IRenderer *self,
                                           struct PlayerSystem *player_system) {
  (void)self;
  (void)player_system;
}
static void voxel_renderer_render_physics_debug_impl(IRenderer *self, Mat4 view,
                                                     Mat4 proj) {
  (void)self;
  (void)view;
  (void)proj;
}
static void voxel_renderer_render_debug_line_impl(IRenderer *self, Vec3 start,
                                                  Vec3 end, Vec3 color) {
  (void)self;
  (void)start;
  (void)end;
  (void)color;
}
static void voxel_renderer_render_debug_box_impl(IRenderer *self, Vec3 center,
                                                 Vec3 size, Quat rotation,
                                                 Vec3 color) {
  (void)self;
  (void)center;
  (void)size;
  (void)rotation;
  (void)color;
}
static void voxel_renderer_render_debug_sphere_impl(IRenderer *self,
                                                    Vec3 center, f32 radius,
                                                    Vec3 color) {
  (void)self;
  (void)center;
  (void)radius;
  (void)color;
}
static void voxel_renderer_set_ambient_light_impl(IRenderer *self,
                                                  f32 ambient_light) {
  VoxelRenderer *vr = (VoxelRenderer *)self->impl_data;
  if (vr)
    vr->enable_ao = (ambient_light > 0.5f); // Simplified
}
static bool voxel_renderer_create_chunk_buffers_impl(IRenderer *self,
                                                     Mesh *mesh,
                                                     void **vertex_buffer,
                                                     void **index_buffer) {
  (void)self;
  (void)mesh;
  (void)vertex_buffer;
  (void)index_buffer;
  return false;
}
static bool voxel_renderer_update_chunk_buffers_impl(IRenderer *self,
                                                     Mesh *mesh,
                                                     void *vertex_buffer,
                                                     void *index_buffer) {
  (void)self;
  (void)mesh;
  (void)vertex_buffer;
  (void)index_buffer;
  return false;
}

// Factory Implementation
IRenderer *renderer_create_with_backend(RendererType type, GPUBackend backend) {
  if (type == RENDERER_TYPE_VOXEL && backend == GPU_BACKEND_METAL) {
    IRenderer *renderer = calloc(1, sizeof(IRenderer));
    if (!renderer)
      return NULL;

    renderer->type = type;
    renderer->init = voxel_renderer_init_impl;
    renderer->cleanup = voxel_renderer_cleanup_impl;
    renderer->resize = voxel_renderer_resize_impl;
    renderer->begin_frame = voxel_renderer_begin_frame_impl;
    renderer->end_frame = voxel_renderer_end_frame_impl;
    renderer->update_camera = voxel_renderer_update_camera_impl;
    renderer->update_camera_uniforms =
        voxel_renderer_update_camera_uniforms_impl;
    renderer->render_chunk = voxel_renderer_render_chunk_impl;
    renderer->render_chunk_mesh = voxel_renderer_render_chunk_mesh_impl;
    renderer->render_dynamic_mesh = voxel_renderer_render_dynamic_mesh_impl;
    renderer->render_sprite = voxel_renderer_render_sprite_impl;
    renderer->render_entity_sprite = voxel_renderer_render_entity_sprite_impl;
    renderer->render_ui_quad = voxel_renderer_render_ui_quad_impl;
    renderer->render_text = voxel_renderer_render_text_impl;
    renderer->render_block_highlight =
        voxel_renderer_render_block_highlight_impl;
    renderer->render_physics_debug = voxel_renderer_render_physics_debug_impl;
    renderer->render_debug_line = voxel_renderer_render_debug_line_impl;
    renderer->render_debug_box = voxel_renderer_render_debug_box_impl;
    renderer->render_debug_sphere = voxel_renderer_render_debug_sphere_impl;
    renderer->set_ambient_light = voxel_renderer_set_ambient_light_impl;
    renderer->create_chunk_buffers = voxel_renderer_create_chunk_buffers_impl;
    renderer->update_chunk_buffers = voxel_renderer_update_chunk_buffers_impl;
    renderer->get_backend_name = voxel_renderer_get_backend_name_impl;

    // Initialize the concrete renderer
    // Note: We delay actual creation until init() is called with window
    renderer->impl_data = NULL;

    return renderer;
  }
  return NULL;
}
