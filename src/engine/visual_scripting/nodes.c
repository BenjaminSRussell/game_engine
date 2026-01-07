#include <visual_scripting/visual_scripting_core.h>
#include <stdio.h>
#include <stdlib.h>

// Log Node
void node_log_execute(VSContext *ctx, VSNode *node) {
  // Get input string
  VSPin *msg_pin = &node->inputs[1]; // Assuming index 1
  const char *msg = "Default Log";
  // Logic to resolve inputs...

  LOG_INFO("VS Log: %s", msg);

  // Flow to next
  VSPin *exec_out = &node->outputs[0];
  if (exec_out->connected_to) {
    ctx->current_node = exec_out->connected_to->node;
  } else {
    ctx->current_node = NULL;
  }
}

VSNode *create_log_node(void) {
  VSNode *node = calloc(1, sizeof(VSNode));
  node->title = "Log Message";
  node->execute = node_log_execute;

  node->input_count = 2;
  node->inputs = calloc(2, sizeof(VSPin));
  node->inputs[0] = (VSPin){"In", VS_PIN_EXECUTION, NULL, node, false, NULL};
  node->inputs[1] = (VSPin){"Message", VS_PIN_STRING, NULL, node, false, NULL};

  node->output_count = 1;
  node->outputs = calloc(1, sizeof(VSPin));
  node->outputs[0] = (VSPin){"Out", VS_PIN_EXECUTION, NULL, node, true, NULL};

  return node;
}

// Math Add Node (Float)
void node_add_execute(VSContext *ctx, VSNode *node) {
  // Pure data node, executed on demand usually?
  // Or execution flow triggers calc.
  // For now simple flow:

  // VSPin *a = &node->inputs[0];
  // VSPin *b = &node->inputs[1];

  // f32 res = a_val + b_val
  // Write to output pin data

  // No exec flow change usually for pure nodes
}

// Register standard nodes
void vs_register_standard_nodes(void) {
  vs_register_node_type("Core/Log", create_log_node);
  // vs_register_node_type("Math/Add", create_add_node);
}
