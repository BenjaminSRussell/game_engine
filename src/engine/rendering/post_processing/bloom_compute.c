// src/engine/rendering/post_processing/bloom_compute.c
//
// Purpose: Compute shader-based Bloom implementation
// Provides high-performance bloom using GPU compute shaders

#include "rendering/post_processing/bloom_compute.h"
#include "core/logger/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "include/rendering/texture_system.h" // For TEXFMT constants
#include "rendering/core/texture.h"
#include <stdlib.h>
#include <string.h>

// Stub declarations for shader system (temporary)
// In a real implementation these would be in a header like
// rendering/core/shader.h
typedef u32 ShaderID;
static ShaderID shader_load_compute(const char *path) { return 1; }
static void shader_destroy(ShaderID shader) {}
static void shader_bind_compute(ShaderID shader) {}
static void shader_set_uniform_compute(ShaderID shader, const char *name,
                                       void *data, u32 size) {}
static void shader_dispatch_compute(ShaderID shader, u32 x, u32 y, u32 z) {}
static void shader_memory_barrier_compute(void) {}
static void texture_bind_compute(TextureID texture, u32 slot) {}
static void texture_bind_image_compute(TextureID texture, u32 slot) {}

// Internal structure to manage bloom state
typedef struct BloomComputeContext {
  BloomComputeSettings settings;

  // GPU resources - We store Texture* to manage lifecycle
  Texture *threshold_texture;
  Texture *blur_textures[2]; // Two buffers for ping-pong blur
  Texture *output_texture;

  // Compute shaders
  ShaderID threshold_shader;
  ShaderID blur_shader;
  ShaderID composite_shader;

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

BloomComputeContext *bloom_compute_create(u32 width, u32 height) {
  LOG_INFO_CAT(LOG_CAT_RENDERER, "Creating bloom compute context (%ux%u)",
               width, height);

  BloomComputeContext *ctx = MALLOC_PERSISTENT(sizeof(BloomComputeContext));
  if (!ctx) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to allocate bloom compute context");
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
  // Use TextureCreateInfo matching texture.h
  TextureCreateInfo desc = {.width = width,
                            .height = height,
                            .depth = 1,
                            .format = TEXFMT_RGBA16F, // Fixed constant
                            .usage =
                                TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
                            .mip_levels = 1,
                            .sample_count = 1,
                            .name = "Bloom Texture"};

  ctx->threshold_texture = texture_create(&desc);
  ctx->blur_textures[0] = texture_create(&desc);
  ctx->blur_textures[1] = texture_create(&desc);
  ctx->output_texture = texture_create(&desc);

  if (!ctx->threshold_texture || !ctx->blur_textures[0] ||
      !ctx->blur_textures[1] || !ctx->output_texture) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to create bloom textures");
    bloom_compute_destroy(ctx);
    return NULL;
  }

  // Load compute shaders
  ctx->threshold_shader =
      shader_load_compute("shaders/post_processing/bloom_threshold.comp");
  ctx->blur_shader =
      shader_load_compute("shaders/post_processing/bloom_blur.comp");
  ctx->composite_shader =
      shader_load_compute("shaders/post_processing/bloom_composite.comp");

  if (!ctx->threshold_shader || !ctx->blur_shader || !ctx->composite_shader) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to load bloom compute shaders");
    bloom_compute_destroy(ctx);
    return NULL;
  }

  // Initialize uniforms
  ctx->threshold_uniforms.threshold = ctx->settings.threshold;
  ctx->threshold_uniforms.soft_knee = ctx->settings.soft_knee;
  ctx->threshold_uniforms.intensity = ctx->settings.intensity;

  ctx->blur_uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};
  ctx->blur_uniforms.direction = (vec2){1.0f, 0.0f}; // Horizontal first
  ctx->blur_uniforms.sigma = ctx->settings.sigma;

  ctx->composite_uniforms.intensity = ctx->settings.intensity;
  ctx->composite_uniforms.dirt_intensity = 0.0f; // Could be configurable
  ctx->composite_uniforms.color_shift = ctx->settings.color_shift;
  ctx->composite_uniforms.texel_size = (vec2){1.0f / width, 1.0f / height};

  ctx->initialized = true;

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Bloom compute context created successfully");
  return ctx;
}

