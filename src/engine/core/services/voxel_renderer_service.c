// Voxel Renderer Service Implementation
// Bridges VoxelRenderer to the IRenderer interface

#include "rendering/voxel_renderer.h"
#include "rendering/renderer.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

// Forward declaration of VoxelRenderer
struct VoxelRenderer;

// Voxel renderer wrapper that implements IRenderer interface
typedef struct VoxelRendererService {
    IRenderer base;
    VoxelRenderer *voxel_renderer;
    
    // IRenderer state
    bool initialized;
    u32 width, height;
    
    // Statistics
    u32 draw_calls;
    u32 triangles;
    u32 vertices;
    f64 frame_time;
    
} VoxelRendererService;

// IRenderer interface implementation
static bool voxel_renderer_service_init(IRenderer *renderer, u32 width, u32 height);
static void voxel_renderer_service_shutdown(IRenderer *renderer);
static void voxel_renderer_service_begin_frame(IRenderer *renderer);
static void voxel_renderer_service_end_frame(IRenderer *renderer);
static void voxel_renderer_service_clear(IRenderer *renderer, const Vec4 *color);
static void voxel_renderer_service_present(IRenderer *renderer);
static void voxel_renderer_service_set_viewport(IRenderer *renderer, u32 x, u32 y, u32 width, u32 height);
static void voxel_renderer_service_set_camera(IRenderer *renderer, const Mat4 *view, const Mat4 *projection);
static void voxel_renderer_service_draw_mesh(IRenderer *renderer, const Mesh *mesh, const Mat4 *transform);
static void voxel_renderer_service_draw_mesh_instanced(IRenderer *renderer, const Mesh *mesh, u32 instance_count, const Mat4 *transforms);
static void voxel_renderer_service_set_lighting(IRenderer *renderer, const Light *lights, u32 light_count);
static void voxel_renderer_service_set_material(IRenderer *renderer, const Material *material);
static void voxel_renderer_service_set_texture(IRenderer *renderer, u32 slot, TextureID texture);
static void voxel_renderer_service_set_uniform(IRenderer *renderer, const char *name, const void *data, u32 size);
static void voxel_renderer_service_get_stats(IRenderer *renderer, RendererStats *stats);

// Create voxel renderer service
IRenderer *voxel_renderer_service_create(void) {
    VoxelRendererService *service = malloc(sizeof(VoxelRendererService));
    if (!service) {
        LOG_ERROR("Failed to allocate voxel renderer service");
        return NULL;
    }
    
    memset(service, 0, sizeof(VoxelRendererService));
    
    // Set up IRenderer interface
    service->base.init = voxel_renderer_service_init;
    service->base.shutdown = voxel_renderer_service_shutdown;
    service->base.begin_frame = voxel_renderer_service_begin_frame;
    service->base.end_frame = voxel_renderer_service_end_frame;
    service->base.clear = voxel_renderer_service_clear;
    service->base.present = voxel_renderer_service_present;
    service->base.set_viewport = voxel_renderer_service_set_viewport;
    service->base.set_camera = voxel_renderer_service_set_camera;
    service->base.draw_mesh = voxel_renderer_service_draw_mesh;
    service->base.draw_mesh_instanced = voxel_renderer_service_draw_mesh_instanced;
    service->base.set_lighting = voxel_renderer_service_set_lighting;
    service->base.set_material = voxel_renderer_service_set_material;
    service->base.set_texture = voxel_renderer_service_set_texture;
    service->base.set_uniform = voxel_renderer_service_set_uniform;
    service->base.get_stats = voxel_renderer_service_get_stats;
    
    // Create underlying voxel renderer
    service->voxel_renderer = voxel_renderer_create();
    if (!service->voxel_renderer) {
        LOG_ERROR("Failed to create voxel renderer");
        free(service);
        return NULL;
    }
    
    LOG_INFO("Voxel renderer service created");
    return (IRenderer *)service;
}

// Initialize renderer
static bool voxel_renderer_service_init(IRenderer *renderer, u32 width, u32 height) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    service->width = width;
    service->height = height;
    service->initialized = true;
    
    LOG_INFO("Voxel renderer service initialized: %ux%u", width, height);
    return true;
}

// Shutdown renderer
static void voxel_renderer_service_shutdown(IRenderer *renderer) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (service->voxel_renderer) {
        voxel_renderer_destroy(service->voxel_renderer);
        service->voxel_renderer = NULL;
    }
    
    service->initialized = false;
    LOG_INFO("Voxel renderer service shutdown");
}

