// Post-Processing Pipeline Implementation
// Combines TAA, Bloom, Tonemapping, HDR using Render Graph
#include "post_processing_pipeline.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for individual effects
extern RGResourceHandle bloom_add_to_graph(RenderGraph *rg, RGResourceHandle scene_color,
                                          f32 intensity, u32 iterations);
extern RGResourceHandle tonemap_add_to_graph(RenderGraph *rg, RGResourceHandle hdr_color,
                                            u32 operator, f32 exposure);

// Create post-processing pipeline
PostProcessingPipeline *post_processing_create(u32 width, u32 height) {
    PostProcessingPipeline *pipeline = malloc(sizeof(PostProcessingPipeline));
    if (!pipeline) {
        LOG_ERROR("Failed to allocate post-processing pipeline");
        return NULL;
    }

    memset(pipeline, 0, sizeof(PostProcessingPipeline));

    pipeline->width = width;
    pipeline->height = height;

    // Initialize default config
    pipeline->config.enable_taa = true;
    pipeline->config.taa_settings.blend_factor = 0.05f;
    pipeline->config.taa_settings.sharpness = 0.5f;
    pipeline->config.taa_settings.enable_sharpening = true;
    pipeline->config.taa_settings.enable_jitter = true;
    pipeline->config.taa_settings.jitter_scale = 1.0f;

    pipeline->config.enable_bloom = true;
    pipeline->config.bloom_intensity = 0.1f;
    pipeline->config.bloom_iterations = 5;

    pipeline->config.enable_tonemapping = true;
    pipeline->config.tonemap_operator = 0; // ACES
    pipeline->config.exposure = 1.0f;

    pipeline->config.enable_color_grading = false;

    // Initialize SSAO settings
    pipeline->config.enable_ssao = true;
    pipeline->config.ssao_radius = 0.5f;
    pipeline->config.ssao_strength = 1.0f;
    pipeline->config.ssao_samples = 16;

    // Initialize SSR settings
    pipeline->config.enable_ssr = true;
    pipeline->config.ssr_max_distance = 50.0f;
    pipeline->config.ssr_max_steps = 64;
    pipeline->config.ssr_thickness = 0.1f;

    // Create TAA context if enabled
    if (pipeline->config.enable_taa) {
        pipeline->taa = taa_create(width, height);
        if (!pipeline->taa) {
            LOG_WARN("Failed to create TAA context");
        }
    }

    // Create SSAO context if enabled
    if (pipeline->config.enable_ssao) {
        pipeline->ssao = ssao_create(width, height);
        if (!pipeline->ssao) {
            LOG_WARN("Failed to create SSAO context");
        }
    }

    // Create SSR context if enabled
    if (pipeline->config.enable_ssr) {
        pipeline->ssr = ssr_create(width, height);
        if (!pipeline->ssr) {
            LOG_WARN("Failed to create SSR context");
        }
    }

    pipeline->initialized = true;
    LOG_INFO("Post-processing pipeline created: %ux%u", width, height);

    return pipeline;
}

// Destroy pipeline
void post_processing_destroy(PostProcessingPipeline *pipeline) {
    if (!pipeline) return;

    if (pipeline->taa) {
        taa_destroy(pipeline->taa);
        pipeline->taa = NULL;
    }

    if (pipeline->ssao) {
        ssao_destroy(pipeline->ssao);
        pipeline->ssao = NULL;
    }

    if (pipeline->ssr) {
        ssr_destroy(pipeline->ssr);
        pipeline->ssr = NULL;
    }

    free(pipeline);
    LOG_INFO("Post-processing pipeline destroyed");
}

// Add full post-processing stack to render graph
RGResourceHandle post_processing_add_to_graph(RenderGraph *rg,
                                             PostProcessingPipeline *pipeline,
                                             RGResourceHandle scene_hdr,
                                             RGResourceHandle depth_buffer,
                                             RGResourceHandle normal_buffer) {
    if (!pipeline || !rg) {
        LOG_ERROR("Invalid post-processing pipeline or render graph");
        return RG_INVALID_RESOURCE;
    }

    RGResourceHandle current = scene_hdr;

    // Apply TAA first (operates on HDR)
    if (pipeline->config.enable_taa && pipeline->taa) {
        current = taa_add_to_graph(rg, pipeline->taa, current, depth_buffer);
        LOG_DEBUG("TAA added to post-processing graph");
    }

    // Apply bloom (operates on HDR)
    if (pipeline->config.enable_bloom) {
        current = bloom_add_to_graph(rg, current,
                                    pipeline->config.bloom_intensity,
                                    pipeline->config.bloom_iterations);
        LOG_DEBUG("Bloom added to post-processing graph");
    }

    // Apply tonemapping (HDR to SDR conversion)
    if (pipeline->config.enable_tonemapping) {
        current = tonemap_add_to_graph(rg, current,
                                      pipeline->config.tonemap_operator,
                                      pipeline->config.exposure);
        LOG_DEBUG("Tonemapping added to post-processing graph");
    }

    // Apply SSAO (Screen Space Ambient Occlusion)
    if (pipeline->config.enable_ssao && pipeline->ssao && normal_buffer.id != 0) {
        current = ssao_add_to_graph(rg, pipeline->ssao, depth_buffer, normal_buffer);
        LOG_DEBUG("SSAO added to post-processing graph");
    }

    // Apply SSR (Screen Space Reflections)
    if (pipeline->config.enable_ssr && pipeline->ssr && normal_buffer.id != 0) {
        // Create a combined normal/roughness buffer for SSR (simplified)
        RGResourceHandle normal_roughness = normal_buffer;
        current = ssr_add_to_graph(rg, pipeline->ssr, current, normal_roughness, depth_buffer);
        LOG_DEBUG("SSR added to post-processing graph");
    }

    // Apply color grading if LUT is available
    if (pipeline->config.enable_color_grading && pipeline->config.lut_texture != 0) {
        current = color_grade_add_to_graph(rg, current, pipeline->config.lut_texture);
        LOG_DEBUG("Color grading added to post-processing graph");
    }

    LOG_DEBUG("Post-processing stack complete: %ux%u", pipeline->width, pipeline->height);
    return current;
}

// Update configuration
void post_processing_update_config(PostProcessingPipeline *pipeline,
                                   const PostProcessingConfig *config) {
    if (!pipeline || !config) return;

    memcpy(&pipeline->config, config, sizeof(PostProcessingConfig));

    // Update TAA settings if enabled
    if (pipeline->taa) {
        taa_update_settings(pipeline->taa, &config->taa_settings);
    }

    LOG_DEBUG("Post-processing config updated");
}
