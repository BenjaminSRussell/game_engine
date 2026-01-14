/**
 * @file pcg_graph.c
 * @brief PCG Graph Implementation
 * @description Procedural Content Generation graph system
 * @date 2026-01-13
 */

#include "engine/include/pcg/pcg_graph.h"
#include "engine/include/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Internal Helpers
// ----------------------------------------------------------------------------

static pcg_data_collection_t *pcg_data_create(u32 capacity) {
  pcg_data_collection_t *data =
      (pcg_data_collection_t *)malloc(sizeof(pcg_data_collection_t));
  if (capacity > 0) {
    data->points = (pcg_point_t *)malloc(sizeof(pcg_point_t) * capacity);
    data->point_capacity = capacity;
  } else {
    data->points = NULL;
    data->point_capacity = 0;
  }
  data->point_count = 0;
  return data;
}

void pcg_data_free(pcg_data_collection_t *data) {
  if (data) {
    if (data->points)
      free(data->points);
    free(data);
  }
}

// node execution functions
static void exec_node_surface_sampler(const pcg_node_t *node,
                                      const pcg_data_collection_t *input,
                                      pcg_data_collection_t *output) {
  // Mock implementation: Generate grid of points
  // float_params[0] = extent x, [1] = extent z, [2] = points per meter
  float ext_x = node->float_params[0];
  float ext_z = node->float_params[1];
  float density = node->float_params[2];
  if (density <= 0.0f)
    density = 1.0f;

  int count_x = (int)(ext_x * density);
  int count_z = (int)(ext_z * density);

  // Resize output
  u32 total = count_x * count_z;
  if (output->point_capacity < total) {
    output->points =
        (pcg_point_t *)realloc(output->points, sizeof(pcg_point_t) * total);
    output->point_capacity = total;
  }

  output->point_count = 0;
  for (int x = 0; x < count_x; x++) {
    for (int z = 0; z < count_z; z++) {
      pcg_point_t *p = &output->points[output->point_count++];
      p->position.x = (x / density) - (ext_x * 0.5f);
      p->position.y =
          0.0f; // Raycasting would happen here in real implementation
      p->position.z = (z / density) - (ext_z * 0.5f);

      p->scale.x = 1.0f;
      p->scale.y = 1.0f;
      p->scale.z = 1.0f;
      p->rotation.x = 0;
      p->rotation.y = 0;
      p->rotation.z = 0;
      p->rotation.w = 1;
      p->density = 1.0f;
      p->seed = x * 73856093 ^ z * 19349663;
    }
  }
}

static void exec_node_density_filter(const pcg_node_t *node,
                                     const pcg_data_collection_t *input,
                                     pcg_data_collection_t *output) {
  // Filter points based on density threshold
  // float_params[0] = lower bound, [1] = upper bound
  float lower = node->float_params[0];
  float upper = node->float_params[1];

  // conservative resize
  if (output->point_capacity < input->point_count) {
    output->points = (pcg_point_t *)realloc(
        output->points, sizeof(pcg_point_t) * input->point_count);
    output->point_capacity = input->point_count;
  }

  output->point_count = 0;
  for (u32 i = 0; i < input->point_count; i++) {
    float d = input->points[i].density;
    // Basic noise modulation could happen here to vary density
    if (d >= lower && d <= upper) {
      output->points[output->point_count++] = input->points[i];
    }
  }
}

static void exec_node_transform(const pcg_node_t *node,
                                const pcg_data_collection_t *input,
                                pcg_data_collection_t *output) {
  // Apply Transform
  // float_params: 0,1,2 = translation add
  float tx = node->float_params[0];
  float ty = node->float_params[1];
  float tz = node->float_params[2];

  if (output->point_capacity < input->point_count) {
    output->points = (pcg_point_t *)realloc(
        output->points, sizeof(pcg_point_t) * input->point_count);
    output->point_capacity = input->point_count;
  }

  output->point_count = 0;
  for (u32 i = 0; i < input->point_count; i++) {
    pcg_point_t p = input->points[i];
    p.position.x += tx;
    p.position.y += ty;
    p.position.z += tz;
    output->points[output->point_count++] = p;
  }
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

pcg_graph_t *pcg_graph_create(void) {
  pcg_graph_t *graph = (pcg_graph_t *)calloc(1, sizeof(pcg_graph_t));
  return graph;
}

void pcg_graph_destroy(pcg_graph_t *graph) {
  if (graph) {
    if (graph->nodes)
      free(graph->nodes);
    if (graph->edges)
      free(graph->edges);
    free(graph);
  }
}

u32 pcg_graph_add_node(pcg_graph_t *graph, pcg_node_type_e type) {
  if (graph->node_count + 1 > graph->node_capacity) {
    u32 new_cap = (graph->node_capacity == 0) ? 8 : graph->node_capacity * 2;
    graph->nodes =
        (pcg_node_t *)realloc(graph->nodes, sizeof(pcg_node_t) * new_cap);
    graph->node_capacity = new_cap;
  }

  pcg_node_t *node = &graph->nodes[graph->node_count];
  node->id = graph->node_count; // Simple ID for execution order
  node->type = type;
  memset(node->float_params, 0, sizeof(node->float_params));
  memset(node->name, 0, 64);

  return graph->node_count++;
}

void pcg_graph_connect(pcg_graph_t *graph, u32 from, u32 to) {
  if (graph->edge_count + 1 > graph->edge_capacity) {
    u32 new_cap = (graph->edge_capacity == 0) ? 8 : graph->edge_capacity * 2;
    graph->edges =
        (pcg_edge_t *)realloc(graph->edges, sizeof(pcg_edge_t) * new_cap);
    graph->edge_capacity = new_cap;
  }
  graph->edges[graph->edge_count].from_node = from;
  graph->edges[graph->edge_count].to_node = to;
  graph->edge_count++;
}

pcg_data_collection_t *pcg_graph_execute(pcg_graph_t *graph,
                                         const pcg_data_collection_t *inputs) {
  // V1 Implementation: Linear execution based on ID order
  // Assumes nodes were added in topological order.
  // Real implementation requires graph traversal.

  pcg_data_collection_t *current_data = pcg_data_create(0);

  // Copy inputs if any (for V1 we expect SurfaceSampler to generate usually)
  if (inputs && inputs->point_count > 0) {
    // deep copy inputs to current
    // ...
  }

  for (u32 i = 0; i < graph->node_count; i++) {
    pcg_node_t *node = &graph->nodes[i];
    pcg_data_collection_t *next_data = pcg_data_create(0);

    switch (node->type) {
    case PCG_NODE_SURFACE_SAMPLER:
      exec_node_surface_sampler(node, current_data, next_data);
      break;
    case PCG_NODE_DENSITY_FILTER:
      exec_node_density_filter(node, current_data, next_data);
      break;
    case PCG_NODE_TRANSFORM_POINTS:
      exec_node_transform(node, current_data, next_data);
      break;
    default:
      // Pass through
      if (next_data->point_capacity < current_data->point_count) {
        next_data->points = (pcg_point_t *)realloc(
            next_data->points, sizeof(pcg_point_t) * current_data->point_count);
        next_data->point_capacity = current_data->point_count;
      }
      memcpy(next_data->points, current_data->points,
             sizeof(pcg_point_t) * current_data->point_count);
      next_data->point_count = current_data->point_count;
      break;
    }

    // Swap buffers
    pcg_data_free(current_data);
    current_data = next_data;
  }

  return current_data;
}