// Begin frame
static void voxel_renderer_service_begin_frame(IRenderer *renderer) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized || !service->voxel_renderer) return;
    
    // Reset statistics
    service->draw_calls = 0;
    service->triangles = 0;
    service->vertices = 0;
    service->frame_time = 0.0;
    
    // Begin voxel renderer frame
    // voxel_renderer_begin_frame(service->voxel_renderer);
}

// End frame
static void voxel_renderer_service_end_frame(IRenderer *renderer) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized || !service->voxel_renderer) return;
    
    // Update voxel renderer
    voxel_renderer_update(service->voxel_renderer, 0.016f);  // 60 FPS assumption
    
    // Get statistics from voxel renderer
    voxel_renderer_get_stats(service->voxel_renderer, 
                            &service->draw_calls, 
                            &service->vertices, 
                            &service->triangles);
}

// Clear screen
static void voxel_renderer_service_clear(IRenderer *renderer, const Vec4 *color) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    // Clear implementation would go here
    // For now, just log
    LOG_DEBUG("Clear screen: r=%.2f g=%.2f b=%.2f a=%.2f", 
              color->x, color->y, color->z, color->w);
}

// Present frame
static void voxel_renderer_service_present(IRenderer *renderer) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized || !service->voxel_renderer) return;
    
    // Present implementation would go here
    LOG_DEBUG("Present frame");
}

// Set viewport
static void voxel_renderer_service_set_viewport(IRenderer *renderer, u32 x, u32 y, u32 width, u32 height) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    service->width = width;
    service->height = height;
    
    LOG_DEBUG("Set viewport: %u,%u %ux%u", x, y, width, height);
}

// Set camera
static void voxel_renderer_service_set_camera(IRenderer *renderer, const Mat4 *view, const Mat4 *projection) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized || !service->voxel_renderer) return;
    
    // Update voxel renderer camera
    voxel_renderer_render(service->voxel_renderer, view, projection);
    
    LOG_DEBUG("Set camera matrices");
}

// Draw mesh (not used in voxel renderer, but required for interface)
static void voxel_renderer_service_draw_mesh(IRenderer *renderer, const Mesh *mesh, const Mat4 *transform) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    // Voxel renderer doesn't draw individual meshes, but we log for debugging
    service->draw_calls++;
    LOG_DEBUG("Draw mesh (voxel renderer - no-op)");
}

// Draw mesh instanced (not used in voxel renderer)
static void voxel_renderer_service_draw_mesh_instanced(IRenderer *renderer, const Mesh *mesh, u32 instance_count, const Mat4 *transforms) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    service->draw_calls++;
    LOG_DEBUG("Draw mesh instanced: %d instances (voxel renderer - no-op)", instance_count);
}

// Set lighting (voxel renderer uses simple lighting)
static void voxel_renderer_service_set_lighting(IRenderer *renderer, const Light *lights, u32 light_count) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    LOG_DEBUG("Set lighting: %d lights (voxel renderer - simplified)", light_count);
}

// Set material (voxel renderer uses block materials)
static void voxel_renderer_service_set_material(IRenderer *renderer, const Material *material) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    LOG_DEBUG("Set material: %s (voxel renderer - block-based)", 
              material ? material->name : "NULL");
}

// Set texture (voxel renderer uses block texture atlas)
static void voxel_renderer_service_set_texture(IRenderer *renderer, u32 slot, TextureID texture) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    LOG_DEBUG("Set texture: slot %u (voxel renderer - uses block atlas)", slot);
}

// Set uniform
static void voxel_renderer_service_set_uniform(IRenderer *renderer, const char *name, const void *data, u32 size) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!service->initialized) return;
    
    LOG_DEBUG("Set uniform: %s (%u bytes) (voxel renderer - limited support)", name, size);
}

// Get statistics
static void voxel_renderer_service_get_stats(IRenderer *renderer, RendererStats *stats) {
    VoxelRendererService *service = (VoxelRendererService *)renderer;
    
    if (!stats) return;
    
    memset(stats, 0, sizeof(RendererStats));
    
    if (service->initialized) {
        stats->draw_calls = service->draw_calls;
        stats->triangles = service->triangles;
        stats->vertices = service->vertices;
        stats->frame_time = service->frame_time;
        stats->width = service->width;
        stats->height = service->height;
    }
}

// Legacy function for backward compatibility
IRenderer *voxel_renderer_create(void) {
    return voxel_renderer_service_create();
}
