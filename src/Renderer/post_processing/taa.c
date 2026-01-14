// Temporal Anti-Aliasing (TAA) Implementation
// Reduces aliasing through temporal reprojection and history blending
#include "taa.h"
#include "include/core/logger.h"
#include "include/rendering/texture_system.h"
#include "rendering/core/texture.h"
#include "rendering/post_processing/taa_compute.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef INVALID_TEXTURE_ID
#define INVALID_TEXTURE_ID 0
#endif

// Internal context structure that matches taa_compute expectations if needed,
// but since taa_compute handles its own context, we just wrap it.
// TAAContext is defined in taa.h as:
// typedef struct TAAContext {
//     TAASettings settings;
//     TextureID history_buffer;
//     u32 shader_program;
//     u32 frame_index;
//     bool initialized;
// } TAAContext;
//
// We need to add void* compute_ctx to it or cast.
// Since we cannot easily change taa.h struct definition without updating
// everyone, we will verify if we can repurpose a field or if taa.h needs
// update. taa.h is used by public API. I will assuming I update taa.h to
// include `void* internal_data`.

// Initialize TAA system
TAAContext *taa_create(u32 width, u32 height) {
  TAAContext *ctx = malloc(sizeof(TAAContext));
  if (!ctx) {
    LOG_ERROR("Failed to allocate TAA context");
    return NULL;
  }

  memset(ctx, 0, sizeof(TAAContext));

  // Create compute context
  TAAComputeContext *compute_ctx = taa_compute_create(width, height);
  if (!compute_ctx) {
    LOG_ERROR("Failed to create TAA compute context");
    free(ctx);
    return NULL;
  }

  // Store compute context (casting to void* or new field)
  // We will update taa.h to add `void* internal_data`
  ctx->internal_data = compute_ctx;

  // Initialize default settings
  ctx->settings.blend_factor = 0.05f;
  ctx->settings.sharpness = 0.5f;
  ctx->settings.enable_sharpening = true;
  ctx->settings.enable_jitter = true;
  ctx->settings.sample_pattern = 0;
  ctx->settings.jitter_scale = 1.0f;

  ctx->frame_index = 0;
  ctx->initialized = true;

  // Set initial history buffer ID (for debugging/reference)
  ctx->history_buffer = taa_compute_get_history(compute_ctx);

  LOG_INFO("TAA context created: %ux%u", width, height);
  return ctx;
}

// Destroy TAA system
void taa_destroy(TAAContext *ctx) {
  if (!ctx)
    return;

  if (ctx->internal_data) {
    taa_compute_destroy((TAAComputeContext *)ctx->internal_data);
  }

  free(ctx);
  LOG_INFO("TAA context destroyed");
}

// Get current jitter offset for camera (2D offset in normalized screen space
// [-0.5, 0.5])
void taa_get_jitter_offset(TAAContext *ctx, f32 *out_x, f32 *out_y) {
  if (!ctx || !out_x || !out_y)
    return;

  if (ctx->internal_data) {
    taa_compute_get_jitter_offset((TAAComputeContext *)ctx->internal_data,
                                  out_x, out_y);
  } else {
    *out_x = 0.0f;
    *out_y = 0.0f;
  }
}

