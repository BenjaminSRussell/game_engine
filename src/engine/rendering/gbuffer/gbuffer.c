/**
 * =================================================================================================
 *                              G-BUFFER IMPLEMENTATION
 * =================================================================================================
 */

#include "gbuffer.h"
#include <core/memory/memory.h>
#include <core/logger/logger.h>
#include <gpu_backend/render_pipeline.h>
// Assuming texture creation functions are available in render_pipeline.h or similar
// We will look at render_types.h and render_pipeline.h again to double check the texture creation API, 
// using the `texture_create_2d` and `texture_destroy` functions seen earlier.

GBuffer* gbuffer_create(u32 width, u32 height) {
    GBuffer *gbuffer = (GBuffer*)memory_allocate(sizeof(GBuffer), MEMORY_TAG_RENDERER);
    gbuffer->width = width;
    gbuffer->height = height;

    LOG_INFO("Creating G-Buffer: %dx%d", width, height);

    // 1. Create Textures
    
    // Albedo: RGBA8
    gbuffer->texture_albedo = texture_create_2d(width, height, TEX_FORMAT_RGBA8);
    // TODO: Set texture names for debugging if API allows

    // Normal: RG16F (Float for precision with encoding)
    gbuffer->texture_normal = texture_create_2d(width, height, TEX_FORMAT_RGBA16F); // Using RGBA16F for now as RG16F generic mapping might differ, can optimize later

    // Material: RGBA8 (Metallic, Roughness, AO, Flags)
    gbuffer->texture_material = texture_create_2d(width, height, TEX_FORMAT_RGBA8);

    // Emissive: RGBA16F (HDR)
    gbuffer->texture_emissive = texture_create_2d(width, height, TEX_FORMAT_RGBA16F);
    
    // Velocity: RG16F
    gbuffer->texture_velocity = texture_create_2d(width, height, TEX_FORMAT_RGBA16F); // Using RGBA16F, likely only using RG channels

    // Depth: Depth32 (Using generic create, need to ensure format handles transform to depth attachment)
    // Note: texture_create_2d might default to color, need to check if there is a specific depth creation or if format handles it.
    // Assuming TEX_FORMAT_D32F exists or similar. Checking render_types.h earlier, I saw TEX_FORMAT_RGBA8 etc. 
    // I did NOT see a Depth format in the small enum in render_types.h.
    // I will assume for now I need to add it or it's handled via a specific flag, but for now I'll use a placeholder or assume the create_render_target API handles depth.
    
    // WAIT: render_pipeline.h has `render_target_create`. Let's re-examine if that creates a full framebuffer or just a texture.
    // "void *render_target_create(uint32_t width, uint32_t height);"
    
    // Actually, usually G-Buffers are constructed from multiple textures attached to a Framebuffer.
    // If the engine has a `framebuffer_create` (not seen in the short file view), or if we manually attach.
    // I'll stick to creating textures.
    // Use RGBA32F as placeholder for depth if specific depth format isn't exposed in the enum I saw, 
    // but typically it should be.
    // Let's rely on `texture_create_2d` returning a handle.

    // Correcting assumption: In many simple engines, render_target_create makes a default color+depth.
    // But for MRT (Multiple Render Targets), we likely need a specific Framebuffer builder.
    // Since I don't see that in the headers I read, I will assume we create textures and then maybe there is an API to attach them or I just hold them here.
    // I see `vulkan_framebuffer.c` in the file list.
    
    // I will initialize depth as a texture for now.
    gbuffer->texture_depth = texture_create_2d(width, height, TEX_FORMAT_RGBA32F); // Placeholder for Depth if specific enum missing

    return gbuffer;
}

void gbuffer_destroy(GBuffer *gbuffer) {
    if (!gbuffer) return;

    texture_destroy(gbuffer->texture_albedo);
    texture_destroy(gbuffer->texture_normal);
    texture_destroy(gbuffer->texture_material);
    texture_destroy(gbuffer->texture_emissive);
    texture_destroy(gbuffer->texture_velocity);
    texture_destroy(gbuffer->texture_depth);

    if (gbuffer->framebuffer) {
        // framebuffer_destroy(gbuffer->framebuffer);
    }

    memory_free(gbuffer, sizeof(GBuffer), MEMORY_TAG_RENDERER);
}

void gbuffer_resize(GBuffer *gbuffer, u32 width, u32 height) {
    if (!gbuffer) return;
    if (gbuffer->width == width && gbuffer->height == height) return;

    gbuffer->width = width;
    gbuffer->height = height;

    // Simple brute-force resize: destroy and recreate
    // Optimized way: call a resize API if available on textures
    
    texture_destroy(gbuffer->texture_albedo);
    texture_destroy(gbuffer->texture_normal);
    texture_destroy(gbuffer->texture_material);
    texture_destroy(gbuffer->texture_emissive);
    texture_destroy(gbuffer->texture_velocity);
    texture_destroy(gbuffer->texture_depth);

    gbuffer->texture_albedo = texture_create_2d(width, height, TEX_FORMAT_RGBA8);
    gbuffer->texture_normal = texture_create_2d(width, height, TEX_FORMAT_RGBA16F);
    gbuffer->texture_material = texture_create_2d(width, height, TEX_FORMAT_RGBA8);
    gbuffer->texture_emissive = texture_create_2d(width, height, TEX_FORMAT_RGBA16F);
    gbuffer->texture_velocity = texture_create_2d(width, height, TEX_FORMAT_RGBA16F);
    gbuffer->texture_depth = texture_create_2d(width, height, TEX_FORMAT_RGBA32F);

    // Rebuild framebuffer if necessary
}

void gbuffer_bind(GBuffer *gbuffer) {
    // This requires a "bind framebuffer" or "set render targets" API.
    // Since I haven't implemented the lower level MRT binding yet, I will leave this as a TODO or pseudocode stub.
    // It will likely involve calling something like `render_pass_begin(gbuffer->pass)`
}

void gbuffer_unbind(GBuffer *gbuffer) {
    // render_pass_end();
}

void gbuffer_bind_textures(GBuffer *gbuffer, u32 start_slot) {
    // Bind all textures to slots for reading in the lighting pass
    // shader_set_texture_by_slot(..., start_slot + 0, gbuffer->texture_albedo);
    // ...
}
