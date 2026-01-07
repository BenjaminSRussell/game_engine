/*
 * postprocess_graph.h
 * High-level API for integrating post-processing into the render graph.
 * 
 * Part of the Post-Processing system
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESS_GRAPH_H
#define POSTPROCESS_GRAPH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../rendering/render_graph/render_pass_node.h"
#include "../rendering/render_graph/graph_compiler.h"
#include "bloom_pipeline.h"
#include "tonemapping_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocess_graph_config {
    bloom_settings_t bloom;
    tonemapping_settings_t tonemapping;
    
    bool enable_bloom;
    bool enable_tonemapping;
    bool enable_auto_exposure;
    bool enable_color_grading;
    
    uint32_t output_width;
    uint32_t output_height;
} postprocess_graph_config_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Helper to get default configuration */
postprocess_graph_config_t postprocess_graph_default_config(uint32_t width, uint32_t height);

/* 
 * Adds the entire post-processing chain to the render graph.
 * 
 * graph: The render graph to add passes to
 * input_hdr: The input HDR texture handle (usually lighting output)
 * config: Configuration settings
 * 
 * Returns the handle to the final LDR/HDR result texture.
 */
rg_resource_handle_t postprocess_graph_add_to_graph(render_graph_t* graph, 
                                                  rg_resource_handle_t input_hdr,
                                                  const postprocess_graph_config_t* config);

/* 
 * Individual pass addition if more control is needed 
 */
rg_resource_handle_t postprocess_graph_add_bloom(render_graph_t* graph, 
                                               rg_resource_handle_t input_hdr,
                                               const bloom_settings_t* settings);

rg_resource_handle_t postprocess_graph_add_tonemapping(render_graph_t* graph, 
                                                    rg_resource_handle_t input_hdr,
                                                    rg_resource_handle_t bloom_hdr,
                                                    const tonemapping_settings_t* settings);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESS_GRAPH_H */
