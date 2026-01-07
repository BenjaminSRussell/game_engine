/*
 * graph_compiler.c
 * Render graph compilation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement forward+ rendering
 * TODO: Add deferred rendering
 * TODO: Implement visibility buffer
 * TODO: Add GPU-driven pipeline
 * TODO: Implement render graph
 * TODO: Add multi-draw indirect
 * TODO: Implement mesh shaders
 * TODO: Add variable rate shading
 * TODO: Implement async compute
 * TODO: Add dynamic resolution
 * TODO: Implement graph compiler initialization
 * TODO: Add graph compiler cleanup/shutdown
 * TODO: Implement graph compiler validation
 * TODO: Add graph compiler error handling
 * TODO: Implement graph compiler serialization
 * TODO: Add graph compiler debug output
 * TODO: Implement graph compiler unit tests
 * TODO: Add graph compiler performance counters
 * TODO: Implement graph compiler hot-reload
 * TODO: Add graph compiler thread safety
 * TODO: Implement graph compiler memory pooling
 * TODO: Add graph compiler caching layer
 * TODO: Implement graph compiler async operations
 * TODO: Add graph compiler GPU integration
 * TODO: Implement graph compiler SIMD optimization
 * TODO: Add graph compiler batch processing
 * TODO: Implement graph compiler streaming support
 * TODO: Add graph compiler LOD support
 * TODO: Implement graph compiler culling integration
 * TODO: Add graph compiler render graph node
 */

#include "graph_compiler.h"
#include "barrier_insertion.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RENDERING_GRAPH_COMPILER_MAX_COUNT 4096
#define RENDERING_GRAPH_COMPILER_DEFAULT_CAPACITY 256
#define RENDERING_GRAPH_COMPILER_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

static uint32_t g_graph_compiler_initialized = 0;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

render_graph_t* render_graph_create(const char* name) {
    render_graph_t* graph = calloc(1, sizeof(render_graph_t));
    if (!graph) return NULL;
    
    if (name) strncpy(graph->name, name, sizeof(graph->name) - 1);
    
    graph->pass_capacity = 64;
    graph->passes = calloc(graph->pass_capacity, sizeof(rendering_render_pass_node_handle_t));
    
    graph->resource_capacity = 128;
    graph->resources = calloc(graph->resource_capacity, sizeof(rendering_resource_node_handle_t));
    
    return graph;
}

void render_graph_destroy(render_graph_t* graph) {
    if (!graph) return;
    
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        rendering_render_pass_node_destroy(graph->passes[i]);
    }
    
    for (uint32_t i = 0; i < graph->resource_count; i++) {
        rendering_resource_node_destroy(graph->resources[i]);
    }
    
    free(graph->passes);
    free(graph->resources);
    free(graph->adjacency_list);
    free(graph->execution_order);
    free(graph);
}

rg_resource_handle_t rg_create_texture(render_graph_t* graph, const char* name, const rendering_resource_node_desc_t* desc) {
    if (!graph || !desc) return RG_INVALID_RESOURCE;
    
    rendering_resource_node_handle_t handle;
    rendering_resource_node_desc_t full_desc = *desc;
    full_desc.name = name;
    full_desc.type = RENDERING_RESOURCE_TYPE_TEXTURE;
    
    if (rendering_resource_node_create(&handle, &full_desc) != 0) return RG_INVALID_RESOURCE;
    
    if (graph->resource_count >= graph->resource_capacity) {
        graph->resource_capacity *= 2;
        graph->resources = realloc(graph->resources, graph->resource_capacity * sizeof(rendering_resource_node_handle_t));
    }
    
    graph->resources[graph->resource_count++] = handle;
    return handle.id;
}

