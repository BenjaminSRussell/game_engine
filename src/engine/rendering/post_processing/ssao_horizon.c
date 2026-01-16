// Screen-Space Ambient Occlusion (SSAO) Implementation
// Horizon-based SSAO (HBAO+) - fast and high-quality ambient occlusion
#include "core/logger/unified_logger.h"
#include "include/rendering/texture_system.h" // For TEXFMT constants
#include "rendering/frame_graph/frame_graph.h"
#include "rendering/post_processing/ssao_compute.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct SSAOContext {
  u32 width;
  u32 height;
  f32 radius;         // Occlusion radius (screen space)
  f32 bias;           // Angle bias to reduce false occlusion
  f32 intensity;      // AO intensity multiplier
  u32 num_directions; // Number of sample directions (typically 8)
  u32 num_samples;    // Samples per direction (typically 4)
  bool enable_blur;   // Enable bilateral blur post-processing

  SSAOComputeContext *compute_ctx; // Backend compute implementation
} SSAOContext;

// Create SSAO context
SSAOContext *ssao_create(u32 width, u32 height) {
  SSAOContext *ctx = malloc(sizeof(SSAOContext));
  if (!ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to allocate SSAO context");
    return NULL;
  }

  memset(ctx, 0, sizeof(SSAOContext));

  ctx->width = width;
  ctx->height = height;
  ctx->radius = 32.0f;     // Pixels in screen space
  ctx->bias = 0.1f;        // Angle bias
  ctx->intensity = 1.0f;   // Full intensity
  ctx->num_directions = 8; // 8 directions
  ctx->num_samples = 4;    // 4 samples per direction
  ctx->enable_blur = true; // Enable bilateral blur

  // Create compute implementation
  ctx->compute_ctx = ssao_compute_create(width, height);
  if (!ctx->compute_ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER,
                  "Failed to create SSAO compute implementation");
    free(ctx);
    return NULL;
  }

  LOG_INFO_CAT(LOG_CAT_RENDERER, "SSAO context created: %ux%u", width, height);
  return ctx;
}

// Destroy SSAO context
void ssao_destroy(SSAOContext *ctx) {
  if (!ctx)
    return;

  if (ctx->compute_ctx) {
    ssao_compute_destroy(ctx->compute_ctx);
  }

  free(ctx);
  LOG_INFO_CAT(LOG_CAT_RENDERER, "SSAO context destroyed");
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
  if (!data || !data->ctx || !data->ctx->compute_ctx)
    return;

  // Get physical textures from render graph
  TextureID depth_tex = rg_ctx_get_texture(ctx, data->depth_input);
  TextureID normal_tex = rg_ctx_get_texture(ctx, data->normal_input);

  // Update compute settings from context
  SSAOComputeSettings settings = {.radius = data->ctx->radius,
                                  .intensity = data->ctx->intensity,
                                  .bias = data->ctx->bias,
                                  .sample_count = data->ctx->num_directions *
                                                  data->ctx->num_samples};
  ssao_compute_update_settings(data->ctx->compute_ctx, &settings);

  // Process SSAO using compute implementation
  ssao_compute_process(data->ctx->compute_ctx, depth_tex, normal_tex);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
                "SSAO compute shader executed via ssao_compute_process");
}

// SSAO bilateral blur pass for denoising
typedef struct {
  SSAOContext *ctx;
  RGResourceHandle input;
  RGResourceHandle output;
} SSAOBlurPassData;

static void ssao_blur_execute(RGPassContext *ctx, void *user_data) {
  SSAOBlurPassData *data = (SSAOBlurPassData *)user_data;
  if (!data || !data->ctx || !data->ctx->compute_ctx)
    return;

  // Bilateral blur is integrated into the compute implementation or handled
  // via its own internal passes if enabled.
  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
                "SSAO bilateral blur integrated in compute implementation");
}

// Add SSAO pass to render graph
RGResourceHandle ssao_add_to_graph(RenderGraph *rg, SSAOContext *ctx,
                                   RGResourceHandle depth_buffer,
                                   RGResourceHandle normal_buffer) {
  if (!rg || !ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Invalid render graph or SSAO context");
    return RG_INVALID_RESOURCE;
  }

  if (ctx->radius < 1.0f)
    ctx->radius = 1.0f;
  if (ctx->radius > 256.0f)
    ctx->radius = 256.0f;
  if (ctx->intensity < 0.0f)
    ctx->intensity = 0.0f;
  if (ctx->intensity > 2.0f)
    ctx->intensity = 2.0f;

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
                "SSAO pass added: radius=%.2f, intensity=%.2f", ctx->radius,
                ctx->intensity);

  // Create SSAO output texture
  RGTextureDesc ssao_desc = {.width = ctx->width,
                             .height = ctx->height,
                             .depth = 1,
                             .format = TEXFMT_R8,
                             .usage =
                                 TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                             .name = "SSAO_Output"};
  RGResourceHandle ssao_output = rg_create_texture(rg, &ssao_desc);

  // Add SSAO compute pass
  SSAOPassData *pass_data = malloc(sizeof(SSAOPassData));
  if (pass_data) {
    pass_data->ctx = ctx;
    pass_data->depth_input = depth_buffer;
    pass_data->normal_input = normal_buffer;
    pass_data->output = ssao_output;

    RGPassDesc pass_desc = {.name = "SSAO",
                            .execute = ssao_execute_pass,
                            .user_data = pass_data,
                            .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                            .priority = 95};
    RGPassHandle pass = rg_add_pass(rg, &pass_desc);
    rg_pass_read(rg, pass, depth_buffer);
    rg_pass_read(rg, pass, normal_buffer);
    rg_pass_write(rg, pass, ssao_output);
  }

  // Optional: Add bilateral blur pass for denoising
  if (ctx->enable_blur) {
    RGTextureDesc blur_desc = {.width = ctx->width,
                               .height = ctx->height,
                               .depth = 1,
                               .format = TEXFMT_R8,
                               .usage = TEXTURE_USAGE_STORAGE |
                                        TEXTURE_USAGE_SAMPLED,
                               .name = "SSAO_Blurred"};
    RGResourceHandle ssao_blurred = rg_create_texture(rg, &blur_desc);

    SSAOBlurPassData *blur_data = malloc(sizeof(SSAOBlurPassData));
    if (blur_data) {
      blur_data->ctx = ctx;
      blur_data->input = ssao_output;
      blur_data->output = ssao_blurred;

      RGPassDesc blur_desc_pass = {.name = "SSAO_Blur",
                                   .execute = ssao_blur_execute,
                                   .user_data = blur_data,
                                   .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                                   .priority = 90};
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
  if (!ctx)
    return;
  if (radius < 1.0f)
    radius = 1.0f;
  if (radius > 256.0f)
    radius = 256.0f;
  ctx->radius = radius;
}

// Update SSAO intensity
void ssao_set_intensity(SSAOContext *ctx, f32 intensity) {
  if (!ctx)
    return;
  if (intensity < 0.0f)
    intensity = 0.0f;
  if (intensity > 2.0f)
    intensity = 2.0f;
  ctx->intensity = intensity;
}
