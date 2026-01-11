#ifndef PCG_GRAPH_H
#define PCG_GRAPH_H

#include "../common.h"
#include "../math/quat.h"
#include "../math/vec3.h"

// ----------------------------------------------------------------------------
// Point Data (The primary data flowing through the graph)
// ----------------------------------------------------------------------------

typedef struct pcg_point_t {
  Vec3 position;
  Quat rotation;
  Vec3 scale;
  float density; // 0.0 - 1.0 probability
  int seed;
  float bounds_min[3];
  float bounds_max[3];
  // Attribute database index could go here for dynamic attributes
} pcg_point_t;

typedef struct pcg_data_collection_t {
  pcg_point_t *points;
  u32 point_count;
  u32 point_capacity;
} pcg_data_collection_t;

// ----------------------------------------------------------------------------
// Nodes
// ----------------------------------------------------------------------------

typedef enum pcg_node_type_e {
  PCG_NODE_INPUT_POINTS,
  PCG_NODE_SURFACE_SAMPLER,
  PCG_NODE_DENSITY_FILTER,
  PCG_NODE_TRANSFORM_POINTS,
  PCG_NODE_STATIC_MESH_SPAWNER,
  PCG_NODE_DIFFERENCE,
  PCG_NODE_UNION,
  PCG_NODE_METADATA
} pcg_node_type_e;

typedef struct pcg_node_t {
  u32 id;
  pcg_node_type_e type;
  char name[64];

  // Node-specific settings would be a union or void* here.
  // keeping it simple for V1 with inline generic params
  float float_params[4];
  int int_params[4];
  char string_param[64];

} pcg_node_t;

// ----------------------------------------------------------------------------
// Graph
// ----------------------------------------------------------------------------

typedef struct pcg_edge_t {
  u32 from_node;
  u32 to_node;
} pcg_edge_t;

typedef struct pcg_graph_t {
  pcg_node_t *nodes;
  u32 node_count;
  u32 node_capacity;

  pcg_edge_t *edges;
  u32 edge_count;
  u32 edge_capacity;

} pcg_graph_t;

// Public API
pcg_graph_t *pcg_graph_create(void);
void pcg_graph_destroy(pcg_graph_t *graph);
u32 pcg_graph_add_node(pcg_graph_t *graph, pcg_node_type_e type);
void pcg_graph_connect(pcg_graph_t *graph, u32 from, u32 to);
pcg_data_collection_t *pcg_graph_execute(pcg_graph_t *graph,
                                         const pcg_data_collection_t *inputs);
void pcg_data_free(pcg_data_collection_t *data);

#endif // PCG_GRAPH_H
