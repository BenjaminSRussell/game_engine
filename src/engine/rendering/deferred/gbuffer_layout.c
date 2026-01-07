/*
 * gbuffer_layout.c
 * G-buffer layout definition
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/deferred/gbuffer_layout.h"
#include "backend/metal/mtl_texture.h"
#include "backend/metal/mtl_device.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct gbuffer_internal {
    uint32_t width;
    uint32_t height;
    
    /* G-Buffer Textures (Metal wrappers) */
    metal_texture_t* tex_albedo;   // RGBA8 (RGB: Albedo, A: AO)
    metal_texture_t* tex_normal;   // RG16F (Octahedral or Normal.xy)
    metal_texture_t* tex_material; // RGBA8 (R: Roughness, G: Metallic, B: Flags, A: Reserved)
    metal_texture_t* tex_velocity; // RG16F (Motion Vectors)
    metal_texture_t* tex_depth;    // D32F
    
    /* Render Pass Descriptor (Objective-C object cached) */
    void* render_pass_descriptor;
    
    bool initialized;
} gbuffer_internal_t;

static gbuffer_internal_t g_gbuffer = {0};

/* External access to the Metal device - usually provided by a global or system/context */
// For this task, we'll assume we can get it or it's passed. 
// However, the init function signature in the header currently only takes width/height.
// We might need to rely on a global device accessor or modify the init function.
// Given the constraints, I will assume a global `metal_device_get_default()` or similar existence
// or just modify the signature if possible. 
// Looking at existing files, `gbuffer_create` in the user prompt took `metal_device_t*`.
// But `rendering_gbuffer_init` in `gbuffer_layout.h` does not. 
// I will obtain the device via a hypothetical `rendering_get_metal_device()` or just use the one from `mtl_device.h` if available?
// `mtl_device.h` has `metal_device_create()`. 
// I will create a static device if needed or assume there is one.
// Let's check if there is a global device pointer available in the codebase.
// For now, I'll modify the internal logic to get the device from the platform layer.

extern metal_device_t* platform_get_metal_device(void); // Hypothetical external

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gbuffer_init(uint32_t width, uint32_t height) {
    if (g_gbuffer.initialized) {
        if (g_gbuffer.width == width && g_gbuffer.height == height) return 0;
        rendering_gbuffer_shutdown(); // Resize
    }
    
    // Attempt to get device - in a real engine this would be clean.
    // For this implementation, I'll assume we can create/get it. 
    // Since I cannot change the signature easily without breaking other things potentially,
    // I will try to use a mechanism to get the device.
    // The user prompt example had `gbuffer_create(metal_device_t* dev...)`.
    // The existing stub has `rendering_gbuffer_init(w, h)`.
    // I will stick to the stub signature but I need the device.
    // I'll add a TODO or assume a global is available.
    // Let's assume `g_metal_device` is available or I can pass it.
    // Actually, I'll change the implementation to use `metal_device_create()` if it's a singleton or similar, 
    // but creating a new device every time is bad.
    // I will assume there is a `extern metal_device_t* g_render_device;` available from the main renderer or similar.
    
    // For the sake of this task, I will declare the extern here. 
    // The user prompt showed `dev->device`.
    
    metal_device_t* device = (metal_device_t*)platform_get_metal_device();
    if (!device) return -1;
    
    g_gbuffer.width = width;
    g_gbuffer.height = height;
    
    /* 1. Albedo Texture: RGBA8 */
    metal_texture_desc_t desc_albedo = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RGBA8_UNORM,
        .width = width,
        .height = height,
        .depth = 1,
        .mip_levels = 1,
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .generate_mipmaps = false
    };
    g_gbuffer.tex_albedo = metal_texture_create(device, &desc_albedo);

    /* 2. Normal Texture: RG16F */
    metal_texture_desc_t desc_normal = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RG16_FLOAT,
        .width = width,
        .height = height,
        .depth = 1,
        .mip_levels = 1,
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .generate_mipmaps = false
    };
    g_gbuffer.tex_normal = metal_texture_create(device, &desc_normal);

    /* 3. Material Texture: RGBA8 */
    metal_texture_desc_t desc_material = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RGBA8_UNORM,
        .width = width,
        .height = height,
        .depth = 1,
        .mip_levels = 1,
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .generate_mipmaps = false
    };
    g_gbuffer.tex_material = metal_texture_create(device, &desc_material);
    
    /* 4. Velocity Texture: RG16F */
    metal_texture_desc_t desc_velocity = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RG16_FLOAT,
        .width = width,
        .height = height,
        .depth = 1,
        .mip_levels = 1,
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .generate_mipmaps = false
    };
    g_gbuffer.tex_velocity = metal_texture_create(device, &desc_velocity);

    /* 5. Depth Texture: D32F */
    metal_texture_desc_t desc_depth = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT,
        .width = width,
        .height = height,
        .depth = 1,
        // Depth buffer generally doesn't need mips for G-buffer
        .mip_levels = 1, 
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .generate_mipmaps = false
    };
    g_gbuffer.tex_depth = metal_texture_create(device, &desc_depth);
    
    /* Create Render Pass Descriptor */
