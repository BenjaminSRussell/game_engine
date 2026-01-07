/*
 * oit_wboit.c
 * Weighted Blended OIT Implementation
 */

#include "oit_wboit.h"
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

struct OITContext {
    u32 width;
    u32 height;
    
    // Render Targets
    void* accum_texture;   // RGBA16F: stores weighted color (rgb * a * w) and weight (a * w) ??? 
                           // actually usually: RGB=sum(Ci * ai * w), A=sum(ai * w) or similar
                           // Common Setup:
                           // Accum: RGBA16F (r: R*a*w, g: G*a*w, b: B*a*w, a: a*w) -> NO
                           // Standard: Accum RGB = sum(Ci * ai * w(zi, ai)), A = sum(ai * w(zi, ai))
                           // Wait, standard paper uses Accum.rgb = sum(Ci * ai * w), Accum.a = sum(ai * w) 
                           // AND Reveal.r = product(1 - ai)
    
    void* reveal_texture;  // R8 or R16F: stores product(1 - a) aka 'revealage'
    
    // Framebuffer that attaches these textures
    void* framebuffer; 
};

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

OITContext* oit_wboit_create(u32 width, u32 height) {
    OITContext* ctx = (OITContext*)malloc(sizeof(OITContext));
    if (!ctx) return NULL;
    
    ctx->width = width;
    ctx->height = height;
    
    // TODO: Create GPU Textures
    // accum_texture = create_texture(width, height, FORMAT_RGBA16F);
    // reveal_texture = create_texture(width, height, FORMAT_R16F);
    ctx->accum_texture = NULL;
    ctx->reveal_texture = NULL;
    ctx->framebuffer = NULL;
    
    LOG_INFO("WBOIT: Initialized contexts %dx%d", width, height);
    return ctx;
}

void oit_wboit_destroy(OITContext* ctx) {
    if (!ctx) return;
    // TODO: Release textures
    free(ctx);
}

void oit_wboit_resize(OITContext* ctx, u32 width, u32 height) {
    if (!ctx) return;
    if (ctx->width == width && ctx->height == height) return;
    
    ctx->width = width;
    ctx->height = height;
    
    // TODO: Recreate textures
    LOG_INFO("WBOIT: Resized to %dx%d", width, height);
}

void oit_wboit_begin_pass(OITContext* ctx) {
    if (!ctx) return;
    
    // TODO: Bind framebuffer
    
    // TODO: Clear targets
    // Accumulation: vec4(0.0)
    // Reveal: 1.0 (fully revealed)
}

void oit_wboit_composite(OITContext* ctx) {
    if (!ctx) return;
    
    // TODO: Bind composite shader
    // TODO: Set blend mode: 
    // Src = (1-SrcAlpha) * SrcColor + DstColor
    // Actually blend func is usually: SrcFactor=ONE_MINUS_SRC_ALPHA, DstFactor=SRC_ALPHA
    
    // The composite shader does:
    // vec4 accum = texture(accum_tex, uv);
    // float reveal = texture(reveal_tex, uv).r;
    // if (reveal == 1.0) discard;
    // vec3 average_color = accum.rgb / max(accum.a, 0.00001);
    // output = vec4(average_color, 1.0 - reveal);
    
    // TODO: Draw fullscreen quad
}

void* oit_wboit_get_accum_texture(OITContext* ctx) {
    return ctx ? ctx->accum_texture : NULL;
}

void* oit_wboit_get_reveal_texture(OITContext* ctx) {
    return ctx ? ctx->reveal_texture : NULL;
}
