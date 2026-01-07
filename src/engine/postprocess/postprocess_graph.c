/*
 * postprocess_graph.c
 * High-level API for integrating post-processing into the render graph.
 */

#include "postprocess/postprocess_graph.h"
#include "../rendering/render_graph/resource_node.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * INTERNAL PASS DATA
 * ============================================================================ */

typedef struct bloom_threshold_data {
    bloom_settings_t settings;
} bloom_threshold_data_t;

typedef struct tonemap_pass_data {
    tonemapping_settings_t settings;
} tonemap_pass_data_t;

/* ============================================================================
 * CALLBACKS
 * ============================================================================ */

static void execute_bloom_threshold(void* cmd, void* user_data) {
    // TODO: Metal/GLSL dispatch
    // bloom_threshold_data_t* data = (bloom_threshold_data_t*)user_data;
}

static void execute_bloom_downsample(void* cmd, void* user_data) {
    // TODO: Metal/GLSL dispatch
}

static void execute_bloom_upsample(void* cmd, void* user_data) {
    // TODO: Metal/GLSL dispatch
}

static void execute_tonemap(void* cmd, void* user_data) {
    // TODO: Metal/GLSL dispatch
    // tonemap_pass_data_t* data = (tonemap_pass_data_t*)user_data;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

postprocess_graph_config_t postprocess_graph_default_config(uint32_t width, uint32_t height) {
    postprocess_graph_config_t config;
    config.bloom = bloom_pipeline_default_settings();
    config.tonemapping = tonemapping_pipeline_default_settings();
    config.enable_bloom = true;
    config.enable_tonemapping = true;
    config.enable_auto_exposure = true;
    config.enable_color_grading = true;
    config.output_width = width;
    config.output_height = height;
    return config;
}

rg_resource_handle_t postprocess_graph_add_bloom(render_graph_t* graph, 
                                               rg_resource_handle_t input_hdr,
                                               const bloom_settings_t* settings) {
    if (!graph || input_hdr == RG_INVALID_RESOURCE) return RG_INVALID_RESOURCE;

    uint32_t mip_count = settings->mip_count > 0 ? settings->mip_count : 6;
    if (mip_count > BLOOM_MAX_MIP_LEVELS) mip_count = BLOOM_MAX_MIP_LEVELS;

    // 1. Threshold Pass
    rendering_resource_node_desc_t threshold_desc = {
        .name = "Bloom Threshold Result",
        .type = RENDERING_RESOURCE_TYPE_TEXTURE,
        .width = settings->mip_count > 0 ? 0 : 0, // Need to fetch input dimensions
        .is_transient = true
    };
    // Note: Dimensions should be fetched from input_hdr if possible, 
    // but rg_create_texture might handle relative sizes later or we fetch it here.
    
    // For now assume same as input
    rg_resource_handle_t threshold_tex = rg_create_texture(graph, "Bloom Threshold", &threshold_desc);

    rendering_render_pass_node_desc_t threshold_pass = {
        .name = "Bloom Threshold",
        .type = RENDERING_PASS_TYPE_COMPUTE,
        .texture_inputs = {input_hdr},
        .texture_input_count = 1,
        .storage_outputs = {threshold_tex},
        .storage_output_count = 1,
        .execute = execute_bloom_threshold,
        .user_data = (void*)settings
    };
    rg_add_pass(graph, "Bloom Threshold", RENDERING_PASS_TYPE_COMPUTE, &threshold_pass);

    // 2. Downsample Chain
    rg_resource_handle_t prev_mip = threshold_tex;
    rg_resource_handle_t mip_chain[BLOOM_MAX_MIP_LEVELS];

    for (uint32_t i = 0; i < mip_count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Bloom Mip %u", i);
        
        rendering_resource_node_desc_t mip_desc = {
            .name = name,
            .type = RENDERING_RESOURCE_TYPE_TEXTURE,
            .is_transient = true
        };
        
        mip_chain[i] = rg_create_texture(graph, name, &mip_desc);

        rendering_render_pass_node_desc_t down_pass = {
            .name = name,
            .type = RENDERING_PASS_TYPE_COMPUTE,
            .texture_inputs = {prev_mip},
            .texture_input_count = 1,
            .storage_outputs = {mip_chain[i]},
            .storage_output_count = 1,
            .execute = execute_bloom_downsample
        };
        rg_add_pass(graph, name, RENDERING_PASS_TYPE_COMPUTE, &down_pass);
        prev_mip = mip_chain[i];
    }

    // 3. Upsample Chain
    rg_resource_handle_t current_upsample = prev_mip;
    for (int i = (int)mip_count - 2; i >= 0; i--) {
        char name[64];
        snprintf(name, sizeof(name), "Bloom Upsample %u", i);
        
        rendering_render_pass_node_desc_t up_pass = {
            .name = name,
            .type = RENDERING_PASS_TYPE_COMPUTE,
            .texture_inputs = {current_upsample, mip_chain[i]},
            .texture_input_count = 2,
            .storage_outputs = {mip_chain[i]}, // In-place or new? Let's use new for safety in RG
            .storage_output_count = 1,
            .execute = execute_bloom_upsample
        };
        rg_add_pass(graph, name, RENDERING_PASS_TYPE_COMPUTE, &up_pass);
        current_upsample = mip_chain[i];
    }

    return current_upsample;
}

rg_resource_handle_t postprocess_graph_add_tonemapping(render_graph_t* graph, 
                                                    rg_resource_handle_t input_hdr,
                                                    rg_resource_handle_t bloom_hdr,
                                                    const tonemapping_settings_t* settings) {
    if (!graph || input_hdr == RG_INVALID_RESOURCE) return RG_INVALID_RESOURCE;

    rendering_resource_node_desc_t output_desc = {
        .name = "Final Scene LDR",
        .type = RENDERING_RESOURCE_TYPE_TEXTURE,
        .is_transient = false // Usually final output
    };
    rg_resource_handle_t output_ldr = rg_create_texture(graph, "Final Scene", &output_desc);

    rendering_render_pass_node_desc_t tonemap_pass = {
        .name = "Post-Processing (Tonemap/Bloom/LUT)",
        .type = RENDERING_PASS_TYPE_COMPUTE,
        .texture_inputs = {input_hdr, bloom_hdr},
        .texture_input_count = (bloom_hdr != RG_INVALID_RESOURCE) ? 2 : 1,
        .storage_outputs = {output_ldr},
        .storage_output_count = 1,
        .execute = execute_tonemap,
        .user_data = (void*)settings
    };
    rg_add_pass(graph, "Tonemapping", RENDERING_PASS_TYPE_COMPUTE, &tonemap_pass);

    return output_ldr;
}

rg_resource_handle_t postprocess_graph_add_to_graph(render_graph_t* graph, 
                                                  rg_resource_handle_t input_hdr,
                                                  const postprocess_graph_config_t* config) {
    if (!graph) return RG_INVALID_RESOURCE;

    rg_resource_handle_t bloom_hdr = RG_INVALID_RESOURCE;
    if (config->enable_bloom) {
        bloom_hdr = postprocess_graph_add_bloom(graph, input_hdr, &config->bloom);
    }

    rg_resource_handle_t final_output = input_hdr;
    if (config->enable_tonemapping) {
        final_output = postprocess_graph_add_tonemapping(graph, input_hdr, bloom_hdr, &config->tonemapping);
    }

    return final_output;
}
