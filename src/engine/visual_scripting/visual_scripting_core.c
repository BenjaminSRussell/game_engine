#include <core/hashmap.h>
#include <stdlib.h>
#include <string.h>
#include <visual_scripting/visual_scripting_core.h>

static HashMap *g_node_factories = NULL; // <string, VSNodeFactory>

static u32 str_hash(const void *key) {
  const char *str = (const char *)key;
  u32 hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}
static bool str_equals(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b) == 0;
}

VSGraph *vs_graph_create(void) {
  VSGraph *graph = calloc(1, sizeof(VSGraph));
  graph->capacity = 16;
  graph->nodes = calloc(graph->capacity, sizeof(VSNode *));
  return graph;
}

void vs_graph_destroy(VSGraph *graph) {
  if (!graph)
    return;
  for (u32 i = 0; i < graph->node_count; i++) {
    // destroy node func
    free(graph->nodes[i]);
  }
  free(graph->nodes);
  free(graph);
}

void vs_register_node_type(const char *type_name, VSNodeFactory factory) {
  if (!g_node_factories) {
    g_node_factories = hashmap_create(16, sizeof(char *), sizeof(VSNodeFactory),
                                      str_hash, str_equals);
  }
  hashmap_set(g_node_factories, type_name, factory);
}

VSNode *vs_graph_add_node(VSGraph *graph, const char *type) {
  if (!g_node_factories)
    return NULL;

  VSNodeFactory factory = (VSNodeFactory)hashmap_get(g_node_factories, type);
  if (!factory) {
    LOG_ERROR("Unknown node type: %s", type);
    return NULL;
  }

  VSNode *node = factory();
  if (!node)
    return NULL;

  // Add to graph
  if (graph->node_count >= graph->capacity) {
    graph->capacity *= 2;
    graph->nodes = realloc(graph->nodes, sizeof(VSNode *) * graph->capacity);
  }
  graph->nodes[graph->node_count++] = node;
  return node;
}

void vs_graph_connect(VSPin *out, VSPin *in) {
  if (!out || !in)
    return;
  if (!out->is_output || in->is_output)
    return; // Basic validation

  // Connect
  out->connected_to = in;
  in->connected_to = out; // Should handle multiple connections for exec vs data
}

void vs_execute_graph(VSGraph *graph) {
  if (!graph || graph->node_count == 0)
    return;

  // Find entry node (simplification)
  VSNode *start = graph->nodes[0]; // Assuming first node is start for now

  VSContext ctx = {0};
  ctx.graph = graph;
  ctx.is_running = true;
  ctx.current_node = start;

  while (ctx.is_running && ctx.current_node) {
    if (ctx.current_node->execute) {
      ctx.current_node->execute(&ctx, ctx.current_node);
    } else {
      break;
    }
    // Flow control handled by nodes updating ctx.current_node
  }
}
