#include "rendering/core/render_graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simple buffer for JSON output
static char json_buffer[65536];

RenderGraph* render_graph_create(const char* name) {
    RenderGraph* graph = (RenderGraph*)malloc(sizeof(RenderGraph));
    if (!graph) return NULL;
    
    graph->name = name; // Assumed static/stable
    graph->head = NULL;
    graph->tail = NULL;
    graph->is_valid = true;
    
    return graph;
}

void render_graph_destroy(RenderGraph* graph) {
    if (!graph) return;
    // Nodes are not freed here as they might be statically allocated or managed elsewhere
    // In a fuller implementation, we'd have ownership flags
    free(graph);
}

void render_graph_add_node(RenderGraph* graph, RenderNode* node) {
    if (!graph || !node) return;
    
    node->next = NULL;
    
    if (graph->tail) {
        graph->tail->next = node;
        graph->tail = node;
    } else {
        graph->head = node;
        graph->tail = node;
    }
}

void render_graph_execute(RenderGraph* graph, struct RenderContext* ctx) {
    if (!graph || !graph->head) return;
    
    RenderNode* current = graph->head;
    while (current) {
        if (current->enabled && current->execute) {
            current->execute(current, ctx);
        }
        current = current->next;
    }
}

const char* render_graph_to_json(RenderGraph* graph) {
    if (!graph) return "null";
    
    int offset = snprintf(json_buffer, sizeof(json_buffer), 
        "{\n  \"name\": \"%s\",\n  \"nodes\": [\n", graph->name);
        
    RenderNode* current = graph->head;
    while (current) {
        // Basic node info
        const char* stats = current->get_stats ? current->get_stats(current) : "{}";
        
        offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset,
            "    {\n      \"name\": \"%s\",\n      \"enabled\": %s,\n      \"stats\": %s\n    }%s\n",
            current->name,
            current->enabled ? "true" : "false",
            stats,
            current->next ? "," : ""
        );
        
        if ((size_t)offset >= sizeof(json_buffer) - 100) break; // formatting safety
        current = current->next;
    }
    
    if ((size_t)offset < sizeof(json_buffer) - 2) {
        snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "  ]\n}");
    }
    return json_buffer;
}
