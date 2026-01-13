// src/engine/rendering/post_processing/ssao_compute.c
//
// Purpose: Compute shader-based SSAO implementation
// Provides high-performance SSAO using GPU compute shaders

#include "rendering/post_processing/ssao_compute.h"
#include "rendering/core/texture.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct SSAOComputeContext {
    SSAOComputeSettings settings;
    
    // GPU resources
    TextureID depth_buffer;
    TextureID normal_buffer;
    TextureID output_buffer;
    TextureID noise_texture;
    TextureID depth_hierarchy;  // Mip chain for hierarchical sampling
    
    // Compute shader
    u32 compute_shader;
    
    // Screen dimensions
    u32 width;
    u32 height;
    
    // Uniform buffer
    struct {
        vec2 texel_size;
        f32 radius;
        f32 intensity;
        f32 bias;
        u32 sample_count;
        vec2 projection_params; // near, far
        u32 padding[3];
    } uniforms;
    
    bool initialized;
} SSAOComputeContext;

// ============================================================================
// NOISE TEXTURE GENERATION
// ============================================================================

static TextureID generate_noise_texture(u32 size) {
    TextureDesc desc = {
        .width = size,
        .height = size,
        .format = TEXTURE_FORMAT_RGBA8,
        .usage = TEXTURE_USAGE_SAMPLED,
        .min_filter = FILTER_NEAREST,
        .mag_filter = FILTER_NEAREST,
        .wrap_u = WRAP_REPEAT,
        .wrap_v = WRAP_REPEAT
    };
    
    TextureID noise = texture_create(&desc);
    if (!noise) return 0;
    
    // Generate random rotation vectors
    u8* data = MALLOC_TEMP(size * size * 4);
    if (!data) {
        texture_destroy(noise);
        return 0;
    }
    
    for (u32 y = 0; y < size; y++) {
        for (u32 x = 0; x < size; x++) {
            u32 index = (y * size + x) * 4;
            
            // Random values in [0, 255]
            data[index + 0] = rand() % 256;     // R
            data[index + 1] = rand() % 256;     // G  
            data[index + 2] = rand() % 256;     // B
            data[index + 3] = 255;              // A
        }
    }
    
    texture_upload_data(noise, data);
    FREE(data);
    
    return noise;
}

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

SSAOComputeContext* ssao_compute_create(u32 width, u32 height) {
    LOG_INFO(LOG_CAT_RENDERER, "Creating SSAO compute context (%ux%u)", width, height);
    
    SSAOComputeContext* ctx = MALLOC_PERSISTENT(sizeof(SSAOComputeContext));
    if (!ctx) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate SSAO compute context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(SSAOComputeContext));
    
    // Initialize default settings
    ctx->settings.radius = 0.5f;
    ctx->settings.intensity = 1.0f;
    ctx->settings.bias = 0.025f;
    ctx->settings.sample_count = 16;
    
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
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSAO output buffer");
        ssao_compute_destroy(ctx);
        return NULL;
    }
    
    // Create noise texture
    ctx->noise_texture = generate_noise_texture(4);
    if (!ctx->noise_texture) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSAO noise texture");
        ssao_compute_destroy(ctx);
        return NULL;
    }
    
    // Create depth hierarchy (mip chain)
    desc.format = TEXTURE_FORMAT_R16F;
    desc.usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_TRANSFER_SRC | TEXTURE_USAGE_TRANSFER_DST;
    desc.mip_levels = 6; // 6 mip levels for hierarchical sampling
    
    ctx->depth_hierarchy = texture_create(&desc);
    if (!ctx->depth_hierarchy) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSAO depth hierarchy");
        ssao_compute_destroy(ctx);
        return NULL;
    }
    
    // Load compute shader
    ctx->compute_shader = shader_load_compute("shaders/post_processing/ssao_compute.comp");
    if (!ctx->compute_shader) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to load SSAO compute shader");
        ssao_compute_destroy(ctx);
        return NULL;
    }
    
    // Initialize uniforms
    ctx->uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
    ctx->uniforms.radius = ctx->settings.radius;
    ctx->uniforms.intensity = ctx->settings.intensity;
    ctx->uniforms.bias = ctx->settings.bias;
    ctx->uniforms.sample_count = ctx->settings.sample_count;
    ctx->uniforms.projection_params = (vec2){0.1f, 100.0f}; // Default near/far
    
    ctx->initialized = true;
    
    LOG_INFO(LOG_CAT_RENDERER, "SSAO compute context created successfully");
    return ctx;
}

void ssao_compute_destroy(SSAOComputeContext* ctx) {
    if (!ctx) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Destroying SSAO compute context");
    
    if (ctx->output_buffer) {
        texture_destroy(ctx->output_buffer);
    }
    
    if (ctx->noise_texture) {
        texture_destroy(ctx->noise_texture);
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

void ssao_compute_process(SSAOComputeContext* ctx, 
                         TextureID depth_buffer,
                         TextureID normal_buffer) {
    if (!ctx || !ctx->initialized) {
        LOG_WARN(LOG_CAT_RENDERER, "SSAO compute context not initialized");
        return;
    }
    
    // Update uniforms
    ctx->uniforms.radius = ctx->settings.radius;
    ctx->uniforms.intensity = ctx->settings.intensity;
    ctx->uniforms.bias = ctx->settings.bias;
    ctx->uniforms.sample_count = ctx->settings.sample_count;
    
    ctx->depth_buffer = depth_buffer;
    ctx->normal_buffer = normal_buffer;
    
    // Generate depth hierarchy mip chain
    texture_generate_mipmaps(depth_buffer);
    texture_copy_to_texture(depth_buffer, ctx->depth_hierarchy);
    
    // Bind resources for compute shader
    shader_bind_compute(ctx->compute_shader);
    
    // Bind textures
    texture_bind_compute(depth_buffer, 0);
    texture_bind_compute(normal_buffer, 1);
    texture_bind_compute(ctx->noise_texture, 2);
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

void ssao_compute_update_settings(SSAOComputeContext* ctx, const SSAOComputeSettings* settings) {
    if (!ctx || !settings) return;
    
    ctx->settings = *settings;
    
    // Update uniforms that changed
    ctx->uniforms.radius = settings->radius;
    ctx->uniforms.intensity = settings->intensity;
    ctx->uniforms.bias = settings->bias;
    ctx->uniforms.sample_count = settings->sample_count;
}

TextureID ssao_compute_get_output(SSAOComputeContext* ctx) {
    return ctx ? ctx->output_buffer : 0;
}

void ssao_compute_set_projection_params(SSAOComputeContext* ctx, f32 near_plane, f32 far_plane) {
    if (!ctx) return;
    
    ctx->uniforms.projection_params = (vec2){near_plane, far_plane};
}

void ssao_compute_resize(SSAOComputeContext* ctx, u32 new_width, u32 new_height) {
    if (!ctx || !ctx->initialized) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Resizing SSAO compute context to %ux%u", new_width, new_height);
    
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
    
    LOG_INFO(LOG_CAT_RENDERER, "SSAO compute context resized successfully");
}
