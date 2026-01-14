#include "engine/include/scripting/visual_scripting/visual_scripting.h"
#include "../include/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Internal Helpers
// ----------------------------------------------------------------------------

static VSNode *get_node(VSGraph *graph, VSNodeID id) {
  if (!graph || id >= graph->node_count)
    return NULL;
  return &graph->nodes[id];
}

static VSPin *get_pin(VSGraph *graph, VSPinID id) {
  // Brute force search for V1. Pin ID structure could be optimized (node_idx <<
  // 8 | pin_idx)
  for (u32 i = 0; i < graph->node_count; i++) {
    VSNode *node = &graph->nodes[i];
    for (u32 j = 0; j < node->input_count; j++) {
      if (node->inputs[j].id == id)
        return &node->inputs[j];
    }
    for (u32 j = 0; j < node->output_count; j++) {
      if (node->outputs[j].id == id)
        return &node->outputs[j];
    }
  }
  return NULL;
}

// ----------------------------------------------------------------------------
// Graph Management
// ----------------------------------------------------------------------------

VSGraph *vs_graph_create(void) {
  VSGraph *graph = (VSGraph *)MALLOC(sizeof(VSGraph));
  memset(graph, 0, sizeof(VSGraph));

  graph->node_capacity = 64;
  graph->nodes = (VSNode *)CALLOC(graph->node_capacity, sizeof(VSNode));

  graph->link_capacity = 128;
  graph->links = (VSLink *)CALLOC(graph->link_capacity, sizeof(VSLink));

  return graph;
}

void vs_graph_destroy(VSGraph *graph) {
  if (!graph)
    return;
  if (graph->nodes)
    FREE(graph->nodes);
  if (graph->links)
    FREE(graph->links);
  FREE(graph);
}

VSNodeID vs_graph_add_node(VSGraph *graph, const char *title, u32 type_id) {
  if (graph->node_count >= graph->node_capacity)
    return 0xFFFFFFFF;

  VSNodeID id = graph->node_count++;
  VSNode *node = &graph->nodes[id];
  node->id = id;
  node->type_id = type_id;
  strncpy(node->title, title, 63);

  // Auto-generate pins based on type (Mock logic)
  // In reality this would come from a registry
  if (type_id == VS_NODE_PRINT) {
    // [In Flow] [In String] -> [Out Flow]
    node->inputs[0] = (VSPin){
        .id = (id << 16) | 1, .type = VS_PIN_FLOW, .direction = VS_PIN_INPUT};
    node->inputs[1] = (VSPin){
        .id = (id << 16) | 2, .type = VS_PIN_STRING, .direction = VS_PIN_INPUT};
    node->input_count = 2;

    node->outputs[0] = (VSPin){
        .id = (id << 16) | 3, .type = VS_PIN_FLOW, .direction = VS_PIN_OUTPUT};
    node->output_count = 1;
  } else if (type_id == VS_NODE_ENTRY) {
    // -> [Out Flow]
    node->outputs[0] = (VSPin){
        .id = (id << 16) | 1, .type = VS_PIN_FLOW, .direction = VS_PIN_OUTPUT};
    node->output_count = 1;
  }

  return id;
}

void vs_graph_connect(VSGraph *graph, VSPinID from, VSPinID to) {
  if (graph->link_count >= graph->link_capacity)
    return;

  VSLink *link = &graph->links[graph->link_count++];
  link->from_pin = from; // Output
  link->to_pin = to;     // Input

  VSPin *p1 = get_pin(graph, from);
  VSPin *p2 = get_pin(graph, to);
  if (p1)
    p1->is_connected = true;
  if (p2)
    p2->is_connected = true;
}

// ----------------------------------------------------------------------------
// Execution
// ----------------------------------------------------------------------------

static VSNode *follow_flow(VSGraph *graph, VSNode *current_node,
                           u32 flow_output_idx) {
  if (flow_output_idx >= current_node->output_count)
    return NULL;
  VSPinID out_pin_id = current_node->outputs[flow_output_idx].id;

  // Find link connected to this output
  for (u32 i = 0; i < graph->link_count; i++) {
    if (graph->links[i].from_pin == out_pin_id) {
      // Found execution link, find target node
      VSPin *target_pin = get_pin(graph, graph->links[i].to_pin);
      if (target_pin) {
        return get_node(graph, target_pin->node_id);
      }
    }
  }
  return NULL; // End of flow
}

void vs_graph_execute(VSGraph *graph, VSNodeID entry_node_id) {
  VSNode *current = get_node(graph, entry_node_id);

  printf("VS: Start execution at node %d (%s)\n", entry_node_id,
         current->title);

  // Simple interpreter loop
  while (current) {
    // Execute Node Logic
    if (current->type_id == VS_NODE_PRINT) {
            // In real logic, we'd pull inputs here
    }

    // Find next flow node (assuming Output 0 is always execution flow for now)
    if (current->output_count > 0 && current->outputs[0].type == VS_PIN_FLOW) {
      current = follow_flow(graph, current, 0);
    } else {
      current = NULL;
    }
  }

  printf("VS: Execution finished\n");
}
