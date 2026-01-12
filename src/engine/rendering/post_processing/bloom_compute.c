// src/engine/rendering/post_processing/bloom_compute.c
//
// Purpose: Compute shader-based Bloom implementation
// Provides high-performance bloom using GPU compute shaders

#include "rendering/post_processing/bloom_compute.h"
#include "rendering/core/texture.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct BloomComputeContext {
    BloomComputeSettings settings;
    
    // GPU resources - ping-pong buffers for blur passes
    TextureID threshold_buffer;
    TextureID blur_buffers[2];  // Two buffers for ping-pong blur
    TextureID output_buffer;
    
    // Compute shaders
    u32 threshold_shader;
    u32 blur_shader;
    u32 composite_shader;
    
    // Screen dimensions
    u32 width;
    u32 height;
    
    // Uniform buffers
    struct {
        f32 threshold;
        f32 soft_knee;
        f32 intensity;
        f32 padding;
    } threshold_uniforms;
    
    struct {
        vec2 texel_size;
        vec2 direction;
        f32 sigma;
        u32 padding[3];
    } blur_uniforms;
    
    struct {
        f32 intensity;
        f32 dirt_intensity;
        vec3 color_shift;
        vec2 texel_size;
        u32 padding[2];
    } composite_uniforms;
    
    bool initialized;
} BloomComputeContext;

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

BloomComputeContext* bloom_compute_create(u32 width, u32 height) {
    LOG_INFO(LOG_CAT_RENDERER, "Creating bloom compute context (%ux%u)", width, height);
    
    BloomComputeContext* ctx = MALLOC_PERSISTENT(sizeof(BloomComputeContext));
    if (!ctx) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate bloom compute context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(BloomComputeContext));
    
    // Initialize default settings
    ctx->settings.threshold = 1.0f;
    ctx->settings.soft_knee = 0.5f;
    ctx->settings.intensity = 0.04f;
    ctx->settings.iterations = 5;
    ctx->settings.sigma = 1.0f;
    ctx->settings.color_shift = (vec3){1.0f, 1.0f, 1.0f};
    
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
    
    ctx->threshold_buffer = texture_create(&desc);
    ctx->blur_buffers[0] = texture_create(&desc);
    ctx->blur_buffers[1] = texture_create(&desc);
    ctx->output_buffer = texture_create(&desc);
    
    if (!ctx->threshold_buffer || !ctx->blur_buffers[0] || 
        !ctx->blur_buffers[1] || !ctx->output_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create bloom textures");
        bloom_compute_destroy(ctx);
        return NULL;
    }
    
    // Load compute shaders
    ctx->threshold_shader = shader_load_compute("shaders/post_processing/bloom_threshold.comp");
    ctx->blur_shader = shader_load_compute("shaders/post_processing/bloom_blur.comp");
    ctx->composite_shader = shader_load_compute("shaders/post_processing/bloom_composite.comp");
    
    if (!ctx->threshold_shader || !ctx->blur_shader || !ctx->composite_shader) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to load bloom compute shaders");
        bloom_compute_destroy(ctx);
        return NULL;
    }
    
    // Initialize uniforms
    ctx->threshold_uniforms.threshold = ctx->settings.threshold;
    ctx->threshold_uniforms.soft_knee = ctx->settings.soft_knee;
    ctx->threshold_uniforms.intensity = ctx->settings.intensity;
    
    ctx->blur_uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
    ctx->blur_uniforms.direction = (vec2){1.0f, 0.0f};  // Horizontal first
    ctx->blur_uniforms.sigma = ctx->settings.sigma;
    
    ctx->composite_uniforms.intensity = ctx->settings.intensity;
    ctx->composite_uniforms.dirt_intensity = 0.0f;  // Could be configurable
    ctx->composite_uniforms.color_shift = ctx->settings.color_shift;
    ctx->composite_uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
    
    ctx->initialized = true;
    
    LOG_INFO(LOG_CAT_RENDERER, "Bloom compute context created successfully");
    return ctx;
}

void bloom_compute_destroy(BloomComputeContext* ctx) {
    if (!ctx) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Destroying bloom compute context");
    
    if (ctx->threshold_buffer) {
        texture_destroy(ctx->threshold_buffer);
    }
    
    if (ctx->blur_buffers[0]) {
        texture_destroy(ctx->blur_buffers[0]);
    }
    
    if (ctx->blur_buffers[1]) {
        texture_destroy(ctx->blur_buffers[1]);
    }
    
    if (ctx->output_buffer) {
        texture_destroy(ctx->output_buffer);
    }
    
    if (ctx->threshold_shader) {
        shader_destroy(ctx->threshold_shader);
    }
    
    if (ctx->blur_shader) {
        shader_destroy(ctx->blur_shader);
    }
    
    if (ctx->composite_shader) {
        shader_destroy(ctx->composite_shader);
    }
    
    FREE(ctx);
}

// ============================================================================
// MAIN PROCESSING
// ============================================================================

