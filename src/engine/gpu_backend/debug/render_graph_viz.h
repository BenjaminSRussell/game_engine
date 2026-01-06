#pragma once

#include <stdbool.h>

typedef struct RenderPass {
    const char *name;
    int id;
    
    // Resources
    int *input_resources;
    int input_count;
    int *output_resources;
    int output_count;
    
    // Stats
    float execution_time_ms;
    bool enabled;
} RenderPass;

typedef struct {
    RenderPass *passes;
    int pass_count;
    
    // Visualization
    bool show_dependencies;
    bool show_resource_sizes;
    int selected_pass;
} RenderGraphViz;

void render_graph_viz_init(RenderGraphViz *viz);
void render_graph_viz_destroy(RenderGraphViz *viz);

// Graph operations
void render_graph_viz_add_pass(RenderGraphViz *viz, const char *name);
void render_graph_viz_add_dependency(RenderGraphViz *viz, int from_pass, int to_pass);

// Visualization
void render_graph_viz_draw(RenderGraphViz *viz);
void render_graph_viz_export_dot(RenderGraphViz *viz, const char *output_path);

// Interaction
void render_graph_viz_toggle_pass(RenderGraphViz *viz, int pass_id);
void render_graph_viz_select_pass(RenderGraphViz *viz, int pass_id);