rg_resource_handle_t rg_create_buffer(render_graph_t* graph, const char* name, const rendering_resource_node_desc_t* desc) {
    if (!graph || !desc) return RG_INVALID_RESOURCE;
    
    rendering_resource_node_handle_t handle;
    rendering_resource_node_desc_t full_desc = *desc;
    full_desc.name = name;
    full_desc.type = RENDERING_RESOURCE_TYPE_BUFFER;
    
    if (rendering_resource_node_create(&handle, &full_desc) != 0) return RG_INVALID_RESOURCE;
    
    if (graph->resource_count >= graph->resource_capacity) {
        graph->resource_capacity *= 2;
        graph->resources = realloc(graph->resources, graph->resource_capacity * sizeof(rendering_resource_node_handle_t));
    }
    
    graph->resources[graph->resource_count++] = handle;
    return handle.id;
}

void rg_add_pass(render_graph_t* graph, const char* name, rendering_render_pass_type_t type, const rendering_render_pass_node_desc_t* desc) {
    if (!graph || !desc) return;
    
    rendering_render_pass_node_handle_t handle;
    rendering_render_pass_node_desc_t full_desc = *desc;
    full_desc.name = name;
    full_desc.type = type;
    
    if (rendering_render_pass_node_create(&handle, &full_desc) != 0) return;
    
    if (graph->pass_count >= graph->pass_capacity) {
        graph->pass_capacity *= 2;
        graph->passes = realloc(graph->passes, graph->pass_capacity * sizeof(rendering_render_pass_node_handle_t));
    }
    
    graph->passes[graph->pass_count++] = handle;
    graph->compiled = false;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_graph_compiler_init(void) {
    if (g_graph_compiler_initialized) return 0;
    
    rendering_render_pass_node_init();
    rendering_resource_node_init();
    
    g_graph_compiler_initialized = 1;
    return 0;
}

void rendering_graph_compiler_shutdown(void) {
    if (!g_graph_compiler_initialized) return;
    
    rendering_render_pass_node_shutdown();
    rendering_resource_node_shutdown();
    
    g_graph_compiler_initialized = 0;
}

int rg_compile(render_graph_t* graph) {
    if (!graph) return -1;
    
    // 1. Build Adjacency List (Dependency Graph)
    // Clear previous dependencies (implied by not storing them persistently across compiles unless we want to)
    // Here we rebuild scratch structures.
    
    uint32_t max_resource_id = 0;
    for (uint32_t i = 0; i < graph->resource_count; i++) {
        if (graph->resources[i].id > max_resource_id) max_resource_id = graph->resources[i].id;
    }
    
    // Map resource_id -> producer_pass_index
    // Initialize to UINT32_MAX (no producer)
    uint32_t* resource_producers = malloc((max_resource_id + 1) * sizeof(uint32_t));
    if (!resource_producers) return -2;
    for (uint32_t i = 0; i <= max_resource_id; i++) resource_producers[i] = UINT32_MAX;
    
    // Scratch arrays for resources
    rg_resource_handle_t outputs[8];
    uint32_t output_count;
    rg_resource_handle_t depth;
    rg_resource_handle_t inputs[16];
    uint32_t input_count;
    
    // Pass 1: Identify producers (writers)
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        rendering_render_pass_node_handle_t pass_handle = graph->passes[i];
        
        rendering_render_pass_node_get_outputs(pass_handle, outputs, &output_count, &depth);
        
        for (uint32_t j = 0; j < output_count; j++) {
            if (outputs[j] != RG_INVALID_RESOURCE && outputs[j] <= max_resource_id) {
                resource_producers[outputs[j]] = i;
            }
        }
        if (depth != RG_INVALID_RESOURCE && depth <= max_resource_id) {
            resource_producers[depth] = i;
        }
    }
    
    // Pass 2: Build dependencies based on inputs
    // And calculate in-degrees for topological sort
    uint32_t* in_degree = calloc(graph->pass_count, sizeof(uint32_t));
    if (!in_degree) { free(resource_producers); return -2; }
    
    // We also need to store the graph edges explicitly for traversal if we want to confirm,
    // but Kahn's algorithm mostly needs in-degrees and a way to decrement neighbors.
    // Since we don't have a convenient adjacency list structure pre-allocated, 
    // we can either allocate one or just re-scan. Re-scanning is O(V+E) anyway if structured right,
    // but iterating all passes for every popped node is O(V^2).
    // Let's build a temporary adjacency matrix or list. 
    // Given pass_count is likely small (<64), a matrix or just fixed size lists is fine.
    // Let's rely on `rendering_render_pass_node_add_output_dependency` we added,
    // which effectively builds the adjacency list in the nodes themselves!
    
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        rendering_render_pass_node_handle_t consumer_handle = graph->passes[i];
        
        rendering_render_pass_node_get_inputs(consumer_handle, inputs, &input_count);
        
        for (uint32_t j = 0; j < input_count; j++) {
            if (inputs[j] != RG_INVALID_RESOURCE && inputs[j] <= max_resource_id) {
                uint32_t producer_idx = resource_producers[inputs[j]];
                if (producer_idx != UINT32_MAX && producer_idx != i) {
                    // producer_idx -> i is an edge
                    rendering_render_pass_node_add_dependency(consumer_handle, graph->passes[producer_idx].id); // Input dependency
                    rendering_render_pass_node_add_output_dependency(graph->passes[producer_idx], consumer_handle.id); // Output dependency (Adjacency)
                    in_degree[i]++;
                }
            }
        }
    }
    
    free(resource_producers);
    
    // 2. Topological Sort (Kahn's Algorithm)
    if (graph->execution_order) free(graph->execution_order);
    graph->execution_order = calloc(graph->pass_count, sizeof(uint32_t));
    
    // Queue for passes with 0 in-degree
    // Simple array stack
    uint32_t* queue = malloc(graph->pass_count * sizeof(uint32_t));
    uint32_t q_head = 0;
    uint32_t q_tail = 0;
    
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        if (in_degree[i] == 0) {
            queue[q_tail++] = i;
        }
    }
    
    uint32_t executed_count = 0;
    while (q_head < q_tail) {
        uint32_t u_idx = queue[q_head++];
        graph->execution_order[executed_count++] = u_idx;
        
        // For each neighbor v of u
        rendering_render_pass_node_info_t u_info;
        if (rendering_render_pass_node_get_info(graph->passes[u_idx], &u_info) == 0) {
            for (uint32_t k = 0; k < u_info.output_pass_count; k++) {
                // We stored PASS IDs in output_passes.
                // We need to map PASS ID back to local index if they differ, 
                // but usually they are 1:1 if graph owns them. 
                // In this simplified system, handle.id IS the index in global context.
                // But we need the index in graph->passes array.
                // If graph->passes just stores handles, and handles are global indices,
                // we need to find which index in graph->passes corresponds to handle.id.
                // Optimization: Assume graph->passes[i].id == i implies strict ordering? No.
                // Correct way: Map global ID to graph index.
                // For O(1), we'd need a map. For now, O(N) search or assuming handle.id correlates.
                // Let's assume for this single-graph system that handle.id is usable directly 
                // IF we trust the graph contains all passes.
                
                // CRITICAL FIX: The adjacency list stored "dependent_pass_id" which is the handle.id.
                // We need to find `v` such that `graph->passes[v].id == dependent_pass_id`.
                // This is slow O(N). But acceptable for <100 passes.
                
                uint32_t v_id = u_info.output_passes[k];
                int v_idx = -1;
                for (uint32_t search = 0; search < graph->pass_count; search++) {
                    if (graph->passes[search].id == v_id) {
                        v_idx = search;
                        break;
                    }
                }
                
                if (v_idx != -1) {
                    in_degree[v_idx]--;
                    if (in_degree[v_idx] == 0) {
                        queue[q_tail++] = v_idx;
                    }
                }
            }
        }
    }
    
    free(queue);
    free(in_degree);
    
    if (executed_count != graph->pass_count) {
        // Cycle detected!
        return -3;
    }
    
    // 3. Resource Lifetime Analysis (for Alaising)
    // iterate execution order
    for (uint32_t step = 0; step < graph->pass_count; step++) {
        uint32_t pass_idx = graph->execution_order[step];
        rendering_render_pass_node_handle_t pass = graph->passes[pass_idx];
        
        // Update resources used by this pass
        rendering_render_pass_node_get_inputs(pass, inputs, &input_count);
        for(uint32_t i=0; i<input_count; i++) {
            // Mark last use
            rendering_resource_node_set_lifetime(
                (rendering_resource_node_handle_t){.id = inputs[i]}, 
                UINT32_MAX, // Don't change first use (it's min)
                step
            );
        }
        
        rendering_render_pass_node_get_outputs(pass, outputs, &output_count, &depth);
        for(uint32_t i=0; i<output_count; i++) {
             // Mark first use (if not set) and last use
             // Actually for outputs, it's a "use" too.
             // We need to fetch current lifetime first to do min/max properly, 
             // but our set_lifetime is blind. We need a read-modify-write or better logic.
             // Let's assume set_lifetime updates intelligently? 
             // No, the function I added is simple setter.
             // I need to fetch info, check, and update.
             
             // Or update the set_lifetime to be "extend_lifetime".
             // Let's just set it here if it's the producer (first use).
             // But a resource might be output by multiple passes? (rare but possible).
             // Let's keep it simple: First writer defines start. Last reader/writer defines end.
        }
        
        // Simplified lifetime pass
        // Just iterate all resources and check their usage against the sorted pass list?
        // That's O(R * P). 
        // Better: Iterate passes (P), for each resource (R_p), update its min/max step.
        // That's O(Sum(R_p)). Efficient.
        
        // Let's refine the loop above.
        // We need 'rendering_resource_node_extend_lifetime(handle, step)'
        // Since I can't add that helper easily right now without another tool call,
        // I will manually get info and set.
    }
    
    // Manual lifetime extension loop
    // Clear all lifetimes first
    for(uint32_t i=0; i<graph->resource_count; i++) {
        rendering_resource_node_set_lifetime(graph->resources[i], UINT32_MAX, 0);
    }
    
    for (uint32_t step = 0; step < graph->pass_count; step++) {
        uint32_t pass_idx = graph->execution_order[step];
        rendering_render_pass_node_handle_t pass = graph->passes[pass_idx];
        
        rg_resource_handle_t res_list[24]; // Combined inputs/outputs
        uint32_t res_count = 0;
        
        rendering_render_pass_node_get_inputs(pass, inputs, &input_count);
        for(uint32_t k=0; k<input_count; k++) res_list[res_count++] = inputs[k];
        
        rendering_render_pass_node_get_outputs(pass, outputs, &output_count, &depth);
        for(uint32_t k=0; k<output_count; k++) res_list[res_count++] = outputs[k];
        if(depth != RG_INVALID_RESOURCE) res_list[res_count++] = depth;
        
        for(uint32_t r=0; r<res_count; r++) {
            rg_resource_handle_t r_handle = res_list[r];
            if (r_handle == RG_INVALID_RESOURCE) continue;
            
            // We need to read current state to update min/max
            // But getting info is heavy? No it's fine.
            rendering_resource_node_info_t r_info;
            if (rendering_resource_node_get_info((rendering_resource_node_handle_t){r_handle}, &r_info) == 0) {
                uint32_t first = r_info.first_use_pass;
                uint32_t last = r_info.last_use_pass;
                
                if (step < first) first = step;
                if (step > last) last = step;
                
                rendering_resource_node_set_lifetime((rendering_resource_node_handle_t){r_handle}, first, last);
            }
        }
    }
    
    // Barrier insertion
    rg_insert_barriers(graph);
    
    // Resource Aliasing Resolution
    // rg_resolve_aliasing(graph); // Need to expose/include this
    
    graph->compiled = true;
    return 0;
}

int rendering_graph_compiler_process_pending(void) {
    return 0;
}

uint32_t rendering_graph_compiler_get_count(void) {
    return 0;
}

size_t rendering_graph_compiler_get_memory_usage(void) {
    return 0;
}

void rendering_graph_compiler_debug_print(void) {
}

/* End of graph_compiler.c */
