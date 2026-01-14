// materials/material_graph.h
// Node-based material authoring (like UE5 Material Editor)
#ifndef MATERIAL_GRAPH_H
#define MATERIAL_GRAPH_H

#include <common.h>
#include "include/materials/pbr_material.h"
#include "math/vec4.h"

#define MAX_GRAPH_NODES 256
#define MAX_NODE_INPUTS 8
#define MAX_NODE_OUTPUTS 4

typedef enum {
  // Input nodes
  NODE_TEXTURE_SAMPLE,
  NODE_VERTEX_COLOR,
  NODE_TIME,
  NODE_CAMERA_VECTOR,
  NODE_WORLD_POSITION,
  NODE_VERTEX_NORMAL,
  NODE_CONSTANT,

  // Math nodes
  NODE_ADD,
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,
  NODE_POWER,
  NODE_SIN,
  NODE_COS,
  NODE_LERP,
  NODE_DOT_PRODUCT,
  NODE_CROSS_PRODUCT,
  NODE_NORMALIZE,

  // Utility nodes
  NODE_IF,
  NODE_SWITCH,
  NODE_FRESNEL,
  NODE_NOISE,
  NODE_VORONOI,

  // Output nodes
  NODE_MATERIAL_OUTPUT
} MaterialNodeType;

typedef struct MaterialGraphNode MaterialGraphNode;

typedef struct {
  MaterialGraphNode *source_node;
  u32 source_output_index;
  Vec4 default_value;
  bool is_connected;
} NodeInput;

typedef struct {
  Vec4 value;
  char semantic[32]; // e.g., "BaseColor", "Normal", "Metallic"
} NodeOutput;

struct MaterialGraphNode {
  u32 id;
  MaterialNodeType type;
  char name[64];

  NodeInput inputs[MAX_NODE_INPUTS];
  NodeOutput outputs[MAX_NODE_OUTPUTS];
  u32 input_count;
  u32 output_count;

  // Type-specific data
  union {
    struct {
      id<MTLTexture> texture;
      Vec2 uv_scale;
      Vec2 uv_offset;
    } texture_sample;

    struct {
      Vec4 value;
    } constant;

    struct {
      f32 frequency;
      f32 amplitude;
    } noise;
  } data;

  bool evaluated;
  bool is_output_node;
};

typedef struct {
  MaterialGraphNode nodes[MAX_GRAPH_NODES];
  u32 node_count;

  MaterialGraphNode *output_node;

  char name[128];
  char shader_code[8192]; // Generated HLSL/Metal code
  bool needs_recompile;

} MaterialGraph;

#ifdef __cplusplus
extern "C" {
#endif

// Graph management
MaterialGraph *material_graph_create(const char *name);
void material_graph_destroy(MaterialGraph *graph);

// Node creation
u32 material_graph_add_node(MaterialGraph *graph, MaterialNodeType type,
                            const char *name);
void material_graph_remove_node(MaterialGraph *graph, u32 node_id);

// Connections
bool material_graph_connect(MaterialGraph *graph, u32 source_node_id,
                            u32 source_output, u32 dest_node_id,
                            u32 dest_input);
void material_graph_disconnect(MaterialGraph *graph, u32 dest_node_id,
                               u32 dest_input);

// Node configuration
void material_graph_set_texture(MaterialGraph *graph, u32 node_id,
                                id<MTLTexture> texture);
void material_graph_set_constant(MaterialGraph *graph, u32 node_id, Vec4 value);

// Compilation
bool material_graph_compile(MaterialGraph *graph, PBRMaterial *output_material);
const char *material_graph_generate_shader(MaterialGraph *graph);

// Evaluation (for preview/CPU)
Vec4 material_graph_evaluate_node(MaterialGraph *graph, u32 node_id);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_GRAPH_H
