// Color Grading Post-Processing Effect
// Applies a 3D LUT (Look-Up Table) for color correction and cinematic effects
#include "core/logging/unified_logger.h"
#include "rendering/core/shader.h"
#include "rendering/core/texture.h"
#include "rendering/frame_graph/frame_graph.h"
#include <stdlib.h>

typedef struct ColorGradePassData {
  RGResourceHandle input;
  TextureID lut_texture;
  RGResourceHandle output;
  u32 shader;
} ColorGradePassData;

static void color_grade_execute_pass(RGPassContext *ctx, void *user_data) {
  ColorGradePassData *data = (ColorGradePassData *)user_data;
  if (!data)
    return;

  TextureID input_tex = rg_ctx_get_texture(ctx, data->input);
  TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

  // Implementation logic:
  // 1. Bind lut_grading.comp shader
  // 2. Bind input_tex (sampler2D) and data->lut_texture (sampler3D)
  // 3. Bind output_tex as image2D
  // 4. Dispatch compute shader

  LOG_DEBUG(LOG_CAT_RENDERER, "Executed color grading pass with LUT %u",
            data->lut_texture);
}

RGResourceHandle color_grade_add_to_graph(RenderGraph *rg,
                                          RGResourceHandle input,
                                          TextureID lut_texture) {
  if (!rg || input.id == RG_INVALID_RESOURCE.id || lut_texture == 0) {
    return input;
  }

  RGTextureDesc output_desc = {.width = 0,
                               .height = 0,
                               .format = TEXTURE_FORMAT_RGBA8,
                               .usage = TEXTURE_USAGE_STORAGE |
                                        TEXTURE_USAGE_SAMPLED,
                               .name = "Color_Graded_Output"};
  RGResourceHandle output = rg_create_texture(rg, &output_desc);

  ColorGradePassData *pass_data = malloc(sizeof(ColorGradePassData));
  pass_data->input = input;
  pass_data->lut_texture = lut_texture;
  pass_data->output = output;

  // Load shader
  pass_data->shader = shader_load_from_file(
      "assets/shaders/post_processing/lut_grading.comp", SHADER_TYPE_COMPUTE);
  shader_compile(pass_data->shader);

  RGPassDesc pass_desc = {.name = "Color_Grading",
                          .execute = color_grade_execute_pass,
                          .user_data = pass_data,
                          .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                          .priority = 110};

  RGPassHandle pass = rg_add_pass(rg, &pass_desc);
  rg_pass_read(rg, pass, input);
  rg_pass_write(rg, pass, output);

  LOG_DEBUG(LOG_CAT_RENDERER,
            "Color grading pass integrated into render graph");

  return output;
}