// Update settings
void taa_update_settings(TAAContext *ctx, const TAASettings *settings) {
  if (!ctx || !settings)
    return;

  memcpy(&ctx->settings, settings, sizeof(TAASettings));

  if (ctx->internal_data) {
    // Map TAASettings to TAAComputeSettings
    TAAComputeSettings compute_settings;
    compute_settings.blend_factor = settings->blend_factor;
    compute_settings.sharpness = settings->sharpness;
    compute_settings.enable_sharpening = settings->enable_sharpening;
    compute_settings.enable_jitter = settings->enable_jitter;
    compute_settings.sample_pattern = settings->sample_pattern;
    compute_settings.jitter_scale = settings->jitter_scale;

    taa_compute_update_settings((TAAComputeContext *)ctx->internal_data,
                                &compute_settings);
  }

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
  if (!data || !data->ctx || !data->ctx->internal_data)
    return;

  // Get physical texture handles from render graph
  TextureID scene_color_tex = rg_ctx_get_texture(ctx, data->scene_color);
  TextureID velocity_tex = rg_ctx_get_texture(ctx, data->velocity_buffer);
  TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

  // Note: data->history_input is handled internally by taa_compute,
  // but we pass it for RG dependencies.
  // However, taa_compute assumes IT manages the history texture swapping.
  // So we just give it the inputs it needs.
  // Wait, taa_compute_process signature:
  // void taa_compute_process(TAAComputeContext *ctx, TextureID current_frame,
  // TextureID velocity, TextureID depth); It doesn't take OUTPUT texture ID. It
  // writes to its INTERNAL output. But RG allocated an OUTPUT texture for us in
  // `data->output`! Conflict: RG expects to allocate the output resource.
  // taa_compute expects to own the output resource.
  // Resolution:
  // We should probably modify taa_compute to accept an external output texture?
  // OR, we just BLIT the result from taa_compute internal output to RG output?
  // OR, we ignore RG output and use taa_compute output?
  // If we ignore RG output, subsequent passes reading it will get... garbage?
  // Unless we update RG output to point to taa_compute output?
  // Accessing internal output: `taa_compute_get_output`.
  // Correct way: taa_compute_process updates its internal state.
  // Then we copy result to RG output? Or utilize it.
  //
  // Actually, taa_compute writes to `ctx->output_buffer`.
  // If we can tell RG "Here is the output texture ID" instead of RG creating
  // it... `rg_create_texture` creates a VIRTUAL resource backed by a PHYSICAL
  // texture from pool. `rg_import_texture` wraps an EXISTING physical texture.
  // CORRECT APPROACH:
  // IMPORT taa_compute's output buffer as the WRITABLE resource.
  // But taa_compute swaps buffers. Next frame output becomes history.
  // `taa_add_to_graph` must handle this.
  // `taa_execute_pass` just calls process.

  // Since taa_compute handles dispatch, we just call it.
  // We pass 0 for depth if not available.
  taa_compute_process((TAAComputeContext *)data->ctx->internal_data,
                      scene_color_tex, velocity_tex, INVALID_TEXTURE_ID);

  LOG_DEBUG("TAA shader executed for frame %u", data->ctx->frame_index);
}

// Add TAA pass to render graph
RGResourceHandle taa_add_to_graph(RenderGraph *rg, TAAContext *ctx,
                                  RGResourceHandle scene_color,
                                  RGResourceHandle velocity_buffer) {
  if (!rg || !ctx || !ctx->internal_data) {
    LOG_ERROR("Invalid render graph or TAA context");
    return RG_INVALID_RESOURCE;
  }

  TAAComputeContext *compute_ctx = (TAAComputeContext *)ctx->internal_data;

  // Get current internal history and output buffers
  TextureID history_id = taa_compute_get_history(compute_ctx);
  TextureID output_id = taa_compute_get_output(compute_ctx);

  // Import them into RG
  // Use unique names to avoid collisions if multiple TAA passes (unlikely)
  RGResourceHandle h_history =
      rg_import_texture(rg, history_id, "TAA_History_Internal");
  RGResourceHandle h_output =
      rg_import_texture(rg, output_id, "TAA_Output_Internal");

  // Prepare pass data
  TAAPassData *pass_data = malloc(sizeof(TAAPassData));
  if (!pass_data) {
    LOG_ERROR("Failed to allocate TAA pass data");
    return RG_INVALID_RESOURCE;
  }

  pass_data->ctx = ctx;
  pass_data->scene_color = scene_color;
  pass_data->velocity_buffer = velocity_buffer;
  pass_data->history_input = h_history;
  pass_data->output = h_output;

  // Add pass to render graph
  RGPassDesc pass_desc = {.name = "TAA_Compute",
                          .execute = taa_execute_pass,
                          .user_data = pass_data,
                          .queue_type = RG_QUEUE_COMPUTE_ASYNC,
                          .priority = 100};
  RGPassHandle pass = rg_add_pass(rg, &pass_desc);

  // Declare resource dependencies
  rg_pass_read(rg, pass, scene_color);
  rg_pass_read(rg, pass, velocity_buffer);
  rg_pass_read(rg, pass, h_history); // Read from previous history
  rg_pass_write(rg, pass, h_output); // Write to next history (output)

  ctx->frame_index++;
  // Update main context history buffer reference
  ctx->history_buffer = output_id;

  LOG_DEBUG("TAA pass added to graph (frame %u)", ctx->frame_index);
  return h_output;
}
