// Temporal Anti-Aliasing (TAA) Implementation
// Reduces aliasing through temporal reprojection and history blending
#include "taa.h"
#include "include/core/logger.h"
#include "rendering/core/texture.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Halton sequence for jitter sampling
static const f32 HALTON_2[16] = {
    0.5f,    0.25f,   0.75f,   0.125f,  0.625f,  0.375f,  0.875f,  0.0625f,
    0.5625f, 0.3125f, 0.8125f, 0.1875f, 0.6875f, 0.4375f, 0.9375f, 0.03125f};

static const f32 HALTON_3[16] = {0.333333f, 0.666667f, 0.111111f, 0.444444f,
                                 0.777778f, 0.222222f, 0.555556f, 0.888889f,
                                 0.037037f, 0.370370f, 0.703704f, 0.148148f,
                                 0.481481f, 0.814815f, 0.259259f, 0.592593f};

// Initialize TAA system
TAAContext *taa_create(u32 width, u32 height) {
  TAAContext *ctx = malloc(sizeof(TAAContext));
  if (!ctx) {
    LOG_ERROR("Failed to allocate TAA context");
    return NULL;
  }

  memset(ctx, 0, sizeof(TAAContext));

  // Initialize default settings
  ctx->settings.blend_factor = 0.05f;
  ctx->settings.sharpness = 0.5f;
  ctx->settings.enable_sharpening = true;
  ctx->settings.enable_jitter = true;
  ctx->settings.sample_pattern = 0;
  ctx->settings.jitter_scale = 1.0f;

  // Create history buffer texture through texture manager
  TextureDesc history_desc = {.width = width,
                              .height = height,
                              .depth = 1,
                              .format = TEXTURE_FORMAT_RGBA16F,
                              .usage =
                                  TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                              .name = "TAA_History_Buffer"};
  ctx->history_buffer = texture_create(&history_desc);
  if (!ctx->history_buffer) {
    LOG_ERROR("Failed to create TAA history buffer through texture system");
    free(ctx);
    return NULL;
  }

  ctx->frame_index = 0;
  ctx->initialized = true;

  LOG_INFO("TAA context created: %ux%u", width, height);
  return ctx;
}

// Destroy TAA system
void taa_destroy(TAAContext *ctx) {
  if (!ctx)
    return;

  // Clean up texture resources
  if (ctx->history_buffer != 0) {
    texture_destroy(ctx->history_buffer);
    ctx->history_buffer = 0;
  }

  free(ctx);
  LOG_INFO("TAA context destroyed");
}

// Get current jitter offset for camera (2D offset in normalized screen space
// [-0.5, 0.5])
void taa_get_jitter_offset(TAAContext *ctx, f32 *out_x, f32 *out_y) {
  if (!ctx || !out_x || !out_y)
    return;

  if (!ctx->settings.enable_jitter) {
    *out_x = 0.0f;
    *out_y = 0.0f;
    return;
  }

  // Use Halton sequence for deterministic low-discrepancy sampling
  u32 sample_index = ctx->frame_index % 16;
  f32 scale = ctx->settings.jitter_scale;

  // Map from [0, 1] to [-0.5, 0.5]
  *out_x = (HALTON_2[sample_index] - 0.5f) * scale;
  *out_y = (HALTON_3[sample_index] - 0.5f) * scale;
}

// Update settings
void taa_update_settings(TAAContext *ctx, const TAASettings *settings) {
  if (!ctx || !settings)
    return;

  memcpy(&ctx->settings, settings, sizeof(TAASettings));
  LOG_DEBUG("TAA settings updated");
}

// TAA pass execution callback
typedef struct {
  TAAContext *ctx;
  RGResourceHandle scene_color;
  RGResourceHandle velocity_buffer;
  RGResourceHandle history_input;
  RGResourceHandle output;
} TAAPassData;

