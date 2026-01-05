#include <renderer/shader_graph.h>
#include "core/logger.h"
#include "core/memory.h"
#include "core/string_utils.h"
#include <stdio.h>
#include <stdlib.h>

SGGraph *sg_create_graph(void) {
  SGGraph *g = calloc(1, sizeof(SGGraph));
  g->capacity = 16;
  g->nodes = calloc(g->capacity, sizeof(SGNode *));
  return g;
}

void sg_destroy_graph(SGGraph *graph) {
  if (!graph)
    return;
  for (u32 i = 0; i < graph->node_count; i++) {
    // destroy node
    free(graph->nodes[i]); // Simplification
  }
  free(graph->nodes);
  free(graph);
}

SGNode *sg_add_node(SGGraph *graph, SGNodeType type) {
  SGNode *node = calloc(1, sizeof(SGNode));
  node->type = type;
  node->id = graph->node_count + 1;

  // Init pins based on type (simplified)
  if (type == SG_NODE_ADD) {
    node->input_count = 2;
    node->inputs = calloc(2, sizeof(SGPin));
    node->output_count = 1;
    node->outputs = calloc(1, sizeof(SGPin));
  } else if (type == SG_NODE_OUTPUT) {
    node->input_count = 1;
    node->inputs = calloc(1, sizeof(SGPin));
  }

  if (graph->node_count >= graph->capacity) {
    graph->capacity *= 2;
    graph->nodes = realloc(graph->nodes, sizeof(SGNode *) * graph->capacity);
  }
  graph->nodes[graph->node_count++] = node;
  return node;
}

char *sg_compile_to_glsl(SGGraph *graph) {
  // Basic topological sort and code generation would go here
  // For now returning a dummy shader logic
  LOG_INFO("Compiling Shader Graph to GLSL...");
  return strdup("void main() { gl_FragColor = vec4(1.0); }");
}
