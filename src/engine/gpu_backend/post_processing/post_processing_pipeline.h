// Post-Processing Pipeline Integration
// Combines TAA, Bloom, Tonemapping using Render Graph

#ifndef POST_PROCESSING_PIPELINE_H
#define POST_PROCESSING_PIPELINE_H

#include "taa.h"
#include "renderer/frame_graph/frame_graph.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Complete post-processing stack configuration
typedef struct PostProcessingConfig {
    // TAA
    bool enable_taa;
    TAASettings taa_settings;
    
    // Bloom
    bool enable_bloom;
    f32 bloom_intensity;       // 0.1f default
    u32 bloom_iterations;       // 5 default
    
    // Tonemapping
    bool enable_tonemapping;
    u32 tonemap_operator;       // 0=ACES, 1=Filmic, 2=Reinhard
    f32 exposure;               // 1.0f default, will be auto later
    
    // Color grading
    bool enable_color_grading;
    TextureID lut_texture;      // 3D LUT for color grading
    
} PostProcessingConfig;

// Post-processing pipeline context
typedef struct PostProcessingPipeline {
    PostProcessingConfig config;
    
    // Effect contexts
    TAAContext *taa;
    
    // Screen dimensions
    u32 width;
    u32 height;
    
    bool initialized;
} PostProcessingPipeline;

// Create post-processing pipeline
PostProcessingPipeline *post_processing_create(u32 width, u32 height);

// Destroy pipeline
void post_processing_destroy(PostProcessingPipeline *pipeline);

// Add full post-processing stack to render graph
// Returns final output resource
RGResourceHandle post_processing_add_to_graph(RenderGraph *rg,
                                               PostProcessingPipeline *pipeline,
                                               RGResourceHandle scene_hdr,
                                               RGResourceHandle velocity);

// Update configuration
void post_processing_update_config(PostProcessingPipeline *pipeline, 
                                    const PostProcessingConfig *config);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESSING_PIPELINE_H