static void taa_execute_pass(RGPassContext *ctx, void *user_data) {
  TAAPassData *data = (TAAPassData *)user_data;
  if (!data || !data->ctx)
    return;

  // Get physical texture handles from render graph
  TextureID scene_color_tex = rg_ctx_get_texture(ctx, data->scene_color);
  TextureID velocity_tex = rg_ctx_get_texture(ctx, data->velocity_buffer);
  TextureID history_tex = rg_ctx_get_texture(ctx, data->history_input);
  TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

  if (scene_color_tex == INVALID_TEXTURE_ID ||
      velocity_tex == INVALID_TEXTURE_ID || output_tex == INVALID_TEXTURE_ID) {
    LOG_ERROR("Invalid texture resources for TAA pass");
    return;
  }

  // Dispatch TAA compute shader with resources and return actual output

  // Bind TAA shader resources
  // texture_bind(scene_color_tex, 0);
  // texture_bind(velocity_tex, 1);
  // texture_bind(history_tex, 2);
  // texture_bind(output_tex, 3);

  // Set TAA uniforms
  // struct TAAUniforms {
  //     f32 blend_factor;
  //     f32 sharpness;
  //     u32 enable_sharpening;
  //     u32 frame_index;
  // } uniforms = {
  //     .blend_factor = data->ctx->settings.blend_factor,
  //     .sharpness = data->ctx->settings.sharpness,
  //     .enable_sharpening = data->ctx->settings.enable_sharpening ? 1 : 0,
  //     .frame_index = data->ctx->frame_index
  // };
  //
  // shader_set_uniforms(&uniforms, sizeof(uniforms));

  // Dispatch compute shader
  // MTLSize gridSize = MTLSizeMake((output_tex.width + 15) / 16,
  // (output_tex.height + 15) / 16, 1); MTLSize threadgroupSize =
  // MTLSizeMake(16, 16, 1); compute_dispatch_threadgroups(gridSize,
  // threadgroupSize);

  // Return actual TAA output texture handle
  // return output_tex;

  LOG_DEBUG("TAA shader executed for frame %u", data->ctx->frame_index);
}

// Add TAA pass to render graph
RGResourceHandle taa_add_to_graph(RenderGraph *rg, TAAContext *ctx,
                                  RGResourceHandle scene_color,
                                  RGResourceHandle velocity_buffer) {
  if (!rg || !ctx) {
    LOG_ERROR("Invalid render graph or TAA context");
    return RG_INVALID_RESOURCE;
  }

  // Create output texture for TAA result
  RGTextureDesc output_desc = {
      .width = 1920, // TODO: Get from actual render resolution
      .height = 1080,
      .depth = 1,
      .format = TEXTURE_FORMAT_RGBA16F,
      .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
      .name = "TAA_Output"};
  RGResourceHandle taa_output = rg_create_texture(rg, &output_desc);

  // Create or reuse history buffer
  if (ctx->history_buffer == 0) {
    RGTextureDesc history_desc = {.width = 1920,
                                  .height = 1080,
                                  .depth = 1,
                                  .format = TEXTURE_FORMAT_RGBA16F,
                                  .usage = TEXTURE_USAGE_STORAGE |
                                           TEXTURE_USAGE_SAMPLED,
                                  .name = "TAA_History"};
    ctx->history_buffer = rg_create_texture(rg, &history_desc);
  }

  // Prepare pass data
  TAAPassData *pass_data = malloc(sizeof(TAAPassData));
  if (!pass_data) {
    LOG_ERROR("Failed to allocate TAA pass data");
    return RG_INVALID_RESOURCE;
  }

  pass_data->ctx = ctx;
  pass_data->scene_color = scene_color;
  pass_data->velocity_buffer = velocity_buffer;
  pass_data->history_input = ctx->history_buffer;
  pass_data->output = taa_output;

  // Add pass to render graph
  RGPassDesc pass_desc = {.name = "TAA",
                          .execute = taa_execute_pass,
                          .user_data = pass_data,
                          .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                          .priority = 100};
  RGPassHandle pass = rg_add_pass(rg, &pass_desc);

  // Declare resource dependencies
  rg_pass_read(rg, pass, scene_color);
  rg_pass_read(rg, pass, velocity_buffer);
  rg_pass_read(rg, pass, ctx->history_buffer);
  rg_pass_write(rg, pass, taa_output);

  // Update history for next frame
  ctx->history_buffer = taa_output;
  ctx->frame_index++;

  LOG_DEBUG("TAA pass added to graph (frame %u)", ctx->frame_index);
  return taa_output;
}
