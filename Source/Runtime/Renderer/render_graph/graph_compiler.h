/*
 * graph_compiler.h
 * Render graph compilation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GRAPH_COMPILER_H
#define RENDERING_GRAPH_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

#include "rendering/render_graph/render_pass_node.h"
#include "rendering/render_graph/resource_node.h"

typedef struct rendering_render_graph {
    char name[128];
    
    rendering_render_pass_node_handle_t* passes;
    uint32_t pass_count;
    uint32_t pass_capacity;
    
    rendering_resource_node_handle_t* resources;
    uint32_t resource_count;
    uint32_t resource_capacity;
    
    uint32_t* adjacency_list; // For topological sort
    uint32_t* execution_order;
    
    bool compiled;
} rendering_render_graph_t;

// Shorthand alias for user convenience
typedef rendering_render_graph_t render_graph_t;

typedef struct rendering_graph_compiler_handle {
    uint32_t id;
} rendering_graph_compiler_handle_t;

typedef struct rendering_graph_compiler_desc {
    uint32_t flags;
    void* user_data;
} rendering_graph_compiler_desc_t;

typedef struct rendering_graph_compiler_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_graph_compiler_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Render Graph Management */
render_graph_t* render_graph_create(const char* name);
void render_graph_destroy(render_graph_t* graph);

rg_resource_handle_t rg_create_texture(render_graph_t* graph, const char* name, const rendering_resource_node_desc_t* desc);
rg_resource_handle_t rg_create_buffer(render_graph_t* graph, const char* name, const rendering_resource_node_desc_t* desc);

void rg_add_pass(render_graph_t* graph, const char* name, rendering_render_pass_type_t type, const rendering_render_pass_node_desc_t* desc);

int rg_compile(render_graph_t* graph);

/* Graph Compiler API */
int rendering_graph_compiler_init(void);
void rendering_graph_compiler_shutdown(void);
int rendering_graph_compiler_process_pending(void);

/* Statistics */
uint32_t rendering_graph_compiler_get_count(void);
size_t rendering_graph_compiler_get_memory_usage(void);
void rendering_graph_compiler_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GRAPH_COMPILER_H */
