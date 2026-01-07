// Post-Processing Pipeline - Implementation
#include "rendering/post_processing/post_processing_pipeline.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

PostProcessingPipeline *post_processing_create(u32 width, u32 height) {
    PostProcessingPipeline *pipeline = (PostProcessingPipeline *)calloc(1, sizeof(PostProcessingPipeline));
    if (!pipeline) {
        LOG_ERROR("Failed to allocate post-processing pipeline");
        return NULL;
    }
    
    pipeline->width = width;
    pipeline->height = height;
    
    // Default configuration
    pipeline->config.enable_taa = true;
    pipeline->config.taa_settings.blend_factor = 0.05f;
    pipeline->config.taa_settings.sharpness = 0.5f;
    pipeline->config.taa_settings.enable_sharpening = true;
    pipeline->config.taa_settings.enable_jitter = true;
    
    pipeline->config.enable_bloom = true;
    pipeline->config.bloom_intensity = 0.1f;
    pipeline->config.bloom_iterations = 5;
    
    pipeline->config.enable_tonemapping = true;
    pipeline->config.tonemap_operator = 0;  // ACES
    pipeline->config.exposure = 1.0f;
    
    pipeline->config.enable_color_grading = false;
    
    // Create TAA context
    pipeline->taa = taa_create(width, height);
    if (!pipeline->taa) {
        LOG_ERROR("Failed to create TAA context");
        free(pipeline);
        return NULL;
    }
    
    pipeline->initialized = true;
    
    LOG_INFO("Post-processing pipeline initialized (%ux%u)", width, height);
    return pipeline;
}

void post_processing_destroy(PostProcessingPipeline *pipeline) {
    if (!pipeline) return;
    
    if (pipeline->taa) {
        taa_destroy(pipeline->taa);
    }
    
    free(pipeline);
}

RGResourceHandle post_processing_add_to_graph(RenderGraph *rg,
                                               PostProcessingPipeline *pipeline,
                                               RGResourceHandle scene_hdr,
                                               RGResourceHandle velocity) {
    if (!rg || !pipeline) return (RGResourceHandle){0};
    
    RGResourceHandle current = scene_hdr;
    
    // TAA
    if (pipeline->config.enable_taa) {
        current = taa_add_to_graph(rg, pipeline->taa, current, velocity);
    }
    
    // Bloom (would add here)
    if (pipeline->config.enable_bloom) {
        // current = bloom_add_to_graph(rg, ...);
    }
    
    // Tonemapping (would add here)
    if (pipeline->config.enable_tonemapping) {
        // current = tonemap_add_to_graph(rg, ...);
    }
    
    // Color grading (would add here)
    if (pipeline->config.enable_color_grading) {
        // current = color_grade_add_to_graph(rg, ...);
    }
    
    return current;
}

void post_processing_update_config(PostProcessingPipeline *pipeline,
                                    const PostProcessingConfig *config) {
    if (!pipeline || !config) return;
    
    pipeline->config = *config;
    
    // Update TAA settings
    if (pipeline->taa) {
        taa_update_settings(pipeline->taa, &config->taa_settings);
    }
}
