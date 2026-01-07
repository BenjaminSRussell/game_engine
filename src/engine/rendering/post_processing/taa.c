// TAA Implementation - Integrates with Render Graph
#include "rendering/post_processing/taa.h"
#include "core/logger.h"
#include "rendering/shader.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// Halton sequence for jitter pattern
static f32 halton_sequence(u32 index, u32 base) {
    f32 result = 0.0f;
    f32 f = 1.0f / (f32)base;
    u32 i = index;
    
    while (i > 0) {
        result += f * (f32)(i % base);
        i /= base;
        f /= (f32)base;
    }
    
    return result;
}

TAAContext *taa_create(u32 width, u32 height) {
    TAAContext *ctx = (TAAContext *)calloc(1, sizeof(TAAContext));
    if (!ctx) {
        LOG_ERROR("Failed to allocate TAA context");
        return NULL;
    }
    
    // Default settings
    ctx->settings.blend_factor = 0.05f;
    ctx->settings.sharpness = 0.5f;
    ctx->settings.enable_sharpening = true;
    ctx->settings.enable_jitter = true;
    ctx->settings.sample_pattern = 0;
    ctx->settings.jitter_scale = 1.0f;
    ctx->frame_index = 0;
    
    // Create history buffer
    glGenTextures(1, &ctx->history_buffer.id);
    glBindTexture(GL_TEXTURE_2D, ctx->history_buffer.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Load TAA shader
    Shader taa_shader;
    if (!shader_create_from_files(&taa_shader,
                                   "assets/shaders/post_processing/taa.vert",
                                   "assets/shaders/post_processing/taa.frag")) {
        LOG_ERROR("Failed to load TAA shader");
        glDeleteTextures(1, &ctx->history_buffer.id);
        free(ctx);
        return NULL;
    }
    
    ctx->shader_program = taa_shader.program_id;
    ctx->initialized = true;
    
    LOG_INFO("TAA initialized (%ux%u)", width, height);
    return ctx;
}

void taa_destroy(TAAContext *ctx) {
    if (!ctx) return;
    
    if (ctx->history_buffer.id) {
        glDeleteTextures(1, &ctx->history_buffer.id);
    }
    
    if (ctx->shader_program) {
        glDeleteProgram(ctx->shader_program);
    }
    
    free(ctx);
}

// TAA pass execution callback
typedef struct TAAPassData {
    TAAContext *ctx;
    RGResourceHandle current_frame;
    RGResourceHandle velocity_buffer;
    RGResourceHandle output;
} TAAPassData;

static void taa_execute_pass(RGPassContext *pass_ctx, void *user_data) {
    TAAPassData *data = (TAAPassData *)user_data;
    TAAContext *ctx = data->ctx;
    
    // Get physical textures
    TextureID current = rg_ctx_get_texture(pass_ctx, data->current_frame);
    TextureID velocity = rg_ctx_get_texture(pass_ctx, data->velocity_buffer);
    TextureID output = rg_ctx_get_texture(pass_ctx, data->output);
    
    // Bind shader
    glUseProgram(ctx->shader_program);
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, current.id);
    glUniform1i(glGetUniformLocation(ctx->shader_program, "u_current_frame"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx->history_buffer.id);
    glUniform1i(glGetUniformLocation(ctx->shader_program, "u_history_frame"), 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, velocity.id);
    glUniform1i(glGetUniformLocation(ctx->shader_program, "u_velocity"), 2);
    
    // Upload parameters
    glUniform1f(glGetUniformLocation(ctx->shader_program, "u_blend_factor"), 
                ctx->settings.blend_factor);
    glUniform1f(glGetUniformLocation(ctx->shader_program, "u_sharpness"), 
                ctx->settings.sharpness);
    glUniform1i(glGetUniformLocation(ctx->shader_program, "u_enable_sharpening"),
                ctx->settings.enable_sharpening ? 1 : 0);
    
    // Render fullscreen quad (would use proper quad from renderer)
    // For now, render to output FBO
    // This is simplified - real implementation would use renderer's quad
    
    // Copy output to history for next frame
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output.id);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 0, 0, 1920, 1080, 0);
    
    glBindTexture(GL_TEXTURE_2D, ctx->history_buffer.id);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 0, 0, 1920, 1080, 0);
    
    ctx->frame_index++;
}

RGResourceHandle taa_add_to_graph(RenderGraph *rg,
                                   TAAContext *ctx,
                                   RGResourceHandle scene_color,
                                   RGResourceHandle velocity_buffer) {
    if (!rg || !ctx) return (RGResourceHandle){0};
    
    // Create output texture
    RGTextureDesc output_desc = {
        .width = 1920,  // Would come from context
        .height = 1080,
        .format = TEXFMT_RGBA16F,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .name = "TAA_Output"
    };
    RGResourceHandle output = rg_create_texture(rg, &output_desc);
    
    // Setup pass data
    TAAPassData *data = (TAAPassData *)malloc(sizeof(TAAPassData));
    data->ctx = ctx;
    data->current_frame = scene_color;
    data->velocity_buffer = velocity_buffer;
    data->output = output;
    
    // Add pass to graph
    RGPassDesc pass_desc = {
        .name = "TAA",
        .execute = taa_execute_pass,
        .user_data = data
    };
    RGPassHandle pass = rg_add_pass(rg, &pass_desc);
    
    // Declare dependencies
    rg_pass_read(rg, pass, scene_color);
    rg_pass_read(rg, pass, velocity_buffer);
    rg_pass_write(rg, pass, output);
    
    LOG_DEBUG("Added TAA pass to render graph");
    return output;
}

void taa_get_jitter_offset(TAAContext *ctx, f32 *out_x, f32 *out_y) {
    if (!ctx || !ctx->settings.enable_jitter) {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }
    
    // Generate Halton sequence jitter
    u32 index = (ctx->frame_index % 16) + 1;  // 16-sample pattern
    *out_x = (halton_sequence(index, 2) - 0.5f) * ctx->settings.jitter_scale;
    *out_y = (halton_sequence(index, 3) - 0.5f) * ctx->settings.jitter_scale;
}

void taa_update_settings(TAAContext *ctx, const TAASettings *settings) {
    if (!ctx || !settings) return;
    ctx->settings = *settings;
}
