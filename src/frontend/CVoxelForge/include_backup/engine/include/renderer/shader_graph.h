#ifndef ENGINE_SHADER_GRAPH_H
#define ENGINE_SHADER_GRAPH_H

#include <core/engine.h>
#include <stdbool.h>

typedef enum SGNodeType {
  SG_NODE_FLOAT,
  SG_NODE_VEC3,
  SG_NODE_TEXTURE,
  SG_NODE_ADD,
  SG_NODE_MULTIPLY,
  SG_NODE_MIX,
  SG_NODE_OUTPUT
} SGNodeType;

typedef struct SGPin {
  const char *name;
  bool is_output;
  struct SGNode *node;
  struct SGPin *connected_to;
} SGPin;

typedef struct SGNode {
  u32 id;
  SGNodeType type;
  f32 position[2]; // Editor position

  SGPin *inputs;
  u32 input_count;
  SGPin *outputs;
  u32 output_count;

  // Constant values if not connected
  union {
    f32 flt;
    f32 vec3[3];
    char *texture_path;
  } data;
} SGNode;

typedef struct SGGraph {
  SGNode **nodes;
  u32 node_count;
  u32 capacity;
} SGGraph;

// API
SGGraph *sg_create_graph(void);
void sg_destroy_graph(SGGraph *graph);
SGNode *sg_add_node(SGGraph *graph, SGNodeType type);
void sg_connect(SGPin *out, SGPin *in);

// Compilation
char *sg_compile_to_glsl(SGGraph *graph);
char *sg_compile_to_spirv(SGGraph *graph, u32 *out_size);

#endif // ENGINE_SHADER_GRAPH_H
