/*
 * gbuffer_layout.c
 * G-buffer layout definition
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "gbuffer_layout.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct gbuffer_internal {
    uint32_t width;
    uint32_t height;
    
    /* G-Buffer Textures */
    void* tex_albedo;   // RGBA8 (RGB: Albedo, A: AO)
    void* tex_normal;   // RG16F (Octahedral or Normal.xy)
    void* tex_material; // RGBA8 (R: Roughness, G: Metallic, B: Flags, A: Reserved)
    void* tex_depth;    // D32F
    
    /* Framebuffer handle */
    void* framebuffer;
    
    bool initialized;
} gbuffer_internal_t;

static gbuffer_internal_t g_gbuffer = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gbuffer_init(uint32_t width, uint32_t height) {
    if (g_gbuffer.initialized) {
        if (g_gbuffer.width == width && g_gbuffer.height == height) return 0;
        rendering_gbuffer_shutdown(); // Resize
    }
    
    g_gbuffer.width = width;
    g_gbuffer.height = height;
    
    /* Mock texture creation */
    // g_gbuffer.tex_albedo = create_texture(width, height, FORMAT_RGBA8_UNORM);
    // g_gbuffer.tex_normal = create_texture(width, height, FORMAT_RG16_FLOAT);
    // g_gbuffer.tex_material = create_texture(width, height, FORMAT_RGBA8_UNORM);
    // g_gbuffer.tex_depth = create_texture(width, height, FORMAT_D32_FLOAT);
    
    /* Mock framebuffer creation */
    // g_gbuffer.framebuffer = create_framebuffer(attachments...);
    
    g_gbuffer.initialized = true;
    return 0;
}

void rendering_gbuffer_shutdown(void) {
    if (!g_gbuffer.initialized) return;
    
    /* Destroy resources */
    // destroy_texture(g_gbuffer.tex_albedo); 
    // ...
    
    memset(&g_gbuffer, 0, sizeof(g_gbuffer));
}

void rendering_gbuffer_get_targets(void** albedo, void** normal, void** material, void** depth) {
    if (albedo) *albedo = g_gbuffer.tex_albedo;
    if (normal) *normal = g_gbuffer.tex_normal;
    if (material) *material = g_gbuffer.tex_material;
    if (depth) *depth = g_gbuffer.tex_depth;
}

void* rendering_gbuffer_get_framebuffer(void) {
    return g_gbuffer.framebuffer;
}
