/*
 * oit_wboit.c
 * Weighted Blended OIT Implementation
 */

#include "rendering/forward/oit_wboit.h"
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

struct OITContext {
    u32 width;
    u32 height;
    
    // Render Targets for WBOIT
    // Accumulation Buffer: RGBA16F
    //   RGB channels: sum(Ci * ai * w(zi, ai))  where w is weight function
    //   A channel: sum(ai * w(zi, ai))
    void* accum_texture;
    
    // Reveal Buffer: R16F or R8
    //   R channel: product(1 - ai) - "revealage" or transmittance
    void* reveal_texture;
    
    // Framebuffer that attaches these textures
    void* framebuffer;
    
    // Weight calculation parameters
    struct {
        float weight_bias;      // Typically around 10.0
        float weight_scale;     // Typically around 3000.0
        float weight_power;     // Typically around 2.0
    } params;
};

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static float calculate_wboit_weight(float z, float alpha, const OITContext* ctx) {
    // Weight function based on depth and alpha
    // Common formula: w = alpha * max(0.01, min(3000.0, 10.0 / (1e-5 + depth^2)))
    // This biases closer fragments more strongly
    
    float depth_weight = ctx->params.weight_bias / (1e-5f + powf(z, ctx->params.weight_power));
    depth_weight = fminf(ctx->params.weight_scale, depth_weight);
    depth_weight = fmaxf(0.01f, depth_weight);
    
    return alpha * depth_weight;
}

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

OITContext* oit_wboit_create(u32 width, u32 height) {
    OITContext* ctx = (OITContext*)malloc(sizeof(OITContext));
    if (!ctx) return NULL;
    
    ctx->width = width;
    ctx->height = height;
    
    // Initialize weight parameters with common defaults
    ctx->params.weight_bias = 10.0f;
    ctx->params.weight_scale = 3000.0f;
    ctx->params.weight_power = 2.0f;
    
    // TODO: Create GPU Textures
    // accum_texture = create_texture(width, height, FORMAT_RGBA16F, CLEAR_COLOR(0,0,0,0));
    // reveal_texture = create_texture(width, height, FORMAT_R16F, CLEAR_COLOR(1,0,0,0)); // 1.0 = fully revealed
    ctx->accum_texture = NULL;
    ctx->reveal_texture = NULL;
    
    // TODO: Create framebuffer with MRT (Multiple Render Targets)
    // framebuffer = create_framebuffer([accum_texture, reveal_texture]);
    ctx->framebuffer = NULL;
    
    LOG_INFO("WBOIT: Initialized context %dx%d", width, height);
    return ctx;
}

void oit_wboit_destroy(OITContext* ctx) {
    if (!ctx) return;
    
    // TODO: Release GPU textures and framebuffer
    // destroy_texture(ctx->accum_texture);
    // destroy_texture(ctx->reveal_texture);
    // destroy_framebuffer(ctx->framebuffer);
    
    free(ctx);
    LOG_INFO("WBOIT: Destroyed context");
}

void oit_wboit_resize(OITContext* ctx, u32 width, u32 height) {
    if (!ctx) return;
    if (ctx->width == width && ctx->height == height) return;
    
    ctx->width = width;
    ctx->height = height;
    
    // TODO: Recreate textures with new dimensions
    // destroy_texture(ctx->accum_texture);
    // destroy_texture(ctx->reveal_texture);
    // ctx->accum_texture = create_texture(width, height, FORMAT_RGBA16F);
    // ctx->reveal_texture = create_texture(width, height, FORMAT_R16F);
    
    LOG_INFO("WBOIT: Resized to %dx%d", width, height);
}

void oit_wboit_begin_pass(OITContext* ctx) {
    if (!ctx) return;
    
    // TODO: Bind framebuffer for MRT
    // framebuffer_bind(ctx->framebuffer);
    
    // TODO: Clear accumulation buffer to (0, 0, 0, 0)
    // clear_color_target(0, vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    // TODO: Clear reveal buffer to 1.0 (fully revealed/no coverage)
    // clear_color_target(1, vec4(1.0f, 0.0f, 0.0f, 0.0f));
    
    // TODO: Set blend modes for each MRT
    // For accumulation buffer (MRT 0):
    //   glBlendFunci(0, GL_ONE, GL_ONE); // Additive blending
    
    // For reveal buffer (MRT 1):
    //   glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // Multiplicative blending for revealage
    
    LOG_DEBUG("WBOIT: Begin accumulation pass");
}

void oit_wboit_render_transparent(OITContext* ctx, void* draw_commands) {
    if (!ctx) return;
    
    // Render transparent geometry
    // Each transparent fragment outputs:
    //   MRT0 (accum): vec4(color.rgb * alpha * weight, alpha * weight)
    //   MRT1 (reveal): vec4(alpha, 0, 0, 0)  // Will be multiplied as (1 - alpha)
    
    // Weight calculation is done in the fragment shader:
    // float weight = calculate_wboit_weight(gl_FragCoord.z, alpha, params);
    
    // TODO: Iterate and render all transparent draw commands
    // The shader should output to both MRT targets simultaneously
    
    LOG_DEBUG("WBOIT: Rendering transparent geometry");
}

void oit_wboit_composite(OITContext* ctx) {
    if (!ctx) return;
    
    // Composite (resolve) pass: combines WBOIT buffers into final image
    
    // TODO: Unbind WBOIT framebuffer, bind main/output framebuffer
    // framebuffer_unbind();
    
    // TODO: Bind composite shader
    // shader_bind(wboit_composite_shader);
    
    // TODO: Bind accumulation and reveal textures as inputs
    // shader_set_texture("u_accum_texture", ctx->accum_texture, 0);
    // shader_set_texture("u_reveal_texture", ctx->reveal_texture, 1);
    
    // TODO: Set blend mode for compositing onto opaque scene
    // Standard: SrcFactor=SRC_ALPHA, DstFactor=ONE_MINUS_SRC_ALPHA
    // Or: SrcFactor=ONE, DstFactor=ONE_MINUS_SRC_ALPHA for premultiplied alpha
    
    // The composite fragment shader does:
    /*
        vec4 accum = texture(u_accum_texture, uv);
        float reveal = texture(u_reveal_texture, uv).r;
        
        // Avoid division by zero
        if (reveal >= 1.0) discard; // No transparency at this pixel
        
        // Calculate average color
        vec3 average_color = accum.rgb / max(accum.a, 1e-5);
        
        // Output final transparent color with correct alpha
        float final_alpha = 1.0 - reveal;
        fragColor = vec4(average_color * final_alpha, final_alpha);
    */
    
    // TODO: Draw fullscreen quad/triangle
    // draw_fullscreen_triangle();
    
    LOG_DEBUG("WBOIT: Composite pass executed");
}

void oit_wboit_set_weight_params(OITContext* ctx, float bias, float scale, float power) {
    if (!ctx) return;
    
    ctx->params.weight_bias = bias;
    ctx->params.weight_scale = scale;
    ctx->params.weight_power = power;
    
    LOG_INFO("WBOIT: Weight params updated (bias=%.2f, scale=%.2f, power=%.2f)", bias, scale, power);
}

void* oit_wboit_get_accum_texture(OITContext* ctx) {
    return ctx ? ctx->accum_texture : NULL;
}

void* oit_wboit_get_reveal_texture(OITContext* ctx) {
    return ctx ? ctx->reveal_texture : NULL;
}
