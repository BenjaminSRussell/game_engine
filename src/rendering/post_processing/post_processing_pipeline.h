// Post-Processing Pipeline Integration
// Combines TAA, Bloom, Tonemapping using Render Graph

#ifndef POST_PROCESSING_PIPELINE_H
#define POST_PROCESSING_PIPELINE_H

#include "rendering/post_processing/taa.h"
#include "rendering/post_processing/ssao.h"
#include "rendering/post_processing/ssr.h"
#include "rendering/frame_graph/frame_graph.h"
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
    
    // SSAO
    bool enable_ssao;
    f32 ssao_radius;           // 0.5f default
    f32 ssao_strength;         // 1.0f default
    u32 ssao_samples;          // 16 default
    
    // SSR
    bool enable_ssr;
    f32 ssr_max_distance;      // 50.0f default
    u32 ssr_max_steps;         // 64 default
    f32 ssr_thickness;         // 0.1f default
    
} PostProcessingConfig;

// Post-processing pipeline context
typedef struct PostProcessingPipeline {
    PostProcessingConfig config;
    
    // Effect contexts
    TAAContext *taa;
    SSAOContext *ssao;
    SSRContext *ssr;
    
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
                                               RGResourceHandle depth_buffer,
                                               RGResourceHandle normal_buffer);

// Update configuration
void post_processing_update_config(PostProcessingPipeline *pipeline, 
                                    const PostProcessingConfig *config);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESSING_PIPELINE_H
