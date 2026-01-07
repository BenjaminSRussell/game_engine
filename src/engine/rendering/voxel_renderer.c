// src/render/voxel_renderer.c
//
// Purpose: Implements VoxelRenderer, a concrete implementation of IRenderer
// that wraps the existing VulkanRenderer for 3D voxel rendering.
//
#include "rendering/renderer.h"
#include "rendering/vulkan.h"
#include "rendering/camera.h"
#include "../include/chunk/chunk.h"
#include "rendering/mesh.h"
#include "../include/player/player.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

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
static void voxel_renderer_update_camera(IRenderer *self, struct Camera *camera, f32 aspect);
static void voxel_renderer_update_camera_uniforms(IRenderer *self, struct Camera *camera, f32 aspect);
static void voxel_renderer_render_chunk(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj);
static void voxel_renderer_render_chunk_mesh(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj);
static void voxel_renderer_render_dynamic_mesh(IRenderer *self, Mesh *mesh, Mat4 view, Mat4 proj);
static void voxel_renderer_render_sprite(IRenderer *self, Vec3 position, Vec2 size, u32 texture_id, f32 rotation);
static void voxel_renderer_render_entity_sprite(IRenderer *self, EntityID entity, Vec3 position, Vec2 size, u32 texture_id);
static void voxel_renderer_render_ui_quad(IRenderer *self, Vec2 pos, Vec2 size, u32 texture_id);
static void voxel_renderer_render_text(IRenderer *self, const char *text, Vec2 pos, f32 scale, Vec3 color);
static void voxel_renderer_render_block_highlight(IRenderer *self, struct PlayerSystem *player_system);
static void voxel_renderer_render_physics_debug(IRenderer *self, Mat4 view, Mat4 proj);
static void voxel_renderer_set_ambient_light(IRenderer *self, f32 ambient_light);
static bool voxel_renderer_create_chunk_buffers(IRenderer *self, Mesh *mesh, void **vertex_buffer, void **index_buffer);
static bool voxel_renderer_update_chunk_buffers(IRenderer *self, Mesh *mesh, void *vertex_buffer, void *index_buffer);

// Create voxel renderer
IRenderer *voxel_renderer_create(void) {
    IRenderer *renderer = (IRenderer *)calloc(1, sizeof(IRenderer));
    if (!renderer) return NULL;
    
    VoxelRendererData *data = (VoxelRendererData *)calloc(1, sizeof(VoxelRendererData));
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
    
    // Initialize function pointers
    renderer->type = RENDERER_TYPE_VOXEL;
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
    renderer->impl_data = data;
    
    return renderer;
}

// Implementation functions
static bool voxel_renderer_init(IRenderer *self, RendererInitParams *params) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return false;
    
    bool success = vulkan_init(data->vulkan_renderer, params->window, 
                               params->width, params->height, params->config);
    if (success) {
        data->initialized = true;
    }
    return success;
}

static void voxel_renderer_cleanup(IRenderer *self) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data) return;
    
    if (data->initialized && data->vulkan_renderer) {
        vulkan_cleanup(data->vulkan_renderer);
    }
    
    if (data->vulkan_renderer) {
        free(data->vulkan_renderer);
    }
    free(data);
    self->impl_data = NULL;
}

static void voxel_renderer_resize(IRenderer *self, u32 width, u32 height) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return;
    
    vulkan_recreate_swapchain(data->vulkan_renderer, width, height);
}

static bool voxel_renderer_begin_frame(IRenderer *self, u32 *image_index) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return false;
    
    return vulkan_begin_frame(data->vulkan_renderer, image_index);
}

static void voxel_renderer_end_frame(IRenderer *self, u32 image_index) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return;
    
    vulkan_end_frame(data->vulkan_renderer, image_index);
}

static void voxel_renderer_update_camera(IRenderer *self, struct Camera *camera, f32 aspect) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !camera) return;
    
    vulkan_update_camera(data->vulkan_renderer, camera);
}

static void voxel_renderer_update_camera_uniforms(IRenderer *self, struct Camera *camera, f32 aspect) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !camera) return;
    
    vulkan_update_camera_uniforms(data->vulkan_renderer, camera, aspect);
}

