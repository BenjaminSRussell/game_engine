// src/engine/rendering/post_processing/taa_compute.c
//
// Purpose: Compute shader-based Temporal Anti-Aliasing implementation
// Provides high-performance TAA using GPU compute shaders

#include "rendering/post_processing/taa_compute.h"
#include "core/logger/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "rendering/core/texture.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct TAAComputeContext {
  TAAComputeSettings settings;

  // GPU resources
  TextureID history_buffer;
  TextureID velocity_buffer;
  TextureID depth_buffer;
  TextureID output_buffer;

  // Compute shader
  u32 compute_shader;

  // Jitter pattern
  u32 frame_index;
  f32 halton_sequence[8][2]; // 8-frame Halton sequence

  // Uniform buffer
  struct {
    vec2 jitter_offset;
    f32 blend_factor;
    f32 variance_clamp;
    vec2 texel_size;
    u32 frame_index;
    u32 padding[3];
  } uniforms;

  bool initialized;
} TAAComputeContext;

// ============================================================================
// HALTON SEQUENCE GENERATION
// ============================================================================

static void generate_halton_sequence(f32 sequence[8][2]) {
  // Halton(2,3) sequence for 8 frames
  const f32 halton2_8[8] = {0.0f,   0.5f,   0.25f,  0.75f,
                            0.125f, 0.625f, 0.375f, 0.875f};
  const f32 halton3_8[8] = {0.0f,   0.333f, 0.667f, 0.111f,
                            0.444f, 0.778f, 0.222f, 0.556f};

  for (int i = 0; i < 8; i++) {
    sequence[i][0] = halton2_8[i] - 0.5f; // Center around 0
    sequence[i][1] = halton3_8[i] - 0.5f;
  }
}

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

TAAComputeContext *taa_compute_create(u32 width, u32 height) {
  LOG_INFO(LOG_CAT_RENDERER, "Creating TAA compute context (%ux%u)", width,
           height);

  TAAComputeContext *ctx = MALLOC_PERSISTENT(sizeof(TAAComputeContext));
  if (!ctx) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate TAA compute context");
    return NULL;
  }

  memset(ctx, 0, sizeof(TAAComputeContext));

  // Initialize default settings
  ctx->settings.blend_factor = 0.05f;
  ctx->settings.sharpness = 0.5f;
  ctx->settings.enable_sharpening = true;
  ctx->settings.enable_jitter = true;
  ctx->settings.sample_pattern = 0;
  ctx->settings.jitter_scale = 1.0f;

  // Generate Halton sequence
  generate_halton_sequence(ctx->halton_sequence);

  // Create textures
  TextureDesc desc = {.width = width,
                      .height = height,
                      .format = TEXTURE_FORMAT_RGBA16F,
                      .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                      .min_filter = FILTER_LINEAR,
                      .mag_filter = FILTER_LINEAR};

  ctx->history_buffer = texture_create(&desc);
  ctx->output_buffer = texture_create(&desc);

  if (!ctx->history_buffer || !ctx->output_buffer) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to create TAA textures");
    taa_compute_destroy(ctx);
    return NULL;
  }

  // Load compute shader
  ctx->compute_shader =
      shader_load_compute("shaders/post_processing/taa_compute.comp");
  if (!ctx->compute_shader) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to load TAA compute shader");
    taa_compute_destroy(ctx);
    return NULL;
  }

  // Initialize uniforms
  ctx->uniforms.blend_factor = ctx->settings.blend_factor;
  ctx->uniforms.variance_clamp = 0.1f;
  ctx->uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
  ctx->uniforms.frame_index = 0;

  ctx->initialized = true;

  LOG_INFO(LOG_CAT_RENDERER, "TAA compute context created successfully");
  return ctx;
}

void taa_compute_destroy(TAAComputeContext *ctx) {
  if (!ctx)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Destroying TAA compute context");

  if (ctx->history_buffer) {
    texture_destroy(ctx->history_buffer);
  }

  if (ctx->output_buffer) {
    texture_destroy(ctx->output_buffer);
  }

  if (ctx->compute_shader) {
    shader_destroy(ctx->compute_shader);
  }

  FREE(ctx);
}

