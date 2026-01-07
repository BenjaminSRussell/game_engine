#ifndef ENGINE_RENDER_GRAPH_H
#define ENGINE_RENDER_GRAPH_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
struct RenderContext;
struct RenderNode;

// Function pointers for node operations
typedef void (*RenderNodeExecuteFn)(struct RenderNode* self, struct RenderContext* ctx);
typedef void (*RenderNodeVisualizeFn)(struct RenderNode* self);
typedef const char* (*RenderNodeGetStatsFn)(struct RenderNode* self);

typedef struct RenderNode {
    const char* name;
    bool enabled;
    
    // Links
    struct RenderNode* next;
    
    // Execution
    RenderNodeExecuteFn execute;
    RenderNodeVisualizeFn debug_visualize;
    RenderNodeGetStatsFn get_stats;
    
    // Node-specific data
    void* params;
    size_t params_size;
} RenderNode;

typedef struct {
    const char* name;
    RenderNode* head;
    RenderNode* tail;
    bool is_valid;
} RenderGraph;

// --- API ---

// Create a new empty render graph
RenderGraph* render_graph_create(const char* name);

// Destroy a render graph
void render_graph_destroy(RenderGraph* graph);

// Add a node to the graph execution chain
// The node memory is managed by the caller usually, or the graph can take ownership if designed so.
// For simplicity, we just link it.
void render_graph_add_node(RenderGraph* graph, RenderNode* node);

// Execute the graph
void render_graph_execute(RenderGraph* graph, struct RenderContext* ctx);

// Export graph as JSON for AI introspection
// Returns a buffer that must be freed or is static
const char* render_graph_to_json(RenderGraph* graph);

#endif // ENGINE_RENDER_GRAPH_H
