// Example Shadow Pass - Render Graph Integration
// Demonstrates how to convert a traditional pass to use the render graph

#include "renderer/frame_graph/frame_graph.h"
#include "renderer/core/command_buffer.h"
#include "core/logger.h"

// Pass-specific data
typedef struct ShadowPassData {
    Scene *scene;
    Camera *light_camera;
    RGResourceHandle shadow_map;
} ShadowPassData;

// Pass execution callback
static void shadow_pass_execute(RGPassContext *ctx, void *user_data) {
    ShadowPassData *data = (ShadowPassData *)user_data;
    
    // Get physical texture from render graph
    TextureID shadow_tex = rg_ctx_get_texture(ctx, data->shadow_map);
    
    LOG_DEBUG("Shadow pass: rendering to texture %u", shadow_tex.id);
    
    // Begin render pass
    RenderPassInfo pass_info = {
        .depth_attachment = shadow_tex,
        .clear_depth = 1.0f,
        .load_op_depth = LOAD_OP_CLEAR
    };
    cmd_begin_render_pass(ctx->cmd, &pass_info);
    
    // Set viewport and scissor
    cmd_set_viewport(ctx->cmd, 0, 0, 2048, 2048);
    cmd_set_scissor(ctx->cmd, 0, 0, 2048, 2048);
    
    // Bind shadow shader pipeline
    // cmd_bind_pipeline(ctx->cmd, shadow_pipeline);
    
    // Render scene from light's perspective
    // render_scene_depth(data->scene, data->light_camera, ctx->cmd);
    
    cmd_end_render_pass(ctx->cmd);
    
    LOG_DEBUG("Shadow pass: complete");
}

// Setup function - declares resources and pass in render graph
RGResourceHandle setup_shadow_pass(RenderGraph *rg, Scene *scene, Camera *light_camera) {
    // Declare shadow map resource
    RGTextureDesc shadow_desc = {
        .width = 2048,
        .height = 2048,
        .depth = 1,
        .format = FORMAT_DEPTH32F,
        .usage = TEXTURE_USAGE_DEPTH_STENCIL | TEXTURE_USAGE_SAMPLED,
        .name = "ShadowMap"
    };
    RGResourceHandle shadow_map = rg_create_texture(rg, &shadow_desc);
    
    // Allocate pass data
    ShadowPassData *data = (ShadowPassData *)malloc(sizeof(ShadowPassData));
    data->scene = scene;
    data->light_camera = light_camera;
    data->shadow_map = shadow_map;
    
    // Declare pass
    RGPassDesc pass_desc = {
        .name = "ShadowPass",
        .execute = shadow_pass_execute,
        .user_data = data
    };
    RGPassHandle pass = rg_add_pass(rg, &pass_desc);
    
    // Declare resource usage
    rg_pass_write(rg, pass, shadow_map);
    
    LOG_INFO("Shadow pass configured: 2048x2048 depth map");
    
    return shadow_map;  // Return for use by subsequent passes
}

// Example usage in main renderer
void example_render_with_graph(Renderer *renderer, Scene *scene) {
    // Create render graph
    RenderGraph *rg = rg_create();
    
    // Import swapchain
    RGResourceHandle swapchain = rg_import_texture(rg, renderer->swapchain_image, "Swapchain");
    
    // Setup shadow pass
    RGResourceHandle shadow_map = setup_shadow_pass(rg, scene, scene->directional_light.camera);
    
    // Setup other passes (G-buffer, lighting, etc.) would go here
    // RGResourceHandle gbuffer = setup_gbuffer_pass(rg, scene, shadow_map);
    // setup_lighting_pass(rg, gbuffer, shadow_map, swapchain);
    
    // Compile the graph
    if (!rg_compile(rg)) {
        LOG_ERROR("Failed to compile render graph");
        rg_destroy(rg);
        return;
    }
    
    // Execute all passes
    rg_execute(rg, renderer->cmd);
    
    // Get statistics
    RGStats stats;
    rg_get_stats(rg, &stats);
    LOG_INFO("Graph stats: %u passes, %u resources, %llu bytes allocated",
             stats.executed_passes, stats.total_resources, stats.transient_memory_allocated);
    
    // Cleanup
    rg_destroy(rg);
}
