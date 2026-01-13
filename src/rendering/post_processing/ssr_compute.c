// src/engine/rendering/post_processing/ssr_compute.c
//
// Purpose: Compute shader-based SSR implementation
// Provides high-performance screen-space reflections using GPU compute shaders

#include "rendering/post_processing/ssr_compute.h"
#include "rendering/core/texture.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct SSRComputeContext {
    SSRComputeSettings settings;
    
    // GPU resources
    TextureID scene_color;
    TextureID normal_roughness;
    TextureID depth_buffer;
    TextureID output_buffer;
    TextureID depth_hierarchy;  // Mip chain for hierarchical ray marching
    
    // Compute shader
    u32 compute_shader;
    
    // Screen dimensions
    u32 width;
    u32 height;
    
    // Uniform buffer
    struct {
        vec2 texel_size;
        f32 max_distance;
        f32 thickness;
        u32 max_steps;
        f32 stride;
        f32 fade_distance;
        vec2 projection_params; // near, far
        u32 padding[2];
    } uniforms;
    
    bool initialized;
} SSRComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

SSRComputeContext* ssr_compute_create(u32 width, u32 height) {
    LOG_INFO(LOG_CAT_RENDERER, "Creating SSR compute context (%ux%u)", width, height);
    
    SSRComputeContext* ctx = MALLOC_PERSISTENT(sizeof(SSRComputeContext));
    if (!ctx) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate SSR compute context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(SSRComputeContext));
    
    // Initialize default settings
    ctx->settings.max_distance = 50.0f;
    ctx->settings.thickness = 0.1f;
    ctx->settings.max_steps = 64;
    ctx->settings.stride = 1.0f;
    ctx->settings.fade_distance = 25.0f;
    
    ctx->width = width;
    ctx->height = height;
    
    // Create textures
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
        .min_filter = FILTER_LINEAR,
        .mag_filter = FILTER_LINEAR
    };
    
    ctx->output_buffer = texture_create(&desc);
    if (!ctx->output_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSR output buffer");
        ssr_compute_destroy(ctx);
        return NULL;
    }
    
    // Create depth hierarchy (mip chain)
    desc.format = TEXTURE_FORMAT_R16F;
    desc.usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_TRANSFER_SRC | TEXTURE_USAGE_TRANSFER_DST;
    desc.mip_levels = 6; // 6 mip levels for hierarchical ray marching
    
    ctx->depth_hierarchy = texture_create(&desc);
    if (!ctx->depth_hierarchy) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSR depth hierarchy");
        ssr_compute_destroy(ctx);
        return NULL;
    }
    
    // Load compute shader
    ctx->compute_shader = shader_load_compute("shaders/post_processing/ssr_compute.comp");
    if (!ctx->compute_shader) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to load SSR compute shader");
        ssr_compute_destroy(ctx);
        return NULL;
    }
    
    // Initialize uniforms
    ctx->uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
    ctx->uniforms.max_distance = ctx->settings.max_distance;
    ctx->uniforms.thickness = ctx->settings.thickness;
    ctx->uniforms.max_steps = ctx->settings.max_steps;
    ctx->uniforms.stride = ctx->settings.stride;
    ctx->uniforms.fade_distance = ctx->settings.fade_distance;
    ctx->uniforms.projection_params = (vec2){0.1f, 100.0f}; // Default near/far
    
    ctx->initialized = true;
    
    LOG_INFO(LOG_CAT_RENDERER, "SSR compute context created successfully");
    return ctx;
}

void ssr_compute_destroy(SSRComputeContext* ctx) {
    if (!ctx) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Destroying SSR compute context");
    
    if (ctx->output_buffer) {
        texture_destroy(ctx->output_buffer);
    }
    
    if (ctx->depth_hierarchy) {
        texture_destroy(ctx->depth_hierarchy);
    }
    
    if (ctx->compute_shader) {
        shader_destroy(ctx->compute_shader);
    }
    
    FREE(ctx);
}

// ============================================================================
// MAIN PROCESSING
// ============================================================================