void bloom_compute_process(BloomComputeContext* ctx, TextureID hdr_input) {
    if (!ctx || !ctx->initialized) {
        LOG_WARN(LOG_CAT_RENDERER, "Bloom compute context not initialized");
        return;
    }
    
    // Update uniforms
    ctx->threshold_uniforms.threshold = ctx->settings.threshold;
    ctx->threshold_uniforms.soft_knee = ctx->settings.soft_knee;
    ctx->threshold_uniforms.intensity = ctx->settings.intensity;
    
    ctx->blur_uniforms.sigma = ctx->settings.sigma;
    
    ctx->composite_uniforms.intensity = ctx->settings.intensity;
    ctx->composite_uniforms.color_shift = ctx->settings.color_shift;
    
    // ============================================================================
    // STEP 1: Threshold extraction
    // ============================================================================
    shader_bind_compute(ctx->threshold_shader);
    
    texture_bind_compute(hdr_input, 0);
    texture_bind_image_compute(ctx->threshold_buffer, 0);
    
    shader_set_uniform_compute(ctx->threshold_shader, "params", 
                              &ctx->threshold_uniforms, sizeof(ctx->threshold_uniforms));
    
    u32 work_groups_x = (ctx->width + 15) / 16;
    u32 work_groups_y = (ctx->height + 15) / 16;
    
    shader_dispatch_compute(ctx->threshold_shader, work_groups_x, work_groups_y, 1);
    shader_memory_barrier_compute();
    
    // ============================================================================
    // STEP 2: Iterative blur (horizontal + vertical passes)
    // ============================================================================
    TextureID current_input = ctx->threshold_buffer;
    
    for (u32 i = 0; i < ctx->settings.iterations; i++) {
        // Horizontal pass
        ctx->blur_uniforms.direction = (vec2){1.0f, 0.0f};
        
        shader_bind_compute(ctx->blur_shader);
        texture_bind_compute(current_input, 0);
        texture_bind_image_compute(ctx->blur_buffers[0], 0);
        
        shader_set_uniform_compute(ctx->blur_shader, "params", 
                                  &ctx->blur_uniforms, sizeof(ctx->blur_uniforms));
        
        shader_dispatch_compute(ctx->blur_shader, work_groups_x, work_groups_y, 1);
        shader_memory_barrier_compute();
        
        // Vertical pass
        ctx->blur_uniforms.direction = (vec2){0.0f, 1.0f};
        
        shader_bind_compute(ctx->blur_shader);
        texture_bind_compute(ctx->blur_buffers[0], 0);
        texture_bind_image_compute(ctx->blur_buffers[1], 0);
        
        shader_set_uniform_compute(ctx->blur_shader, "params", 
                                  &ctx->blur_uniforms, sizeof(ctx->blur_uniforms));
        
        shader_dispatch_compute(ctx->blur_shader, work_groups_x, work_groups_y, 1);
        shader_memory_barrier_compute();
        
        // Use result as input for next iteration
        current_input = ctx->blur_buffers[1];
    }
    
    // ============================================================================
    // STEP 3: Composite with original scene
    // ============================================================================
    shader_bind_compute(ctx->composite_shader);
    
    texture_bind_compute(hdr_input, 0);
    texture_bind_compute(current_input, 1);
    texture_bind_image_compute(ctx->output_buffer, 0);
    
    shader_set_uniform_compute(ctx->composite_shader, "params", 
                              &ctx->composite_uniforms, sizeof(ctx->composite_uniforms));
    
    shader_dispatch_compute(ctx->composite_shader, work_groups_x, work_groups_y, 1);
    shader_memory_barrier_compute();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void bloom_compute_update_settings(BloomComputeContext* ctx, const BloomComputeSettings* settings) {
    if (!ctx || !settings) return;
    
    ctx->settings = *settings;
    
    // Update uniforms that changed
    ctx->threshold_uniforms.threshold = settings->threshold;
    ctx->threshold_uniforms.soft_knee = settings->soft_knee;
    ctx->threshold_uniforms.intensity = settings->intensity;
    
    ctx->blur_uniforms.sigma = settings->sigma;
    
    ctx->composite_uniforms.intensity = settings->intensity;
    ctx->composite_uniforms.color_shift = settings->color_shift;
}

TextureID bloom_compute_get_output(BloomComputeContext* ctx) {
    return ctx ? ctx->output_buffer : 0;
}

TextureID bloom_compute_get_threshold(BloomComputeContext* ctx) {
    return ctx ? ctx->threshold_buffer : 0;
}

TextureID bloom_compute_get_blurred(BloomComputeContext* ctx) {
    return ctx ? ctx->blur_buffers[1] : 0;
}

void bloom_compute_resize(BloomComputeContext* ctx, u32 new_width, u32 new_height) {
    if (!ctx || !ctx->initialized) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Resizing bloom compute context to %ux%u", new_width, new_height);
    
    ctx->width = new_width;
    ctx->height = new_height;
    
    // Recreate textures with new dimensions
    TextureDesc desc = {
        .width = new_width,
        .height = new_height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
        .min_filter = FILTER_LINEAR,
        .mag_filter = FILTER_LINEAR
    };
    
    // Destroy old textures
    texture_destroy(ctx->threshold_buffer);
    texture_destroy(ctx->blur_buffers[0]);
    texture_destroy(ctx->blur_buffers[1]);
    texture_destroy(ctx->output_buffer);
    
    // Create new textures
    ctx->threshold_buffer = texture_create(&desc);
    ctx->blur_buffers[0] = texture_create(&desc);
    ctx->blur_buffers[1] = texture_create(&desc);
    ctx->output_buffer = texture_create(&desc);
    
    // Update texel size in uniforms
    ctx->blur_uniforms.texel_size = (vec2){1.0f / new_width, 1.0f / new_height};
    ctx->composite_uniforms.texel_size = (vec2){1.0f / new_width, 1.0f / new_height};
    
    LOG_INFO(LOG_CAT_RENDERER, "Bloom compute context resized successfully");
}
