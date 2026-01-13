/**
 * BEHAVIOR TREE SYSTEM
 * Modular behavioral AI logic
 */

#include <stdbool.h>
#include <stdlib.h>

typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;

typedef struct BTNode BTNode;

struct BTNode {
  BTStatus (*tick)(BTNode *self, void *context);
  void (*reset)(BTNode *self);

  BTNode **children;
  int child_count;
  int capacity;

  void *data;        // User data for leaf nodes
  int current_child; // For sequences/selectors
};

// --- Core Node Types ---

// Sequence: Run children in order. Fail if any fail. Success if all succeed.
BTStatus bt_sequence_tick(BTNode *self, void *context) {
  for (; self->current_child < self->child_count; self->current_child++) {
    BTStatus status = self->children[self->current_child]->tick(
        self->children[self->current_child], context);

    if (status == BT_RUNNING)
      return BT_RUNNING;
    if (status == BT_FAILURE) {
      self->current_child = 0; // Reset
      return BT_FAILURE;
    }
  }
  self->current_child = 0; // Reset
  return BT_SUCCESS;
}

// Selector: Run children in order. Success if any succeed. Fail if all fail.
BTStatus bt_selector_tick(BTNode *self, void *context) {
  for (; self->current_child < self->child_count; self->current_child++) {
    BTStatus status = self->children[self->current_child]->tick(
        self->children[self->current_child], context);

    if (status == BT_RUNNING)
      return BT_RUNNING;
    if (status == BT_SUCCESS) {
      self->current_child = 0; // Reset
      return BT_SUCCESS;
    }
  }
  self->current_child = 0; // Reset
  return BT_FAILURE;
}

// Helper: Create Node
BTNode *bt_create_node(BTStatus (*tick_func)(BTNode *, void *)) {
  BTNode *node = calloc(1, sizeof(BTNode));
  node->tick = tick_func;
  return node;
}

BTNode *bt_create_sequence() { return bt_create_node(bt_sequence_tick); }

BTNode *bt_create_selector() { return bt_create_node(bt_selector_tick); }

void bt_add_child(BTNode *parent, BTNode *child) {
  if (parent->child_count >= parent->capacity) {
    int new_cap = parent->capacity == 0 ? 4 : parent->capacity * 2;
    parent->children = realloc(parent->children, new_cap * sizeof(BTNode *));
    parent->capacity = new_cap;
  }
  parent->children[parent->child_count++] = child;
}

// --- Decorators ---

// Inverter
BTStatus bt_inverter_tick(BTNode *self, void *context) {
  if (self->child_count == 0)
    return BT_SUCCESS;
  BTStatus status = self->children[0]->tick(self->children[0], context);
  if (status == BT_SUCCESS)
    return BT_FAILURE;
  if (status == BT_FAILURE)
    return BT_SUCCESS;
  return BT_RUNNING;
}

// Succeeder
BTStatus bt_succeeder_tick(BTNode *self, void *context) {
  if (self->child_count == 0)
    return BT_SUCCESS;
  BTStatus status = self->children[0]->tick(self->children[0], context);
  if (status == BT_RUNNING)
    return BT_RUNNING;
  return BT_SUCCESS;
}

// --- Leaf Examples ---

// Wait Action
typedef struct {
  float duration;
  float elapsed;
} WaitData;

BTStatus bt_wait_tick(BTNode *self, void *context) {
  WaitData *data = (WaitData *)self->data;
  float dt = *(float *)context; // Context assumed to contain delta time

  data->elapsed += dt;
  if (data->elapsed >= data->duration) {
    data->elapsed = 0;
    return BT_SUCCESS;
  }
  return BT_RUNNING;
}

BTNode *bt_create_wait(float duration) {
  BTNode *node = bt_create_node(bt_wait_tick);
  WaitData *data = malloc(sizeof(WaitData));
  data->duration = duration;
  data->elapsed = 0;
  node->data = data;
  return node;
}

// --- Builder API ---
typedef struct {
  BTNode *root;
  BTNode *stack[32];
  int stack_top;
} BTBuilder;

void bt_builder_init(BTBuilder *b) {
  b->root = NULL;
  b->stack_top = -1;
}

void bt_builder_sequence(BTBuilder *b) {
  BTNode *node = bt_create_sequence();
  if (b->stack_top >= 0) {
    bt_add_child(b->stack[b->stack_top], node);
  } else {
    b->root = node;
  }
  b->stack[++b->stack_top] = node;
}

void bt_builder_selector(BTBuilder *b) {
  BTNode *node = bt_create_selector();
  if (b->stack_top >= 0) {
    bt_add_child(b->stack[b->stack_top], node);
  } else {
    b->root = node;
  }
  b->stack[++b->stack_top] = node;
}

void bt_builder_end(BTBuilder *b) {
  if (b->stack_top >= 0) {
    b->stack_top--;
  }
}

void bt_destroy(BTNode *node) {
  if (!node)
    return;
  for (int i = 0; i < node->child_count; i++) {
    bt_destroy(node->children[i]);
  }
  free(node->children);
  if (node->data)
    free(node->data);
  free(node);
}
