#ifndef VISUAL_SCRIPTING_H
#define VISUAL_SCRIPTING_H

#include "../common.h"
#include "../math/vec3.h"

// ----------------------------------------------------------------------------
// Types & Enums
// ----------------------------------------------------------------------------

typedef enum {
  VS_PIN_FLOW, // Execution flow
  VS_PIN_BOOL,
  VS_PIN_INT,
  VS_PIN_FLOAT,
  VS_PIN_VEC3,
  VS_PIN_ENTITY,
  VS_PIN_STRING
} VSPinType;

typedef enum { VS_PIN_INPUT, VS_PIN_OUTPUT } VSPinDirection;

typedef u32 VSNodeID;
typedef u32 VSPinID;

// ----------------------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------------------

typedef struct {
  VSPinID id;
  VSNodeID node_id;
  char name[32];
  VSPinType type;
  VSPinDirection direction;

  // Default value storage (primitive union)
  union {
    bool b_val;
    int i_val;
    float f_val;
    Vec3 v3_val;
  } default_value;

  bool is_connected;
} VSPin;

typedef struct {
  VSNodeID id;
  char title[64];
  Vec3 position; // Editor canvas position

  // For V1, fixed size generic arrays. In prod, dynamic list.
  VSPin inputs[8];
  u32 input_count;

  VSPin outputs[8];
  u32 output_count;

  // Function pointer for execution
  // void (*execute)(struct VSGraph* graph, struct VSNode* node);
  u32 type_id; // For dispatch
} VSNode;

typedef struct {
  VSPinID from_pin; // Must be output
  VSPinID to_pin;   // Must be input
} VSLink;

typedef struct {
  VSNode *nodes;
  u32 node_count;
  u32 node_capacity;

  VSLink *links;
  u32 link_count;
  u32 link_capacity;

  // Runtime state
  VSNodeID current_node;
} VSGraph;

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

VSGraph *vs_graph_create(void);
void vs_graph_destroy(VSGraph *graph);

VSNodeID vs_graph_add_node(VSGraph *graph, const char *title, u32 type_id);
void vs_graph_connect(VSGraph *graph, VSPinID from, VSPinID to);

// Execution
void vs_graph_execute(VSGraph *graph, VSNodeID entry_node);

// Node Registration (Mock)
#define VS_NODE_ENTRY 0
#define VS_NODE_PRINT 1
#define VS_NODE_ADD_FLOAT 2
#define VS_NODE_SET_TRANSFORM 3

#endif // VISUAL_SCRIPTING_H
