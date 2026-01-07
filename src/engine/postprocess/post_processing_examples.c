// Example: Post-Processing Integration with Render Graph

#include "postprocess/post_processing_pipeline.h"
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"

void example_post_processing_integration() {
    LOG_INFO("=== Post-Processing Integration Example ===");
    
    // Create render graph
    RenderGraph *rg = rg_create();
    
    // Import scene render targets
    // (In real code, these would come from G-buffer/lighting pass)
    TextureID scene_hdr_tex = {1};  // Placeholder
    TextureID velocity_tex = {2};   // Placeholder
    
    RGResourceHandle scene_hdr = rg_import_texture(rg, scene_hdr_tex, "Scene_HDR");
    RGResourceHandle velocity = rg_import_texture(rg, velocity_tex, "Velocity");
    
    // Create post-processing pipeline
    PostProcessingPipeline *pp = post_processing_create(1920, 1080);
    
    // Configure post-processing
    PostProcessingConfig config = pp->config;
    config.enable_taa = true;
    config.taa_settings.blend_factor = 0.05f;
    config.enable_bloom = true;
    config.bloom_intensity = 0.15f;
    config.enable_tonemapping = true;
    config.tonemap_operator = 0;  // ACES
    post_processing_update_config(pp, &config);
    
    // Add post-processing to render graph
    RGResourceHandle final_output = post_processing_add_to_graph(rg, pp, scene_hdr, velocity);
    
    // Export to swapchain
    TextureID swapchain_tex = {100};
    RGResourceHandle swapchain = rg_import_texture(rg, swapchain_tex, "Swapchain");
    
    // Add blit pass (copy final_output to swapchain)
    // Would use proper blit implementation
    
    // Compile and execute
    if (rg_compile(rg)) {
        // Get camera jitter for TAA
        f32 jitter_x, jitter_y;
        taa_get_jitter_offset(pp->taa, &jitter_x, &jitter_y);
        
        // Apply jitter to camera projection before rendering scene
        // projection_matrix = apply_jitter(projection_matrix, jitter_x, jitter_y);
        
        LOG_INFO("Camera jitter: (%.4f, %.4f)", jitter_x, jitter_y);
        
        // Execute render graph
        // rg_execute(rg, cmd_buffer);
    }
    
    // Cleanup
    post_processing_destroy(pp);
    rg_destroy(rg);
    
    LOG_INFO("=== Post-Processing Example Complete ===");
}

// Example: Render loop with post-processing
void example_render_loop_with_postprocessing() {
    // One-time setup
    RenderGraph *rg = rg_create();
    PostProcessingPipeline *pp = post_processing_create(1920, 1080);
    
    // Per-frame rendering
    for (int frame = 0; frame < 100; frame++) {
        // Reset graph for new frame
        rg_reset(rg);
        
        // Declare resources
        // ... (scene rendering passes)
        
        // Add post-processing
        // final_output = post_processing_add_to_graph(rg, pp, scene, velocity);
        
        // Compile and execute
        // rg_compile(rg);
        // rg_execute(rg, cmd);
    }
    
    // Cleanup
    post_processing_destroy(pp);
    rg_destroy(rg);
}
