#ifndef ENGINE_VISUAL_SCRIPTING_CORE_H
#define ENGINE_VISUAL_SCRIPTING_CORE_H

#include "common.h"
#include "core/logger.h"
#include <stdbool.h>

typedef struct VSContext VSContext;
typedef struct VSNode VSNode;

typedef enum VSPinType {
  VS_PIN_EXECUTION,
  VS_PIN_FLOAT,
  VS_PIN_INT,
  VS_PIN_BOOL,
  VS_PIN_STRING,
  VS_PIN_OBJECT
} VSPinType;

typedef struct VSPin {
  const char *name;
  VSPinType type;
  void *data; // Default value or connection ptr
  struct VSNode *node;
  bool is_output;
  struct VSPin *connected_to; // Simple 1:1 for now, should be list for output
} VSPin;

typedef struct VSNode {
  u32 id;
  const char *title;
  VSPin *inputs;
  u32 input_count;
  VSPin *outputs;
  u32 output_count;

  // Execution callback
  void (*execute)(struct VSContext *ctx, struct VSNode *node);

  void *node_data;
} VSNode;

typedef struct VSGraph {
  VSNode **nodes;
  u32 node_count;
  u32 capacity;
  // Variable storage
} VSGraph;

typedef struct VSContext {
  VSGraph *graph;
  VSNode *current_node;
  // Runtime variables
  bool is_running;
} VSContext;

// API
VSGraph *vs_graph_create(void);
void vs_graph_destroy(VSGraph *graph);
VSNode *vs_graph_add_node(VSGraph *graph, const char *type);
void vs_graph_connect(VSPin *out, VSPin *in);

void vs_execute_graph(VSGraph *graph);

// Node registration
typedef VSNode *(*VSNodeFactory)(void);
void vs_register_node_type(const char *type_name, VSNodeFactory factory);

#endif // ENGINE_VISUAL_SCRIPTING_CORE_H
