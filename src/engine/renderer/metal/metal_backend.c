#include "metal_backend.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

// Implemented in metal_device.m
extern void* metal_device_get(void);
extern void* metal_device_get_command_queue(void);
extern bool metal_device_init(void);
extern void metal_device_shutdown(void);
extern void metal_device_set_layer(void* layer);

static const char* metal_get_backend_name(IRenderer* self) {
    return "Metal";
}

static bool metal_init(IRenderer* self, RendererInitParams* params) {
    MetalRenderer* mr = (MetalRenderer*)self;
    LOG_INFO("Initializing Metal Backend...");

    if (!metal_device_init()) {
        LOG_ERROR("Failed to initialize Metal Device");
        return false;
    }

    mr->device = metal_device_get();
    mr->command_queue = metal_device_get_command_queue();
    
    // If we have a window/surface provided in params (generic pointer), use it
    // In hosted mode, window might be a pointer to CAMetalLayer
    if (params->window) {
        // Warning: This assumes params->window IS A CAMetalLayer* when using Metal backend in hosted mode
        // Or we need a specific 'surface' field.
        // For now, assuming direct pass-through for hosted scenarios.
       // metal_device_set_layer(params->window); 
    }

    LOG_INFO("Metal Backend Initialized Successfully");
    return true;
}

static void metal_cleanup(IRenderer* self) {
    LOG_INFO("Shutting down Metal Backend");
    metal_device_shutdown();
}

static void metal_resize(IRenderer* self, u32 width, u32 height) {
    // Metal layer resize handled by host usually, but we might need to update internal buffers
}

static bool metal_begin_frame(IRenderer* self, u32* image_index) {
    *image_index = 0; // Simplified for now
    return true;
}

static void metal_end_frame(IRenderer* self, u32 image_index) {
    // Commit command buffer
}

// Stubs for other methods
static void metal_update_camera(IRenderer *self, struct Camera *camera, f32 aspect) {}
static void metal_update_camera_uniforms(IRenderer *self, struct Camera *camera, f32 aspect) {}
static void metal_render_chunk(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj) {}
static void metal_render_chunk_mesh(IRenderer *self, Chunk *chunk, Mat4 view, Mat4 proj) {}
static void metal_render_dynamic_mesh(IRenderer *self, Mesh *mesh, Mat4 view, Mat4 proj) {}
static void metal_render_sprite(IRenderer *self, Vec3 position, Vec2 size, u32 texture_id, f32 rotation) {}
static void metal_render_entity_sprite(IRenderer *self, Entity entity, Vec3 position, Vec2 size, u32 texture_id) {}
static void metal_render_ui_quad(IRenderer *self, Vec2 pos, Vec2 size, u32 texture_id) {}
static void metal_render_text(IRenderer *self, const char *text, Vec2 pos, f32 scale, Vec3 color) {}
static void metal_render_block_highlight(IRenderer *self, struct PlayerSystem *player_system) {}
static void metal_render_physics_debug(IRenderer *self, Mat4 view, Mat4 proj) {}
static void metal_render_debug_line(IRenderer *self, Vec3 start, Vec3 end, Vec3 color) {}
static void metal_render_debug_box(IRenderer *self, Vec3 center, Vec3 size, Quat rotation, Vec3 color) {}
static void metal_render_debug_sphere(IRenderer *self, Vec3 center, f32 radius, Vec3 color) {}
static void metal_set_ambient_light(IRenderer *self, f32 ambient_light) {}
static bool metal_create_chunk_buffers(IRenderer *self, Mesh *mesh, void **vertex_buffer, void **index_buffer) { return true; }
static bool metal_update_chunk_buffers(IRenderer *self, Mesh *mesh, void *vertex_buffer, void *index_buffer) { return true; }

IRenderer* metal_renderer_create(void) {
    MetalRenderer* renderer = (MetalRenderer*)calloc(1, sizeof(MetalRenderer));
    
    renderer->interface.type = RENDERER_TYPE_VOXEL; // Masquerade as voxel for now, or use custom
    
    renderer->interface.init = metal_init;
    renderer->interface.cleanup = metal_cleanup;
    renderer->interface.resize = metal_resize;
    renderer->interface.begin_frame = metal_begin_frame;
    renderer->interface.end_frame = metal_end_frame;
    renderer->interface.get_backend_name = metal_get_backend_name;

    // Assign stubs
    renderer->interface.update_camera = metal_update_camera;
    renderer->interface.update_camera_uniforms = metal_update_camera_uniforms;
    renderer->interface.render_chunk = metal_render_chunk;
    renderer->interface.render_chunk_mesh = metal_render_chunk_mesh;
    renderer->interface.render_dynamic_mesh = metal_render_dynamic_mesh;
    renderer->interface.render_sprite = metal_render_sprite;
    renderer->interface.render_entity_sprite = metal_render_entity_sprite;
    renderer->interface.render_ui_quad = metal_render_ui_quad;
    renderer->interface.render_text = metal_render_text;
    renderer->interface.render_block_highlight = metal_render_block_highlight;
    renderer->interface.render_physics_debug = metal_render_physics_debug;
    renderer->interface.render_debug_line = metal_render_debug_line;
    renderer->interface.render_debug_box = metal_render_debug_box;
    renderer->interface.render_debug_sphere = metal_render_debug_sphere;
    renderer->interface.set_ambient_light = metal_set_ambient_light;
    renderer->interface.create_chunk_buffers = metal_create_chunk_buffers;
    renderer->interface.update_chunk_buffers = metal_update_chunk_buffers;

    return (IRenderer*)renderer;
}
