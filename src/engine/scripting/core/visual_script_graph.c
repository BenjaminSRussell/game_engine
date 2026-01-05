/**
 * @file visual_script_graph.c
 * @brief Visual Scripting Backend (Node Graph).
 *
 * Executes logic defined by node connections (Blueprint style).
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <scripting/core/visual_script_graph.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef enum NodeType {
  NODE_EVENT_BEGIN_PLAY,
  NODE_EVENT_TICK,
  NODE_FUNCTION_CALL,
  NODE_VARIABLE_GET,
  NODE_VARIABLE_SET,
  NODE_BRANCH,
  NODE_MATH_ADD
} NodeType;

typedef struct Pin {
  int id;
  enum { PIN_EXEC, PIN_DATA } type;
  enum { DIR_IN, DIR_OUT } direction;
  int connected_to_node_id;
  int connected_to_pin_id;
  // ... value ...
} Pin;

typedef struct Node {
  int id;
  NodeType type;
  Pin pins[8];
  int pin_count;
  // ... function ptr or data ...
} Node;

typedef struct Graph {
  Node nodes[1024];
  int node_count;
} Graph;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Executes a node and returns the next node to run.
 */
Node *graph_execute_node(Graph *g, Node *current, float dt) {
  switch (current->type) {
  case NODE_EVENT_TICK:
    // Find Output Exec Pin
    // Return connected node
    break;

  case NODE_BRANCH:
    // Evaluate Bool Input Pin
    // If True -> Return True Exec Node
    // If False -> Return False Exec Node
    break;

  case NODE_MATH_ADD:
    // Read A, B
    // Write Result = A + B
    // Pure node, no exec flow
    break;

  case NODE_FUNCTION_CALL:
    // Call C function wrapper
    // Return Next Exec
    break;
  }
  return NULL; // End of flow
}

void graph_tick(Graph *g, float dt) {
  // Find Tick Event
  Node *current = NULL; // ... find NODE_EVENT_TICK ...

  while (current) {
    current = graph_execute_node(g, current, dt);
  }
}
