#include "scripting_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VISUAL SCRIPTING IMPLEMENTATION
 * ============================================================================
 */

static VSNode *get_node(VSGraph *graph, VSNodeID id) {
  if (!graph || id >= graph->node_count)
    return NULL;
  return &graph->nodes[id];
}

static VSPin *get_pin(VSGraph *graph, VSPinID id) {
  // Search all nodes for the pin
  for (uint32_t i = 0; i < graph->node_count; i++) {
    VSNode *node = &graph->nodes[i];

    // Check inputs
    for (uint32_t j = 0; j < node->input_count; j++) {
      if (node->inputs[j].id == id) {
        return &node->inputs[j];
      }
    }

    // Check outputs
    for (uint32_t j = 0; j < node->output_count; j++) {
      if (node->outputs[j].id == id) {
        return &node->outputs[j];
      }
    }
  }

  return NULL;
}

VSGraph *vs_graph_create(void) {
  VSGraph *graph = UNIFIED_ALLOC(sizeof(VSGraph));
  if (!graph) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate visual scripting graph");
    return NULL;
  }

  memset(graph, 0, sizeof(VSGraph));

  graph->node_capacity = 64;
  graph->nodes = UNIFIED_ALLOC(graph->node_capacity * sizeof(VSNode));

  graph->link_capacity = 128;
  graph->links = UNIFIED_ALLOC(graph->link_capacity * sizeof(VSLink));

  if (!graph->nodes || !graph->links) {
    vs_graph_destroy(graph);
    return NULL;
  }

  memset(graph->nodes, 0, graph->node_capacity * sizeof(VSNode));
  memset(graph->links, 0, graph->link_capacity * sizeof(VSLink));

  LOG_INFO(LOG_CAT_GENERAL, "Visual scripting graph created");
  return graph;
}

void vs_graph_destroy(VSGraph *graph) {
  if (!graph)
    return;

  if (graph->nodes)
    UNIFIED_FREE(graph->nodes);
  if (graph->links)
    UNIFIED_FREE(graph->links);

  UNIFIED_FREE(graph);
}

VSNodeID vs_graph_add_node(VSGraph *graph, const char *title, VSNodeType type) {
  if (!graph)
    return 0xFFFFFFFF;
  if (graph->node_count >= graph->node_capacity) {
    LOG_ERROR(LOG_CAT_GENERAL, "Node capacity reached");
    return 0xFFFFFFFF;
  }

  VSNodeID id = graph->node_count++;
  VSNode *node = &graph->nodes[id];

  node->id = id;
  node->type_id = type;
  strncpy(node->title, title, sizeof(node->title) - 1);

  // Auto-generate pins based on type
  if (type == VS_NODE_PRINT) {
    // Input: Flow, String  => Output: Flow
    node->inputs[0] = (VSPin){.id = (id << 16) | 1,
                              .type = VS_PIN_FLOW,
                              .direction = VS_PIN_INPUT,
                              .node_id = id,
                              .is_connected = false};
    node->inputs[1] = (VSPin){.id = (id << 16) | 2,
                              .type = VS_PIN_STRING,
                              .direction = VS_PIN_INPUT,
                              .node_id = id,
                              .is_connected = false};
    node->input_count = 2;

    node->outputs[0] = (VSPin){.id = (id << 16) | 3,
                               .type = VS_PIN_FLOW,
                               .direction = VS_PIN_OUTPUT,
                               .node_id = id,
                               .is_connected = false};
    node->output_count = 1;
  } else if (type == VS_NODE_ENTRY) {
    // Entry has only an output flow
    node->outputs[0] = (VSPin){.id = (id << 16) | 1,
                               .type = VS_PIN_FLOW,
                               .direction = VS_PIN_OUTPUT,
                               .node_id = id,
                               .is_connected = false};
    node->output_count = 1;
  }

  return id;
}

void vs_graph_connect(VSGraph *graph, VSPinID from_pin, VSPinID to_pin) {
  if (!graph)
    return;
  if (graph->link_count >= graph->link_capacity) {
    LOG_ERROR(LOG_CAT_GENERAL, "Link capacity reached");
    return;
  }

  VSLink *link = &graph->links[graph->link_count++];
  link->from_pin = from_pin;
  link->to_pin = to_pin;

  VSPin *p1 = get_pin(graph, from_pin);
  VSPin *p2 = get_pin(graph, to_pin);

  if (p1)
    p1->is_connected = true;
  if (p2)
    p2->is_connected = true;
}

static VSNode *follow_flow(VSGraph *graph, VSNode *current_node,
                           uint32_t flow_output_idx) {
  if (!current_node || flow_output_idx >= current_node->output_count)
    return NULL;

  VSPinID out_pin_id = current_node->outputs[flow_output_idx].id;

  // Find link connected to this output
  for (uint32_t i = 0; i < graph->link_count; i++) {
    if (graph->links[i].from_pin == out_pin_id) {
      VSPin *target_pin = get_pin(graph, graph->links[i].to_pin);
      if (target_pin) {
        return get_node(graph, target_pin->node_id);
      }
    }
  }

  return NULL;
}

void vs_graph_execute(VSGraph *graph, VSNodeID entry_node_id) {
  if (!graph)
    return;

  VSNode *current = get_node(graph, entry_node_id);
  if (!current) {
    LOG_ERROR(LOG_CAT_GENERAL, "Invalid entry node ID");
    return;
  }

  LOG_INFO(LOG_CAT_GENERAL, "VS: Start execution at node %u (%s)",
           entry_node_id, current->title);

  // Simple interpreter loop
  while (current) {
    // Execute node logic based on type
    if (current->type_id == VS_NODE_PRINT) {
      LOG_INFO(LOG_CAT_GENERAL, "VS: Execute PRINT node");
    }

    // Follow flow to next node
    if (current->output_count > 0 && current->outputs[0].type == VS_PIN_FLOW) {
      current = follow_flow(graph, current, 0);
    } else {
      current = NULL;
    }
  }

  LOG_INFO(LOG_CAT_GENERAL, "VS: Execution finished");
}
