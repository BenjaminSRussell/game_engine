// src/engine/rendering/post_processing/ssao_compute.c
#include "rendering/post_processing/ssao_compute.h"
#include "core/logger/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "rendering/core/shader.h"
#include "rendering/core/texture.h"
#include <stdlib.h>
#include <string.h>

#ifndef TEXFMT_RGBA16F
#define TEXFMT_RGBA16F 25
#endif
#ifndef TEXTURE_FORMAT_R16F
#define TEXTURE_FORMAT_R16F 15
#endif
#ifndef TEXTURE_FORMAT_RGBA8
#define TEXTURE_FORMAT_RGBA8 20 // Placeholder
#endif

// Helper macros for logging
#define LOG_RENDERER_INFO(fmt, ...)                                            \
  LOG_INFO_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)
#define LOG_RENDERER_WARN(fmt, ...)                                            \
  LOG_WARN_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)
#define LOG_RENDERER_ERROR(fmt, ...)                                           \
  LOG_ERROR_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)

typedef struct SSAOComputeContext {
  SSAOComputeSettings settings;

  TextureID depth_buffer;
  TextureID normal_buffer;

  Texture *output_texture;
  Texture *noise_texture;
  Texture *depth_hierarchy_texture;

  u32 compute_shader;
  u32 width;
  u32 height;

  struct {
    vec2 texel_size;
    f32 radius;
    f32 intensity;
    f32 bias;
    u32 sample_count;
    vec2 projection_params;
    u32 padding[3];
  } uniforms;

  bool initialized;
} SSAOComputeContext;

static Texture *generate_noise_texture(u32 size) {
  u32 noise_width = size;
  u32 noise_height = size;

  TextureCreateInfo desc = {.width = noise_width,
                            .height = noise_height,
                            .format = TEXTURE_FORMAT_RGBA8,
                            .usage = TEXTURE_USAGE_SAMPLED,
                            .mip_levels = 1,
                            .sample_count = 1,
                            .name = "SSAO_Noise"};

  Texture *noise = texture_create(&desc);
  if (!noise)
    return NULL;
  return noise;
}

SSAOComputeContext *ssao_compute_create(u32 width, u32 height) {
  LOG_RENDERER_INFO("Creating SSAO compute context (%ux%u)", width, height);

  SSAOComputeContext *ctx = MALLOC_PERSISTENT(sizeof(SSAOComputeContext));
  if (!ctx) {
    LOG_RENDERER_ERROR("Failed to allocate SSAO compute context");
    return NULL;
  }

  memset(ctx, 0, sizeof(SSAOComputeContext));

  ctx->settings.radius = 0.5f;
  ctx->settings.intensity = 1.0f;
  ctx->settings.bias = 0.025f;
  ctx->settings.sample_count = 16;

  ctx->width = width;
  ctx->height = height;

  TextureCreateInfo desc = {.width = width,
                            .height = height,
                            .depth = 1,
                            .format = TEXFMT_RGBA16F,
                            .usage =
                                TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                            .mip_levels = 1,
                            .sample_count = 1,
                            .name = "SSAO_Output"};

  ctx->output_texture = texture_create(&desc);
  if (!ctx->output_texture) {
    LOG_RENDERER_ERROR("Failed to create SSAO output buffer");
    ssao_compute_destroy(ctx);
    return NULL;
  }

  ctx->noise_texture = generate_noise_texture(4);
  if (!ctx->noise_texture) {
    LOG_RENDERER_ERROR("Failed to create SSAO noise texture");
    ssao_compute_destroy(ctx);
    return NULL;
  }

  desc.format = TEXTURE_FORMAT_R16F;
  desc.usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_TRANSFER_SRC |
               TEXTURE_USAGE_TRANSFER_DST;
  desc.mip_levels = 6;
  desc.name = "SSAO_Depth_Hierarchy";

  ctx->depth_hierarchy_texture = texture_create(&desc);
  if (!ctx->depth_hierarchy_texture) {
    LOG_RENDERER_ERROR("Failed to create SSAO depth hierarchy");
    ssao_compute_destroy(ctx);
    return NULL;
  }

  ctx->compute_shader =
      shader_load_compute("shaders/post_processing/ssao_compute.comp");
  if (!ctx->compute_shader) {
    LOG_RENDERER_ERROR("Failed to load SSAO compute shader");
    ssao_compute_destroy(ctx);
    return NULL;
  }

  ctx->uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
  ctx->uniforms.radius = ctx->settings.radius;
  ctx->uniforms.intensity = ctx->settings.intensity;
  ctx->uniforms.bias = ctx->settings.bias;
  ctx->uniforms.sample_count = ctx->settings.sample_count;
  ctx->uniforms.projection_params = (vec2){0.1f, 100.0f};

  ctx->initialized = true;

  LOG_RENDERER_INFO("SSAO compute context created successfully");
  return ctx;
}

void ssao_compute_destroy(SSAOComputeContext *ctx) {
  if (!ctx)
    return;

  LOG_RENDERER_INFO("Destroying SSAO compute context");

  if (ctx->output_texture)
    texture_destroy(ctx->output_texture);
  if (ctx->noise_texture)
    texture_destroy(ctx->noise_texture);
  if (ctx->depth_hierarchy_texture)
    texture_destroy(ctx->depth_hierarchy_texture);

  FREE(ctx);
}

void ssao_compute_process(SSAOComputeContext *ctx, TextureID depth_buffer,
                          TextureID normal_buffer) {
  if (!ctx || !ctx->initialized) {
    LOG_RENDERER_WARN("SSAO compute context not initialized");
    return;
  }
  // Stubbed implementation
}

void ssao_compute_update_settings(SSAOComputeContext *ctx,
                                  const SSAOComputeSettings *settings) {
  if (!ctx || !settings)
    return;
  ctx->settings = *settings;
}

TextureID ssao_compute_get_output(SSAOComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->output_texture) : 0;
}

void ssao_compute_set_projection_params(SSAOComputeContext *ctx, f32 near_plane,
                                        f32 far_plane) {
  if (ctx)
    ctx->uniforms.projection_params = (vec2){near_plane, far_plane};
}

void ssao_compute_resize(SSAOComputeContext *ctx, u32 new_width,
                         u32 new_height) {
  if (!ctx || !ctx->initialized)
    return;
  LOG_RENDERER_INFO("Resizing SSAO compute context to %ux%u", new_width,
                    new_height);
}
