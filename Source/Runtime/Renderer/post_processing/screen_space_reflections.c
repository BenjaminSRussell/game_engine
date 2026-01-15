// Screen-Space Reflections (SSR) Implementation
// Efficient screen-space reflection algorithm with hierarchical ray marching
#include "core/logger/unified_logger.h"
#include "rendering/frame_graph/frame_graph.h"
#include "rendering/post_processing/ssr_compute.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct SSRContext {
  u32 width;
  u32 height;
  f32 max_distance; // Screen space pixel radius for reflections
  f32 thickness;    // Surface thickness for intersection
  u32 max_steps;    // Max ray march steps
  f32 stride;       // Ray march initial stride

  SSRComputeContext *compute_ctx; // Backend compute implementation
} SSRContext;

// Create SSR context
SSRContext *ssr_create(u32 width, u32 height) {
  SSRContext *ctx = malloc(sizeof(SSRContext));
  if (!ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to allocate SSR context");
    return NULL;
  }

  memset(ctx, 0, sizeof(SSRContext));

  ctx->width = width;
  ctx->height = height;
  ctx->max_distance = 64.0f;
  ctx->thickness = 0.5f;
  ctx->max_steps = 32;
  ctx->stride = 2.0f;

  // Create compute implementation
  ctx->compute_ctx = ssr_compute_create(width, height);
  if (!ctx->compute_ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to create SSR compute implementation");
    free(ctx);
    return NULL;
  }

  LOG_INFO_CAT(LOG_CAT_RENDERER, "SSR context created: %ux%u", width, height);
  return ctx;
}

// Destroy SSR context
void ssr_destroy(SSRContext *ctx) {
  if (!ctx)
    return;

  if (ctx->compute_ctx) {
    ssr_compute_destroy(ctx->compute_ctx);
  }

  free(ctx);
  LOG_INFO_CAT(LOG_CAT_RENDERER, "SSR context destroyed");
}

// SSR pass execution callback
typedef struct {
  SSRContext *ctx;
  RGResourceHandle scene_color;
  RGResourceHandle normal_roughness;
  RGResourceHandle depth_buffer;
  RGResourceHandle output;
} SSRPassData;

static void ssr_execute_pass(RGPassContext *ctx, void *user_data) {
  SSRPassData *data = (SSRPassData *)user_data;
  if (!data || !data->ctx || !data->ctx->compute_ctx)
    return;

  // Get physical textures from render graph
  TextureID scene_tex = rg_ctx_get_texture(ctx, data->scene_color);
  TextureID normal_roughness_tex =
      rg_ctx_get_texture(ctx, data->normal_roughness);
  TextureID depth_tex = rg_ctx_get_texture(ctx, data->depth_buffer);

  // Update compute settings from context
  SSRComputeSettings settings = {.max_distance = data->ctx->max_distance,
                                 .thickness = data->ctx->thickness,
                                 .max_steps = data->ctx->max_steps,
                                 .stride = data->ctx->stride,
                                 .fade_distance = 25.0f};
  ssr_compute_update_settings(data->ctx->compute_ctx, &settings);

  // Process SSR using compute implementation
  ssr_compute_process(data->ctx->compute_ctx, scene_tex, normal_roughness_tex,
                      depth_tex);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
            "SSR compute shader executed via ssr_compute_process");
}

// Add SSR pass to render graph
RGResourceHandle ssr_add_to_graph(RenderGraph *rg, SSRContext *ctx,
                                  RGResourceHandle scene_color,
                                  RGResourceHandle normal_roughness,
                                  RGResourceHandle depth_buffer) {
  // Create SSR output texture resource in graph
  RGTextureDesc ssr_desc = {.width = ctx->width,
                            .height = ctx->height,
                            .format = TEXFMT_RGBA16F,
                            .usage =
                                TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                            .name = "SSR_Output"};
  RGResourceHandle ssr_output = rg_create_texture(rg, &ssr_desc);

  // Setup pass data
  SSRPassData *pass_data = malloc(sizeof(SSRPassData));
  if (pass_data) {
    pass_data->ctx = ctx;
    pass_data->scene_color = scene_color;
    pass_data->normal_roughness = normal_roughness;
    pass_data->depth_buffer = depth_buffer;
    pass_data->output = ssr_output;

    RGPassDesc pass_desc = {.name = "SSR",
                            .execute = ssr_execute_pass,
                            .user_data = pass_data,
                            .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                            .priority = 85};

    RGPassHandle pass = rg_add_pass(rg, &pass_desc);
    rg_pass_read(rg, pass, scene_color);
    rg_pass_read(rg, pass, normal_roughness);
    rg_pass_read(rg, pass, depth_buffer);
    rg_pass_write(rg, pass, ssr_output);

    LOG_DEBUG_CAT(LOG_CAT_RENDERER, "SSR pass integrated into render graph");
  }

  return ssr_output;
}

// Update SSR parameters
void ssr_set_max_distance(SSRContext *ctx, f32 distance) {
  if (!ctx)
    return;
  if (distance < 16.0f)
    distance = 16.0f;
  if (distance > 512.0f)
    distance = 512.0f;
  ctx->max_distance = distance;
}

void ssr_set_thickness(SSRContext *ctx, f32 thickness) {
  if (!ctx)
    return;
  if (thickness < 0.01f)
    thickness = 0.01f;
  if (thickness > 2.0f)
    thickness = 2.0f;
  ctx->thickness = thickness;
}

void ssr_set_max_steps(SSRContext *ctx, u32 steps) {
  if (!ctx)
    return;
  if (steps < 32)
    steps = 32;
  if (steps > 256)
    steps = 256;
  ctx->max_steps = steps;
}
