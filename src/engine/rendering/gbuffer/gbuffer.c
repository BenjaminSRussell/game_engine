/**
 * =================================================================================================
 *                              G-BUFFER IMPLEMENTATION
 * =================================================================================================
 */

#include "rendering/gbuffer/gbuffer.h"
#include <core/memory.h>
#include <core/logger.h>
#include <rendering/render_pipeline.h>
#include <rendering/framebuffer.h>
#include <rendering/render_types.h>

GBuffer* gbuffer_create(u32 width, u32 height) {
    GBuffer *gbuffer = (GBuffer*)memory_allocate(sizeof(GBuffer), MEMORY_TAG_RENDERER);
    gbuffer->width = width;
    gbuffer->height = height;

    LOG_INFO("Creating G-Buffer: %dx%d", width, height);

    // 1. Create Textures with proper formats
    
    // Albedo: RGBA8 (8-bit per channel, standard color)
    gbuffer->texture_albedo = texture_create_2d(width, height, TEX_FORMAT_RGBA8);

    // Normal: RG16F (2-channel 16-bit float for octahedral encoding)
    gbuffer->texture_normal = texture_create_2d(width, height, TEX_FORMAT_RG16F);

    // Material: RGBA8 (Metallic, Roughness, AO, Flags)
    gbuffer->texture_material = texture_create_2d(width, height, TEX_FORMAT_RGBA8);

    // Emissive: R11G11B10F (Packed HDR format, more efficient than RGBA16F)
    gbuffer->texture_emissive = texture_create_2d(width, height, TEX_FORMAT_R11G11B10F);
    
    // Velocity: RG16F (2-channel for screen space motion vectors)
    gbuffer->texture_velocity = texture_create_2d(width, height, TEX_FORMAT_RG16F);

    // Depth: D32F (32-bit float depth buffer)
    gbuffer->texture_depth = texture_create_2d(width, height, TEX_FORMAT_D32F);

    // 2. Create and configure framebuffer
    gbuffer->framebuffer = framebuffer_create(width, height);
    
    // Attach all textures to framebuffer
    framebuffer_attach_color(gbuffer->framebuffer, 0, gbuffer->texture_albedo);
    framebuffer_attach_color(gbuffer->framebuffer, 1, gbuffer->texture_normal);
    framebuffer_attach_color(gbuffer->framebuffer, 2, gbuffer->texture_material);
    framebuffer_attach_color(gbuffer->framebuffer, 3, gbuffer->texture_emissive);
    framebuffer_attach_color(gbuffer->framebuffer, 4, gbuffer->texture_velocity);
    framebuffer_attach_depth(gbuffer->framebuffer, gbuffer->texture_depth);
    
    // Validate framebuffer
    if (!framebuffer_validate(gbuffer->framebuffer)) {
        LOG_ERROR("G-Buffer framebuffer validation failed!");
        gbuffer_destroy(gbuffer);
        return NULL;
    }
    
    LOG_INFO("G-Buffer created successfully with 5 color attachments + depth");

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
        framebuffer_destroy(gbuffer->framebuffer);
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
    gbuffer->texture_normal = texture_create_2d(width, height, TEX_FORMAT_RG16F);
    gbuffer->texture_material = texture_create_2d(width, height, TEX_FORMAT_RGBA8);
    gbuffer->texture_emissive = texture_create_2d(width, height, TEX_FORMAT_R11G11B10F);
    gbuffer->texture_velocity = texture_create_2d(width, height, TEX_FORMAT_RG16F);
    gbuffer->texture_depth = texture_create_2d(width, height, TEX_FORMAT_D32F);
    
    // Recreate framebuffer
    if (gbuffer->framebuffer) {
        framebuffer_destroy(gbuffer->framebuffer);
    }
    
    gbuffer->framebuffer = framebuffer_create(width, height);
    framebuffer_attach_color(gbuffer->framebuffer, 0, gbuffer->texture_albedo);
    framebuffer_attach_color(gbuffer->framebuffer, 1, gbuffer->texture_normal);
    framebuffer_attach_color(gbuffer->framebuffer, 2, gbuffer->texture_material);
    framebuffer_attach_color(gbuffer->framebuffer, 3, gbuffer->texture_emissive);
    framebuffer_attach_color(gbuffer->framebuffer, 4, gbuffer->texture_velocity);
    framebuffer_attach_depth(gbuffer->framebuffer, gbuffer->texture_depth);
    framebuffer_validate(gbuffer->framebuffer);

}

void gbuffer_bind(GBuffer *gbuffer) {
    if (!gbuffer || !gbuffer->framebuffer) {
        LOG_ERROR("gbuffer_bind: Invalid G-Buffer");
        return;
    }
    
    framebuffer_bind(gbuffer->framebuffer);
    
    // Clear all attachments
    framebuffer_clear_color(gbuffer->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);
    framebuffer_clear_depth(gbuffer->framebuffer, 1.0f);
}

void gbuffer_unbind(GBuffer *gbuffer) {
    framebuffer_unbind();
}

void gbuffer_bind_textures(GBuffer *gbuffer, u32 start_slot) {
    if (!gbuffer) return;
    
    // Bind all G-buffer textures for reading in the lighting pass
    shader_set_texture(0, "texAlbedo", gbuffer->texture_albedo);
    shader_set_texture(0, "texNormal", gbuffer->texture_normal);
    shader_set_texture(0, "texMaterial", gbuffer->texture_material);
    shader_set_texture(0, "texDepth", gbuffer->texture_depth);
    shader_set_texture(0, "texEmissive", gbuffer->texture_emissive);
    
    // Note: Using placeholder shader ID of 0, actual implementation should use active shader
}
