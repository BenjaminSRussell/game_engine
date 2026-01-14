// Tonemapping Post-Processing Effect
// Converts HDR color space to SDR (LDR) with multiple tone curve operators
#include "core/logger/unified_logger.h"
#include "rendering/core/shader.h"
#include "rendering/core/texture.h"
#include "rendering/frame_graph/frame_graph.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEXTURE_FORMAT_RGBA8
#define TEXTURE_FORMAT_RGBA8 20
#endif

typedef struct TonemapPassData {
  RGResourceHandle input_hdr;
  RGResourceHandle output_srgb;
  u32 operator;
  f32 exposure;
  u32 shader;
} TonemapPassData;

static void tonemap_execute_pass(RGPassContext *ctx, void *user_data) {
  TonemapPassData *data = (TonemapPassData *)user_data;

  // Get physical textures from render graph
  TextureID input_tex = rg_ctx_get_texture(ctx, data->input_hdr);
  TextureID output_tex = rg_ctx_get_texture(ctx, data->output_srgb);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER, "Executed tonemapping pass (OP=%u, EXP=%.2f)",
                data->operator, data->exposure);
}

// Tone mapping operators
typedef enum TonemapOperator {
  TONEMAP_ACES = 0,     // ACES filmic tone mapping (industry standard)
  TONEMAP_FILMIC = 1,   // Uncharted 2 filmic operator
  TONEMAP_REINHARD = 2, // Reinhard tone mapping (simple)
  TONEMAP_COUNT
} TonemapOperator;

// ACES tone mapping curve (approximated)
static inline f32 aces_tonemap(f32 x) {
  f32 a = 2.51f;
  f32 b = 0.03f;
  f32 c = 2.43f;
  f32 d = 0.59f;
  f32 e = 0.14f;

  return fmax(0.0f, (x * (a * x + b)) / (x * (c * x + d) + e));
}

// Uncharted 2 filmic tone mapping
static inline f32 uncharted2_tonemap(f32 x) {
  f32 A = 0.15f;
  f32 B = 0.50f;
  f32 C = 0.10f;
  f32 D = 0.20f;
  f32 E = 0.02f;
  f32 F = 0.30f;

  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Reinhard tone mapping (simple, efficient)
static inline f32 reinhard_tonemap(f32 x) { return x / (1.0f + x); }

// Apply selected tone mapping operator
static inline f32 apply_tonemap(f32 value, TonemapOperator op) {
  switch (op) {
  case TONEMAP_ACES:
    return aces_tonemap(value);
  case TONEMAP_FILMIC:
    return uncharted2_tonemap(value);
  case TONEMAP_REINHARD:
    return reinhard_tonemap(value);
  default:
    return value; // No tone mapping
  }
}

RGResourceHandle tonemap_add_to_graph(RenderGraph *rg,
                                      RGResourceHandle hdr_color, u32 operator,
                                      f32 exposure) {
  if (!rg || hdr_color.id == RG_INVALID_RESOURCE.id) {
    return RG_INVALID_RESOURCE;
  }

  if (operator>= TONEMAP_COUNT) {
    LOG_WARN_CAT(LOG_CAT_RENDERER,
                 "Invalid tone mapping operator %u, using ACES", operator);
    operator= TONEMAP_ACES;
  }

  if (exposure < 0.1f)
    exposure = 0.1f;
  if (exposure > 4.0f)
    exposure = 4.0f;

  // Create output LDR texture resource in the graph
  RGTextureDesc output_desc = {
      .width = 0, // Inherit from context/input if supported by RG
      .height = 0,
      .format = TEXTURE_FORMAT_RGBA8,
      .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
      .name = "Tonemapped Output"};
  RGResourceHandle output = rg_create_texture(rg, &output_desc);

  // Setup pass data
  TonemapPassData *pass_data = malloc(sizeof(TonemapPassData));
  pass_data->input_hdr = hdr_color;
  pass_data->output_srgb = output;
  pass_data->operator= operator;
  pass_data->exposure = exposure;

  // Load tonemapping shader
  pass_data->shader = shader_load_from_file(
      "assets/shaders/post_processing/tonemap.comp", SHADER_TYPE_COMPUTE);
  shader_compile(pass_data->shader);

  // Add compute pass to the graph
  RGPassDesc pass_desc = {.name = "Tonemapping",
                          .execute = tonemap_execute_pass,
                          .user_data = pass_data,
                          .queue_type = RG_QUEUE_COMPUTE_ASYNC};

  RGPassHandle pass = rg_add_pass(rg, &pass_desc);

  // Declare resource dependencies
  rg_pass_read(rg, pass, hdr_color);
  rg_pass_write(rg, pass, output);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
                "Tonemapping pass integrated into render graph");

  return output;
}