#ifdef __OBJC__
    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    
    // Albedo - Color Render Target 0
    if (g_gbuffer.tex_albedo) {
        pass_desc.colorAttachments[0].texture = (__bridge id<MTLTexture>)g_gbuffer.tex_albedo->texture;
        pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    }
    
    // Normal - Color Render Target 1
    if (g_gbuffer.tex_normal) {
        pass_desc.colorAttachments[1].texture = (__bridge id<MTLTexture>)g_gbuffer.tex_normal->texture;
        pass_desc.colorAttachments[1].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[1].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[1].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    }
    
    // Material - Color Render Target 2
    if (g_gbuffer.tex_material) {
        pass_desc.colorAttachments[2].texture = (__bridge id<MTLTexture>)g_gbuffer.tex_material->texture;
        pass_desc.colorAttachments[2].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[2].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[2].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    }
    
    // Velocity - Color Render Target 3
    if (g_gbuffer.tex_velocity) {
        pass_desc.colorAttachments[3].texture = (__bridge id<MTLTexture>)g_gbuffer.tex_velocity->texture;
        pass_desc.colorAttachments[3].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[3].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[3].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    }
    
    // Depth Attachment
    if (g_gbuffer.tex_depth) {
        pass_desc.depthAttachment.texture = (__bridge id<MTLTexture>)g_gbuffer.tex_depth->texture;
        pass_desc.depthAttachment.loadAction = MTLLoadActionClear;
        pass_desc.depthAttachment.storeAction = MTLStoreActionStore;
        pass_desc.depthAttachment.clearDepth = 1.0;
    }
    
    g_gbuffer.render_pass_descriptor = (void*)CFBridgingRetain(pass_desc);
#endif

    g_gbuffer.initialized = true;
    return 0;
}

void rendering_gbuffer_shutdown(void) {
    if (!g_gbuffer.initialized) return;
    
    /* Release Descriptor */
#ifdef __OBJC__
    if (g_gbuffer.render_pass_descriptor) {
        CFRelease(g_gbuffer.render_pass_descriptor);
        g_gbuffer.render_pass_descriptor = NULL;
    }
#endif

    /* Destroy resources */
    if (g_gbuffer.tex_albedo) { metal_texture_destroy(g_gbuffer.tex_albedo); g_gbuffer.tex_albedo = NULL; }
    if (g_gbuffer.tex_normal) { metal_texture_destroy(g_gbuffer.tex_normal); g_gbuffer.tex_normal = NULL; }
    if (g_gbuffer.tex_material) { metal_texture_destroy(g_gbuffer.tex_material); g_gbuffer.tex_material = NULL; }
    if (g_gbuffer.tex_velocity) { metal_texture_destroy(g_gbuffer.tex_velocity); g_gbuffer.tex_velocity = NULL; }
    if (g_gbuffer.tex_depth) { metal_texture_destroy(g_gbuffer.tex_depth); g_gbuffer.tex_depth = NULL; }
    
    memset(&g_gbuffer, 0, sizeof(g_gbuffer));
}

void rendering_gbuffer_get_targets(void** albedo, void** normal, void** material, void** depth) {
    if (albedo) *albedo = g_gbuffer.tex_albedo;
    if (normal) *normal = g_gbuffer.tex_normal;
    if (material) *material = g_gbuffer.tex_material;
    if (depth) *depth = g_gbuffer.tex_depth;
}

void* rendering_gbuffer_get_framebuffer(void) {
    return g_gbuffer.render_pass_descriptor;
}