static void voxel_renderer_render_chunk(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !chunk) return;
    
    // Use chunk mesh if available
    if (chunk->mesh.vertex_count > 0) {
        vulkan_render_chunk_mesh(data->vulkan_renderer, chunk, view, proj);
    }
}

static void voxel_renderer_render_chunk_mesh(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !chunk) return;
    
    vulkan_render_chunk_mesh(data->vulkan_renderer, chunk, view, proj);
}

static void voxel_renderer_render_dynamic_mesh(IRenderer *self, Mesh *mesh, Mat4 view, Mat4 proj) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !mesh) return;
    
    vulkan_render_dynamic_mesh(data->vulkan_renderer, mesh, view, proj);
}

static void voxel_renderer_render_sprite(IRenderer *self, Vec3 position, Vec2 size, u32 texture_id, f32 rotation) {
    // Voxel renderer doesn't support sprites - this is a no-op
    (void)self; (void)position; (void)size; (void)texture_id; (void)rotation;
}

static void voxel_renderer_render_entity_sprite(IRenderer *self, EntityID entity, Vec3 position, Vec2 size, u32 texture_id) {
    // Voxel renderer doesn't support entity sprites - this is a no-op
    (void)self; (void)entity; (void)position; (void)size; (void)texture_id;
}

static void voxel_renderer_render_ui_quad(IRenderer *self, Vec2 pos, Vec2 size, u32 texture_id) {
    // UI rendering is handled separately - this is a placeholder
    (void)self; (void)pos; (void)size; (void)texture_id;
}

static void voxel_renderer_render_text(IRenderer *self, const char *text, Vec2 pos, f32 scale, Vec3 color) {
    // Text rendering is handled separately - this is a placeholder
    (void)self; (void)text; (void)pos; (void)scale; (void)color;
}

static void voxel_renderer_render_block_highlight(IRenderer *self, struct PlayerSystem *player_system) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !player_system) return;
    
    vulkan_render_block_highlight(data->vulkan_renderer, player_system);
}

static void voxel_renderer_render_physics_debug(IRenderer *self, Mat4 view, Mat4 proj) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return;
    
    vulkan_render_physics_debug(data->vulkan_renderer, view, proj);
}

static void voxel_renderer_set_ambient_light(IRenderer *self, f32 ambient_light) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer) return;
    
    vulkan_set_ambient_light(data->vulkan_renderer, ambient_light);
}

static bool voxel_renderer_create_chunk_buffers(IRenderer *self, Mesh *mesh, void **vertex_buffer, void **index_buffer) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !mesh) return false;
    
    VkBuffer vk_vertex_buffer, vk_index_buffer;
    VkDeviceMemory vk_vertex_memory, vk_index_memory;
    
    bool success = vulkan_create_chunk_vertex_buffer(data->vulkan_renderer, mesh, 
                                                     &vk_vertex_buffer, &vk_vertex_memory);
    if (!success) return false;
    
    success = vulkan_create_chunk_index_buffer(data->vulkan_renderer, mesh,
                                                &vk_index_buffer, &vk_index_memory);
    if (!success) return false;
    
    *vertex_buffer = (void *)vk_vertex_buffer;
    *index_buffer = (void *)vk_index_buffer;
    return true;
}

static bool voxel_renderer_update_chunk_buffers(IRenderer *self, Mesh *mesh, void *vertex_buffer, void *index_buffer) {
    VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
    if (!data || !data->vulkan_renderer || !mesh) return false;
    
    return vulkan_update_chunk_buffers(data->vulkan_renderer, mesh,
                                       (VkBuffer)vertex_buffer, (VkBuffer)index_buffer);
}

// Forward declaration
IRenderer *sprite_3d_renderer_create(void);

// Factory function implementation
IRenderer *renderer_create(RendererType type) {
    switch (type) {
        case RENDERER_TYPE_VOXEL:
            return voxel_renderer_create();
        case RENDERER_TYPE_SPRITE_3D:
            return sprite_3d_renderer_create();
        default:
            return NULL;
    }
}

void renderer_destroy(IRenderer *renderer) {
    if (!renderer) return;
    
    if (renderer->cleanup) {
        renderer->cleanup(renderer);
    }
    free(renderer);
}