void bloom_compute_destroy(BloomComputeContext *ctx) {
  if (!ctx)
    return;

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Destroying bloom compute context");

  if (ctx->threshold_texture) {
    texture_destroy(ctx->threshold_texture);
  }

  if (ctx->blur_textures[0]) {
    texture_destroy(ctx->blur_textures[0]);
  }

  if (ctx->blur_textures[1]) {
    texture_destroy(ctx->blur_textures[1]);
  }

  if (ctx->output_texture) {
    texture_destroy(ctx->output_texture);
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

void bloom_compute_process(BloomComputeContext *ctx, TextureID hdr_input) {
  if (!ctx || !ctx->initialized) {
    LOG_WARN_CAT(LOG_CAT_RENDERER, "Bloom compute context not initialized");
    return;
  }

  // Update uniforms
  ctx->threshold_uniforms.threshold = ctx->settings.threshold;
  ctx->threshold_uniforms.soft_knee = ctx->settings.soft_knee;
  ctx->threshold_uniforms.intensity = ctx->settings.intensity;

  ctx->blur_uniforms.sigma = ctx->settings.sigma;

  ctx->composite_uniforms.intensity = ctx->settings.intensity;
  ctx->composite_uniforms.color_shift = ctx->settings.color_shift;

  // Helpers to get IDs
  TextureID threshold_id = texture_get_id(ctx->threshold_texture);
  TextureID blur0_id = texture_get_id(ctx->blur_textures[0]);
  TextureID blur1_id = texture_get_id(ctx->blur_textures[1]);
  TextureID output_id = texture_get_id(ctx->output_texture);

  // ============================================================================
  // STEP 1: Threshold extraction
  // ============================================================================
  shader_bind_compute(ctx->threshold_shader);

  texture_bind_compute(hdr_input, 0);
  texture_bind_image_compute(threshold_id, 0);

  shader_set_uniform_compute(ctx->threshold_shader, "params",
                             &ctx->threshold_uniforms,
                             sizeof(ctx->threshold_uniforms));

  u32 work_groups_x = (ctx->width + 15) / 16;
  u32 work_groups_y = (ctx->height + 15) / 16;

  shader_dispatch_compute(ctx->threshold_shader, work_groups_x, work_groups_y,
                          1);
  shader_memory_barrier_compute();

  // ============================================================================
  // STEP 2: Iterative blur (horizontal + vertical passes)
  // ============================================================================
  TextureID current_input = threshold_id;

  for (u32 i = 0; i < ctx->settings.iterations; i++) {
    // Horizontal pass
    ctx->blur_uniforms.direction = (vec2){1.0f, 0.0f};

    shader_bind_compute(ctx->blur_shader);
    texture_bind_compute(current_input, 0);
    texture_bind_image_compute(blur0_id, 0);

    shader_set_uniform_compute(ctx->blur_shader, "params", &ctx->blur_uniforms,
                               sizeof(ctx->blur_uniforms));

    shader_dispatch_compute(ctx->blur_shader, work_groups_x, work_groups_y, 1);
    shader_memory_barrier_compute();

    // Vertical pass
    ctx->blur_uniforms.direction = (vec2){0.0f, 1.0f};

    shader_bind_compute(ctx->blur_shader);
    texture_bind_compute(blur0_id, 0);
    texture_bind_image_compute(blur1_id, 0);

    shader_set_uniform_compute(ctx->blur_shader, "params", &ctx->blur_uniforms,
                               sizeof(ctx->blur_uniforms));

    shader_dispatch_compute(ctx->blur_shader, work_groups_x, work_groups_y, 1);
    shader_memory_barrier_compute();

    // Use result as input for next iteration
    current_input = blur1_id;
  }

  // ============================================================================
  // STEP 3: Composite with original scene
  // ============================================================================
  shader_bind_compute(ctx->composite_shader);

  texture_bind_compute(hdr_input, 0);
  texture_bind_compute(current_input, 1);
  texture_bind_image_compute(output_id, 0);

  shader_set_uniform_compute(ctx->composite_shader, "params",
                             &ctx->composite_uniforms,
                             sizeof(ctx->composite_uniforms));

  shader_dispatch_compute(ctx->composite_shader, work_groups_x, work_groups_y,
                          1);
  shader_memory_barrier_compute();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void bloom_compute_update_settings(BloomComputeContext *ctx,
                                   const BloomComputeSettings *settings) {
  if (!ctx || !settings)
    return;

  ctx->settings = *settings;

  // Update uniforms that changed
  ctx->threshold_uniforms.threshold = settings->threshold;
  ctx->threshold_uniforms.soft_knee = settings->soft_knee;
  ctx->threshold_uniforms.intensity = settings->intensity;

  ctx->blur_uniforms.sigma = settings->sigma;

  ctx->composite_uniforms.intensity = settings->intensity;
  ctx->composite_uniforms.color_shift = settings->color_shift;
}

TextureID bloom_compute_get_output(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->output_texture) : 0;
}

TextureID bloom_compute_get_threshold(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->threshold_texture) : 0;
}

TextureID bloom_compute_get_blurred(BloomComputeContext *ctx) {
  return ctx ? texture_get_id(ctx->blur_textures[1]) : 0;
}

void bloom_compute_resize(BloomComputeContext *ctx, u32 new_width,
                          u32 new_height) {
  if (!ctx || !ctx->initialized)
    return;

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Resizing bloom compute context to %ux%u",
               new_width, new_height);

  ctx->width = new_width;
  ctx->height = new_height;

  // Recreate textures with new dimensions
  TextureCreateInfo desc = {
      .width = new_width,
      .height = new_height,
      .depth = 1,
      .format = TEXFMT_RGBA16F,
      .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
      .mip_levels = 1,
      .sample_count = 1,
      // Assuming fix if field exists, but TextureCreateInfo in
      // texture.h didn't have filter fields.
      // Wait, texture.h TextureCreateInfo didn't show min_filter/mag_filter.
      // It had name, mip_levels, sample_count.
      // I should check if I need filters. texture.h didn't show them.
      // I will omit filters for now to match texture.h definition.
      .name = "Bloom Texture Resized"};

  // Destroy old textures
  texture_destroy(ctx->threshold_texture);
  texture_destroy(ctx->blur_textures[0]);
  texture_destroy(ctx->blur_textures[1]);
  texture_destroy(ctx->output_texture);

  // Create new textures
  ctx->threshold_texture = texture_create(&desc);
  ctx->blur_textures[0] = texture_create(&desc);
  ctx->blur_textures[1] = texture_create(&desc);
  ctx->output_texture = texture_create(&desc);

  // Update texel size in uniforms
  ctx->blur_uniforms.texel_size = (vec2){1.0f / new_width, 1.0f / new_height};
  ctx->composite_uniforms.texel_size =
      (vec2){1.0f / new_width, 1.0f / new_height};

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Bloom compute context resized successfully");
}
