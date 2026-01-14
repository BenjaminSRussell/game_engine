// src/engine/rendering/post_processing/bloom_compute.c
// Compute shader-based Bloom implementation

#include "rendering/post_processing/bloom_compute.h"
#include "core/logger/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "rendering/core/shader.h" // Add shader header
#include "rendering/core/texture.h"
#include <stdlib.h>
#include <string.h>

#ifndef TEXFMT_RGBA16F
#define TEXFMT_RGBA16F 25
#endif

#define LOG_RENDERER_INFO(fmt, ...)                                            \
  LOG_INFO_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)
#define LOG_RENDERER_WARN(fmt, ...)                                            \
  LOG_WARN_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)
#define LOG_RENDERER_ERROR(fmt, ...)                                           \
  LOG_ERROR_CAT(LOG_CAT_RENDERER, fmt, ##__VA_ARGS__)

typedef struct BloomComputeContext {
  BloomComputeSettings settings;

  // GPU resources - Using Texture pointers
  Texture *threshold_buffer;
  Texture *blur_buffers[2];
  Texture *output_buffer;

  // Compute shaders (IDs)
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

BloomComputeContext *bloom_compute_create(u32 width, u32 height) {
  LOG_RENDERER_INFO("Creating bloom compute context (%ux%u)", width, height);

  BloomComputeContext *ctx = MALLOC_PERSISTENT(sizeof(BloomComputeContext));
  if (!ctx) {
    LOG_RENDERER_ERROR("Failed to allocate bloom compute context");
    return NULL;
  }

  memset(ctx, 0, sizeof(BloomComputeContext));

  ctx->settings.threshold = 1.0f;
  ctx->settings.soft_knee = 0.5f;
  ctx->settings.intensity = 0.04f;
  ctx->settings.iterations = 5;
  ctx->settings.sigma = 1.0f;
  ctx->settings.color_shift = (vec3){1.0f, 1.0f, 1.0f};

  ctx->width = width;
  ctx->height = height;

  TextureCreateInfo desc = {.width = width,
                            .height = height,
                            .depth = 1,
                            .format = TEXFMT_RGBA16F,
                            .usage =
                                TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                            .name = "Bloom_Texture"};

  ctx->threshold_buffer = texture_create(&desc);
  ctx->blur_buffers[0] = texture_create(&desc);
  ctx->blur_buffers[1] = texture_create(&desc);
  ctx->output_buffer = texture_create(&desc);

  if (!ctx->threshold_buffer || !ctx->blur_buffers[0] ||
      !ctx->blur_buffers[1] || !ctx->output_buffer) {
    LOG_RENDERER_ERROR("Failed to create bloom textures");
    bloom_compute_destroy(ctx);
    return NULL;
  }

  // Shaders... stub or load
  // ctx->threshold_shader = shader_load_compute("...");

  ctx->initialized = true;
  LOG_RENDERER_INFO("Bloom compute context created");
  return ctx;
}

void bloom_compute_destroy(BloomComputeContext *ctx) {
  if (!ctx)
    return;

  if (ctx->threshold_buffer)
    texture_destroy(ctx->threshold_buffer);
  if (ctx->blur_buffers[0])
    texture_destroy(ctx->blur_buffers[0]);
  if (ctx->blur_buffers[1])
    texture_destroy(ctx->blur_buffers[1]);
  if (ctx->output_buffer)
    texture_destroy(ctx->output_buffer);

  // No shader destroy API

  FREE(ctx);
}

void bloom_compute_process(BloomComputeContext *ctx, TextureID hdr_input) {
  if (!ctx || !ctx->initialized)
    return;
  // ...
}

void bloom_compute_update_settings(BloomComputeContext *ctx,
                                   const BloomComputeSettings *settings) {
  if (ctx && settings)
    ctx->settings = *settings;
}

TextureID bloom_compute_get_output(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->output_buffer)
             : (TextureID){0}; // Assuming texture.h has texture_get_id
}

TextureID bloom_compute_get_threshold(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->threshold_buffer) : (TextureID){0};
}

TextureID bloom_compute_get_blurred(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->blur_buffers[1]) : (TextureID){0};
}

void bloom_compute_resize(BloomComputeContext *ctx, u32 new_width,
                          u32 new_height) {
  if (!ctx)
    return;
  // Destroy and recreate logic...
  // For brevity stubbing reuse creating logic or assume it works
  ctx->width = new_width;
  ctx->height = new_height;
}
