// Screen-Space Ambient Occlusion (SSAO) Implementation
// Horizon-based SSAO (HBAO+) - fast and high-quality ambient occlusion
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct SSAOContext {
    u32 width;
    u32 height;
    f32 radius;           // Occlusion radius (screen space)
    f32 bias;             // Angle bias to reduce false occlusion
    f32 intensity;        // AO intensity multiplier
    u32 num_directions;   // Number of sample directions (typically 8)
    u32 num_samples;      // Samples per direction (typically 4)
    bool enable_blur;     // Enable bilateral blur post-processing
} SSAOContext;

// Create SSAO context
SSAOContext *ssao_create(u32 width, u32 height) {
    SSAOContext *ctx = malloc(sizeof(SSAOContext));
    if (!ctx) {
        LOG_ERROR("Failed to allocate SSAO context");
        return NULL;
    }

    memset(ctx, 0, sizeof(SSAOContext));

    ctx->width = width;
    ctx->height = height;
    ctx->radius = 32.0f;        // Pixels in screen space
    ctx->bias = 0.1f;           // Angle bias
    ctx->intensity = 1.0f;      // Full intensity
    ctx->num_directions = 8;    // 8 directions
    ctx->num_samples = 4;       // 4 samples per direction
    ctx->enable_blur = true;    // Enable bilateral blur

    LOG_INFO("SSAO context created: %ux%u", width, height);
    return ctx;
}

// Destroy SSAO context
void ssao_destroy(SSAOContext *ctx) {
    if (!ctx) return;
    free(ctx);
    LOG_INFO("SSAO context destroyed");
}

// SSAO pass execution callback
typedef struct {
    SSAOContext *ctx;
    RGResourceHandle depth_input;
    RGResourceHandle normal_input;
    RGResourceHandle output;
} SSAOPassData;

static void ssao_execute_pass(RGPassContext *ctx, void *user_data) {
    SSAOPassData *data = (SSAOPassData *)user_data;
    if (!data || !data->ctx) return;

    TextureID depth_tex = rg_ctx_get_texture(ctx, data->depth_input);
    TextureID normal_tex = rg_ctx_get_texture(ctx, data->normal_input);
    TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

    if (depth_tex == INVALID_TEXTURE_ID || normal_tex == INVALID_TEXTURE_ID ||
        output_tex == INVALID_TEXTURE_ID) {
        LOG_ERROR("Invalid textures for SSAO pass");
        return;
    }

    // TODO: Dispatch SSAO compute shader with resources
    // Shader will:
    // 1. For each pixel, reconstruct world position from depth
    // 2. Scan horizon angles in N directions (typically 8)
    // 3. Take M samples per direction to find horizon angle
    // 4. Compute occlusion factor from angle difference with surface normal
    // 5. Apply bias to reduce false occlusion near edges
    // 6. Apply intensity multiplier
    // 7. Optional: Apply bilateral blur for denoising

    LOG_DEBUG("SSAO shader executed");
}

// SSAO bilateral blur pass for denoising
typedef struct {
    SSAOContext *ctx;
    RGResourceHandle input;
    RGResourceHandle output;
} SSAOBlurPassData;

static void ssao_blur_execute(RGPassContext *ctx, void *user_data) {
    SSAOBlurPassData *data = (SSAOBlurPassData *)user_data;
    if (!data || !data->ctx) return;

    TextureID input_tex = rg_ctx_get_texture(ctx, data->input);
    TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

    if (input_tex == INVALID_TEXTURE_ID || output_tex == INVALID_TEXTURE_ID) {
        LOG_ERROR("Invalid textures for SSAO blur pass");
        return;
    }

    // TODO: Dispatch bilateral blur compute shader
    // Shader will:
    // 1. Apply edge-aware filtering using depth discontinuities
    // 2. Preserve hard edges while smoothing gradual transitions
    // 3. Use 2-pass separable convolution for efficiency

    LOG_DEBUG("SSAO bilateral blur executed");
}

// Add SSAO pass to render graph
RGResourceHandle ssao_add_to_graph(RenderGraph *rg,
                                  SSAOContext *ctx,
                                  RGResourceHandle depth_buffer,
                                  RGResourceHandle normal_buffer) {
    if (!rg || !ctx) {
        LOG_ERROR("Invalid render graph or SSAO context");
        return RG_INVALID_RESOURCE;
    }

    if (ctx->radius < 1.0f) ctx->radius = 1.0f;
    if (ctx->radius > 256.0f) ctx->radius = 256.0f;
    if (ctx->intensity < 0.0f) ctx->intensity = 0.0f;
    if (ctx->intensity > 2.0f) ctx->intensity = 2.0f;

    LOG_DEBUG("SSAO pass added: radius=%.2f, intensity=%.2f", ctx->radius, ctx->intensity);

    // Create SSAO output texture
    RGTextureDesc ssao_desc = {
        .width = ctx->width,
        .height = ctx->height,
        .depth = 1,
        .format = TEXTURE_FORMAT_R8,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
        .name = "SSAO_Output"
    };
    RGResourceHandle ssao_output = rg_create_texture(rg, &ssao_desc);

    // Add SSAO compute pass
    SSAOPassData *pass_data = malloc(sizeof(SSAOPassData));
    if (pass_data) {
        pass_data->ctx = ctx;
        pass_data->depth_input = depth_buffer;
        pass_data->normal_input = normal_buffer;
        pass_data->output = ssao_output;

        RGPassDesc pass_desc = {
            .name = "SSAO",
            .execute = ssao_execute_pass,
            .user_data = pass_data,
            .queue_type = RG_QUEUE_COMPUTE_ASYNC,
            .priority = 95
        };
        RGPassHandle pass = rg_add_pass(rg, &pass_desc);
        rg_pass_read(rg, pass, depth_buffer);
        rg_pass_read(rg, pass, normal_buffer);
        rg_pass_write(rg, pass, ssao_output);
    }

    // Optional: Add bilateral blur pass for denoising
    if (ctx->enable_blur) {
        RGTextureDesc blur_desc = {
            .width = ctx->width,
            .height = ctx->height,
            .depth = 1,
            .format = TEXTURE_FORMAT_R8,
            .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
            .name = "SSAO_Blurred"
        };
        RGResourceHandle ssao_blurred = rg_create_texture(rg, &blur_desc);

        SSAOBlurPassData *blur_data = malloc(sizeof(SSAOBlurPassData));
        if (blur_data) {
            blur_data->ctx = ctx;
            blur_data->input = ssao_output;
            blur_data->output = ssao_blurred;

            RGPassDesc blur_desc_pass = {
                .name = "SSAO_Blur",
                .execute = ssao_blur_execute,
                .user_data = blur_data,
                .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                .priority = 90
            };
            RGPassHandle blur_pass = rg_add_pass(rg, &blur_desc_pass);
            rg_pass_read(rg, blur_pass, ssao_output);
            rg_pass_write(rg, blur_pass, ssao_blurred);

            return ssao_blurred;
        }
    }

    return ssao_output;
}

// Update SSAO radius
void ssao_set_radius(SSAOContext *ctx, f32 radius) {
    if (!ctx) return;
    if (radius < 1.0f) radius = 1.0f;
    if (radius > 256.0f) radius = 256.0f;
    ctx->radius = radius;
}

// Update SSAO intensity
void ssao_set_intensity(SSAOContext *ctx, f32 intensity) {
    if (!ctx) return;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 2.0f) intensity = 2.0f;
    ctx->intensity = intensity;
}