// ============================================================================
// MAIN PROCESSING
// ============================================================================

void taa_compute_process(TAAComputeContext *ctx, TextureID current_frame,
                         TextureID velocity_buffer, TextureID depth_buffer) {
  if (!ctx || !ctx->initialized) {
    LOG_WARN(LOG_CAT_RENDERER, "TAA compute context not initialized");
    return;
  }

  // Update jitter offset
  if (ctx->settings.enable_jitter) {
    u32 pattern_index = ctx->frame_index % 8;
    ctx->uniforms.jitter_offset.x =
        ctx->halton_sequence[pattern_index][0] * ctx->settings.jitter_scale;
    ctx->uniforms.jitter_offset.y =
        ctx->halton_sequence[pattern_index][1] * ctx->settings.jitter_scale;
  } else {
    ctx->uniforms.jitter_offset = (vec2){0.0f, 0.0f};
  }

  // Update uniforms
  ctx->uniforms.blend_factor = ctx->settings.blend_factor;
  ctx->uniforms.frame_index = ctx->frame_index;
  ctx->velocity_buffer = velocity_buffer;
  ctx->depth_buffer = depth_buffer;

  // Bind resources for compute shader
  shader_bind_compute(ctx->compute_shader);

  // Bind textures
  texture_bind_compute(current_frame, 0);
  texture_bind_compute(ctx->history_buffer, 1);
  texture_bind_compute(velocity_buffer, 2);
  texture_bind_compute(depth_buffer, 3);

  // Bind output image
  texture_bind_image_compute(ctx->output_buffer, 0);

  // Set uniforms
  shader_set_uniform_compute(ctx->compute_shader, "params", &ctx->uniforms,
                             sizeof(ctx->uniforms));

  // Dispatch compute shader
  u32 work_groups_x = (ctx->output_buffer->width + 15) / 16;
  u32 work_groups_y = (ctx->output_buffer->height + 15) / 16;

  shader_dispatch_compute(ctx->compute_shader, work_groups_x, work_groups_y, 1);

  // Memory barrier to ensure writes are complete
  shader_memory_barrier_compute();

  // Swap history buffer (ping-pong)
  TextureID temp = ctx->history_buffer;
  ctx->history_buffer = ctx->output_buffer;
  ctx->output_buffer = temp;

  ctx->frame_index++;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void taa_compute_get_jitter_offset(TAAComputeContext *ctx, f32 *out_x,
                                   f32 *out_y) {
  if (!ctx || !out_x || !out_y)
    return;

  if (ctx->settings.enable_jitter) {
    u32 pattern_index = ctx->frame_index % 8;
    *out_x =
        ctx->halton_sequence[pattern_index][0] * ctx->settings.jitter_scale;
    *out_y =
        ctx->halton_sequence[pattern_index][1] * ctx->settings.jitter_scale;
  } else {
    *out_x = 0.0f;
    *out_y = 0.0f;
  }
}

void taa_compute_update_settings(TAAComputeContext *ctx,
                                 const TAAComputeSettings *settings) {
  if (!ctx || !settings)
    return;

  ctx->settings = *settings;

  // Update uniforms that changed
  ctx->uniforms.blend_factor = settings->blend_factor;
  ctx->uniforms.variance_clamp = 0.1f; // Could be configurable
}

TextureID taa_compute_get_output(TAAComputeContext *ctx) {
  return ctx ? ctx->output_buffer : 0;
}

TextureID taa_compute_get_history(TAAComputeContext *ctx) {
  return ctx ? ctx->history_buffer : 0;
}

void taa_compute_reset(TAAComputeContext *ctx) {
  if (!ctx)
    return;

  ctx->frame_index = 0;

  // Clear history buffer
  if (ctx->history_buffer) {
    texture_clear(ctx->history_buffer, (vec4){0.0f, 0.0f, 0.0f, 0.0f});
  }

  LOG_INFO(LOG_CAT_RENDERER, "TAA compute context reset");
}
