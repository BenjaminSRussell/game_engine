// Post-Processing System Implementation
// Integrates TAA, Bloom, Tonemapping, SSAO, and other effects
#include "rendering/post_processing.h"
#include "rendering/post_processing/post_processing_pipeline.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include "rendering/core/renderer.h"
#include <stdlib.h>
#include <string.h>

// Global post-processing state
static PostProcessingPipeline* g_pipeline = NULL;
static bool g_initialized = false;

// Initialize post-processing system
bool post_processing_init(u32 width, u32 height) {
    if (g_initialized) {
        LOG_WARN("Post-processing already initialized");
        return true;
    }

    g_pipeline = post_processing_create(width, height);
    if (!g_pipeline) {
        LOG_ERROR("Failed to create post-processing pipeline");
        return false;
    }

    g_initialized = true;
    LOG_INFO("Post-processing system initialized (%ux%u)", width, height);
    return true;
}

// Shutdown post-processing system
void post_processing_shutdown(void) {
    if (!g_initialized || !g_pipeline) {
        return;
    }

    post_processing_destroy(g_pipeline);
    g_pipeline = NULL;
    g_initialized = false;

    LOG_INFO("Post-processing system shutdown");
}

// Apply post-processing to scene
bool post_processing_apply(Renderer* renderer, Texture* scene_color, Texture* depth_buffer) {
    if (!g_initialized || !g_pipeline) {
        LOG_ERROR("Post-processing not initialized");
        return false;
    }

    // Build render graph with post-processing effects
    RenderGraph* rg = renderer_get_render_graph(renderer);
    if (!rg) {
        LOG_ERROR("Failed to get render graph");
        return false;
    }

    // Add scene color as input
    RGResourceHandle input = render_graph_import_texture(rg, scene_color, "scene_color");
    if (input == RG_INVALID_HANDLE) {
        LOG_ERROR("Failed to import scene color texture");
        return false;
    }

    // Apply post-processing pipeline
    RGResourceHandle output = post_processing_pipeline_execute(g_pipeline, rg, input);
    if (output == RG_INVALID_HANDLE) {
        LOG_ERROR("Failed to execute post-processing pipeline");
        return false;
    }

    return true;
}

// Update post-processing settings
void post_processing_update_settings(const PostProcessingSettings* settings) {
    if (!g_initialized || !g_pipeline || !settings) {
        return;
    }

    post_processing_pipeline_set_config(g_pipeline, settings);
}

// Get current post-processing settings
const PostProcessingSettings* post_processing_get_settings(void) {
    if (!g_initialized || !g_pipeline) {
        return NULL;
    }

    return post_processing_pipeline_get_config(g_pipeline);
}

// Enable/disable specific effects
void post_processing_enable_effect(PostProcessingEffect effect, bool enable) {
    if (!g_initialized || !g_pipeline) {
        return;
    }

    switch (effect) {
        case POST_PROCESS_EFFECT_TAA:
            g_pipeline->config.enable_taa = enable;
            break;
        case POST_PROCESS_EFFECT_BLOOM:
            g_pipeline->config.enable_bloom = enable;
            break;
        case POST_PROCESS_EFFECT_TONEMAPPING:
            g_pipeline->config.enable_tonemapping = enable;
            break;
        case POST_PROCESS_EFFECT_SSAO:
            g_pipeline->config.enable_ssao = enable;
            break;
        case POST_PROCESS_EFFECT_COLOR_GRADING:
            g_pipeline->config.enable_color_grading = enable;
            break;
        case POST_PROCESS_EFFECT_FXAA:
            g_pipeline->config.enable_fxaa = enable;
            break;
        case POST_PROCESS_EFFECT_MOTION_BLUR:
            g_pipeline->config.enable_motion_blur = enable;
            break;
        default:
            LOG_WARN("Unknown post-processing effect: %d", effect);
            break;
    }
}

// Check if post-processing is initialized
bool post_processing_is_initialized(void) {
    return g_initialized && g_pipeline != NULL;
}

// Resize post-processing buffers
bool post_processing_resize(u32 width, u32 height) {
    if (!g_initialized || !g_pipeline) {
        LOG_ERROR("Post-processing not initialized");
        return false;
    }

    return post_processing_pipeline_resize(g_pipeline, width, height);
}