void ssr_compute_process(SSRComputeContext* ctx,
                        TextureID scene_color,
                        TextureID normal_roughness,
                        TextureID depth_buffer) {
    if (!ctx || !ctx->initialized) {
        LOG_WARN(LOG_CAT_RENDERER, "SSR compute context not initialized");
        return;
    }
    
    // Update uniforms
    ctx->uniforms.max_distance = ctx->settings.max_distance;
    ctx->uniforms.thickness = ctx->settings.thickness;
    ctx->uniforms.max_steps = ctx->settings.max_steps;
    ctx->uniforms.stride = ctx->settings.stride;
    ctx->uniforms.fade_distance = ctx->settings.fade_distance;
    
    ctx->scene_color = scene_color;
    ctx->normal_roughness = normal_roughness;
    ctx->depth_buffer = depth_buffer;
    
    // Generate depth hierarchy mip chain
    texture_generate_mipmaps(depth_buffer);
    texture_copy_to_texture(depth_buffer, ctx->depth_hierarchy);
    
    // Bind resources for compute shader
    shader_bind_compute(ctx->compute_shader);
    
    // Bind textures
    texture_bind_compute(scene_color, 0);
    texture_bind_compute(normal_roughness, 1);
    texture_bind_compute(depth_buffer, 2);
    texture_bind_compute(ctx->depth_hierarchy, 3);
    
    // Bind output image
    texture_bind_image_compute(ctx->output_buffer, 0);
    
    // Set uniforms
    shader_set_uniform_compute(ctx->compute_shader, "params", 
                              &ctx->uniforms, sizeof(ctx->uniforms));
    
    // Dispatch compute shader
    u32 work_groups_x = (ctx->width + 15) / 16;
    u32 work_groups_y = (ctx->height + 15) / 16;
    
    shader_dispatch_compute(ctx->compute_shader, work_groups_x, work_groups_y, 1);
    
    // Memory barrier to ensure writes are complete
    shader_memory_barrier_compute();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void ssr_compute_update_settings(SSRComputeContext* ctx, const SSRComputeSettings* settings) {
    if (!ctx || !settings) return;
    
    ctx->settings = *settings;
    
    // Update uniforms that changed
    ctx->uniforms.max_distance = settings->max_distance;
    ctx->uniforms.thickness = settings->thickness;
    ctx->uniforms.max_steps = settings->max_steps;
    ctx->uniforms.stride = settings->stride;
    ctx->uniforms.fade_distance = settings->fade_distance;
}

TextureID ssr_compute_get_output(SSRComputeContext* ctx) {
    return ctx ? ctx->output_buffer : 0;
}

void ssr_compute_set_projection_params(SSRComputeContext* ctx, f32 near_plane, f32 far_plane) {
    if (!ctx) return;
    
    ctx->uniforms.projection_params = (vec2){near_plane, far_plane};
}

void ssr_compute_resize(SSRComputeContext* ctx, u32 new_width, u32 new_height) {
    if (!ctx || !ctx->initialized) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Resizing SSR compute context to %ux%u", new_width, new_height);
    
    ctx->width = new_width;
    ctx->height = new_height;
    
    // Recreate output buffer with new dimensions
    TextureDesc desc = {
        .width = new_width,
        .height = new_height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
        .min_filter = FILTER_LINEAR,
        .mag_filter = FILTER_LINEAR
    };
    
    texture_destroy(ctx->output_buffer);
    ctx->output_buffer = texture_create(&desc);
    
    // Recreate depth hierarchy
    desc.format = TEXTURE_FORMAT_R16F;
    desc.usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_TRANSFER_SRC | TEXTURE_USAGE_TRANSFER_DST;
    desc.mip_levels = 6;
    
    texture_destroy(ctx->depth_hierarchy);
    ctx->depth_hierarchy = texture_create(&desc);
    
    // Update texel size
    ctx->uniforms.texel_size = (vec2){1.0f / new_width, 1.0f / new_height};
    
    LOG_INFO(LOG_CAT_RENDERER, "SSR compute context resized successfully");
}
