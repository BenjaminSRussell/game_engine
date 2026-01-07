/*
 * render_graph_integration.h
 * Render graph integration for GPU-driven rendering
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_RENDER_GRAPH_INTEGRATION_H
#define RENDERING_RENDER_GRAPH_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct gpu_culling_context gpu_culling_context_t;
typedef struct instance_buffer instance_buffer_t;
typedef struct rendering_multi_draw_indirect_handle rendering_multi_draw_indirect_handle_t;

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Render pass that uses instancing
typedef struct instancing_render_pass {
    const char* pass_name;
    
    // Inputs
    instance_buffer_t* instance_buffer;
    gpu_culling_context_t* culling_context;
    
    // Outputs
    rendering_multi_draw_indirect_handle_t indirect_handle;
    
    // Configuration
    bool enable_culling;
    bool enable_batching;
    uint32_t max_instances;
} instancing_render_pass_t;

// Render graph node for GPU culling pass
typedef struct culling_pass_node {
    const char* name;
    gpu_culling_context_t* context;
    
    // Dependencies
    const char* input_instances;    // Name of instance buffer resource
    const char* output_visible_ids; // Name of visible IDs resource
    
    bool enabled;
} culling_pass_node_t;

// Render graph node for batch generation pass
typedef struct batch_gen_pass_node {
    const char* name;
    
    // Dependencies
    const char* input_visible_ids;  // From culling pass
    const char* output_commands;    // Indirect draw commands
    
    bool enabled;
} batch_gen_pass_node_t;

/* ============================================================================
 * API - RENDER GRAPH INTEGRATION
 * ============================================================================ */

/**
 * Register instancing render pass with render graph
 * Stub implementation - would integrate with actual render graph system
 */
int render_graph_add_instancing_pass(
    void* render_graph,
    const instancing_render_pass_t* pass_desc);

/**
 * Register GPU culling pass as render graph node
 */
int render_graph_add_culling_pass(
    void* render_graph,
    const culling_pass_node_t* culling_node);

/**
 * Register batch generation pass as render graph node
 */
int render_graph_add_batch_gen_pass(
    void* render_graph,
    const batch_gen_pass_node_t* batch_node);

/**
 * Setup dependencies between instancing-related passes
 */
int render_graph_setup_instancing_dependencies(
    void* render_graph,
    const char* culling_pass,
    const char* batch_pass,
    const char* render_pass);

/**
 * Execute instancing passes in render graph order
 * Stub implementation
 */
int render_graph_execute_instancing_passes(void* render_graph);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================ */

/**
 * Create default instancing render pass descriptor
 */
instancing_render_pass_t render_graph_create_default_instancing_pass(void);

/**
 * Validate instancing pass configuration
 */
bool render_graph_validate_instancing_pass(const instancing_render_pass_t* pass);

/**
 * Print instancing render graph structure
 */
void render_graph_print_instancing_structure(void* render_graph);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_RENDER_GRAPH_INTEGRATION_H */
