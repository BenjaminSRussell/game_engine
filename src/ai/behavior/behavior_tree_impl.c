/**
 * =================================================================================================
 *                          BEHAVIOR TREE IMPLEMENTATION
 *                          Phase 4: AI & Navigation
 * =================================================================================================
 *
 * PURPOSE: Behavior trees for AI decision making
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BT_MAX_CHILDREN 16
#define BT_MAX_NODES 256
#define BT_MAX_NAME 64

// Node execution result
typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;

// Node types
typedef enum {
  BT_COMPOSITE_SEQUENCE,   // Run children in order, fail on first failure
  BT_COMPOSITE_SELECTOR,   // Run children in order, succeed on first success
  BT_COMPOSITE_PARALLEL,   // Run all children simultaneously
  BT_DECORATOR_INVERTER,   // Invert child result
  BT_DECORATOR_REPEATER,   // Repeat child N times
  BT_DECORATOR_UNTIL_FAIL, // Repeat until failure
  BT_DECORATOR_UNTIL_SUCCESS,
  BT_DECORATOR_COOLDOWN, // Prevent execution for N seconds
  BT_LEAF_ACTION,        // Execute action
  BT_LEAF_CONDITION      // Check condition
} BTNodeType;

// Forward declaration
struct BTNode;
struct Blackboard;

// Callbacks
typedef BTStatus (*BTActionFunc)(struct BTNode *node, void *context, float dt);
typedef bool (*BTConditionFunc)(struct BTNode *node, void *context);

// Behavior tree node
typedef struct BTNode {
  char name[BT_MAX_NAME];
  BTNodeType type;
  uint32_t id;

  // Parent and children
  struct BTNode *parent;
  struct BTNode *children[BT_MAX_CHILDREN];
  uint32_t child_count;

  // Current state
  BTStatus last_status;
  uint32_t current_child; // For sequences/selectors
  bool is_running;

  // Decorator data
  uint32_t repeat_count;
  uint32_t repeat_current;
  float cooldown_time;
  float cooldown_remaining;

  // Parallel data
  uint32_t success_threshold; // How many must succeed
  uint32_t failure_threshold; // How many must fail

  // Leaf node callbacks
  BTActionFunc action;
  BTConditionFunc condition;

  // User data
  void *user_data;
  struct Blackboard *blackboard;
} BTNode;

// Blackboard for sharing data
typedef struct Blackboard {
  struct {
    char key[32];
    union {
      int int_val;
      float float_val;
      bool bool_val;
      void *ptr_val;
    } value;
    int type; // 0=int, 1=float, 2=bool, 3=ptr
  } entries[64];
  uint32_t entry_count;
} Blackboard;

// Behavior tree
typedef struct {
  BTNode *root;
  BTNode *nodes[BT_MAX_NODES];
  uint32_t node_count;
  Blackboard *blackboard;
  void *context;
} BehaviorTree;

// -----------------------------------------------------------------------------
// Blackboard
// -----------------------------------------------------------------------------

Blackboard *blackboard_create(void) {
  return (Blackboard *)calloc(1, sizeof(Blackboard));
}

void blackboard_destroy(Blackboard *bb) {
  if (bb)
    free(bb);
}

void blackboard_set_int(Blackboard *bb, const char *key, int value) {
  if (!bb)
    return;

  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0) {
      bb->entries[i].value.int_val = value;
      bb->entries[i].type = 0;
      return;
    }
  }

  if (bb->entry_count < 64) {
    strncpy(bb->entries[bb->entry_count].key, key, 31);
    bb->entries[bb->entry_count].value.int_val = value;
    bb->entries[bb->entry_count].type = 0;
    bb->entry_count++;
  }
}

void blackboard_set_float(Blackboard *bb, const char *key, float value) {
  if (!bb)
    return;

  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0) {
      bb->entries[i].value.float_val = value;
      bb->entries[i].type = 1;
      return;
    }
  }

  if (bb->entry_count < 64) {
    strncpy(bb->entries[bb->entry_count].key, key, 31);
    bb->entries[bb->entry_count].value.float_val = value;
    bb->entries[bb->entry_count].type = 1;
    bb->entry_count++;
  }
}

void blackboard_set_bool(Blackboard *bb, const char *key, bool value) {
  if (!bb)
    return;

  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0) {
      bb->entries[i].value.bool_val = value;
      bb->entries[i].type = 2;
      return;
    }
  }

  if (bb->entry_count < 64) {
    strncpy(bb->entries[bb->entry_count].key, key, 31);
    bb->entries[bb->entry_count].value.bool_val = value;
    bb->entries[bb->entry_count].type = 2;
    bb->entry_count++;
  }
}

int blackboard_get_int(Blackboard *bb, const char *key, int default_val) {
  if (!bb)
    return default_val;
  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0 && bb->entries[i].type == 0) {
      return bb->entries[i].value.int_val;
    }
  }
  return default_val;
}

float blackboard_get_float(Blackboard *bb, const char *key, float default_val) {
  if (!bb)
    return default_val;
  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0 && bb->entries[i].type == 1) {
      return bb->entries[i].value.float_val;
    }
  }
  return default_val;
}

bool blackboard_get_bool(Blackboard *bb, const char *key, bool default_val) {
  if (!bb)
    return default_val;
  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0 && bb->entries[i].type == 2) {
      return bb->entries[i].value.bool_val;
    }
  }
  return default_val;
}

// -----------------------------------------------------------------------------
// Node Creation
// -----------------------------------------------------------------------------

static BTNode *btnode_create(BTNodeType type, const char *name) {
  BTNode *node = (BTNode *)calloc(1, sizeof(BTNode));
  if (!node)
    return NULL;

  node->type = type;
  strncpy(node->name, name, BT_MAX_NAME - 1);
  node->last_status = BT_SUCCESS;
  node->success_threshold = 1;
  node->failure_threshold = 1;

  return node;
}

BTNode *bt_sequence(const char *name) {
  return btnode_create(BT_COMPOSITE_SEQUENCE, name);
}

BTNode *bt_selector(const char *name) {
  return btnode_create(BT_COMPOSITE_SELECTOR, name);
}

BTNode *bt_parallel(const char *name, uint32_t success_needed,
                    uint32_t failure_needed) {
  BTNode *node = btnode_create(BT_COMPOSITE_PARALLEL, name);
  if (node) {
    node->success_threshold = success_needed;
    node->failure_threshold = failure_needed;
  }
  return node;
}

BTNode *bt_inverter(const char *name) {
  return btnode_create(BT_DECORATOR_INVERTER, name);
}

BTNode *bt_repeater(const char *name, uint32_t times) {
  BTNode *node = btnode_create(BT_DECORATOR_REPEATER, name);
  if (node)
    node->repeat_count = times;
  return node;
}

BTNode *bt_cooldown(const char *name, float seconds) {
  BTNode *node = btnode_create(BT_DECORATOR_COOLDOWN, name);
  if (node)
    node->cooldown_time = seconds;
  return node;
}

BTNode *bt_action(const char *name, BTActionFunc func) {
  BTNode *node = btnode_create(BT_LEAF_ACTION, name);
  if (node)
    node->action = func;
  return node;
}

BTNode *bt_condition(const char *name, BTConditionFunc func) {
  BTNode *node = btnode_create(BT_LEAF_CONDITION, name);
  if (node)
    node->condition = func;
  return node;
}

void bt_add_child(BTNode *parent, BTNode *child) {
  if (!parent || !child)
    return;
  if (parent->child_count >= BT_MAX_CHILDREN)
    return;

  parent->children[parent->child_count++] = child;
  child->parent = parent;
}

// -----------------------------------------------------------------------------
// Tree Creation
// -----------------------------------------------------------------------------

BehaviorTree *bt_create(void *context) {
  BehaviorTree *tree = (BehaviorTree *)calloc(1, sizeof(BehaviorTree));
  if (!tree)
    return NULL;

  tree->blackboard = blackboard_create();
  tree->context = context;

  return tree;
}

void bt_destroy(BehaviorTree *tree) {
  if (!tree)
    return;

  for (uint32_t i = 0; i < tree->node_count; i++) {
    free(tree->nodes[i]);
  }
  blackboard_destroy(tree->blackboard);
  free(tree);
}

void bt_set_root(BehaviorTree *tree, BTNode *root) {
  if (!tree)
    return;
  tree->root = root;
  root->blackboard = tree->blackboard;
}

static void bt_register_node(BehaviorTree *tree, BTNode *node) {
  if (!tree || !node)
    return;
  if (tree->node_count >= BT_MAX_NODES)
    return;

  node->id = tree->node_count;
  node->blackboard = tree->blackboard;
  tree->nodes[tree->node_count++] = node;

  for (uint32_t i = 0; i < node->child_count; i++) {
    bt_register_node(tree, node->children[i]);
  }
}

void bt_build(BehaviorTree *tree) {
  if (!tree || !tree->root)
    return;
  bt_register_node(tree, tree->root);
}

// -----------------------------------------------------------------------------
// Node Execution
// -----------------------------------------------------------------------------

static BTStatus bt_tick_node(BTNode *node, void *context, float dt);

static BTStatus bt_tick_sequence(BTNode *node, void *context, float dt) {
  for (; node->current_child < node->child_count; node->current_child++) {
    BTStatus status =
        bt_tick_node(node->children[node->current_child], context, dt);

    if (status == BT_RUNNING) {
      return BT_RUNNING;
    }
    if (status == BT_FAILURE) {
      node->current_child = 0;
      return BT_FAILURE;
    }
  }

  node->current_child = 0;
  return BT_SUCCESS;
}

static BTStatus bt_tick_selector(BTNode *node, void *context, float dt) {
  for (; node->current_child < node->child_count; node->current_child++) {
    BTStatus status =
        bt_tick_node(node->children[node->current_child], context, dt);

    if (status == BT_RUNNING) {
      return BT_RUNNING;
    }
    if (status == BT_SUCCESS) {
      node->current_child = 0;
      return BT_SUCCESS;
    }
  }

  node->current_child = 0;
  return BT_FAILURE;
}

static BTStatus bt_tick_parallel(BTNode *node, void *context, float dt) {
  uint32_t success_count = 0;
  uint32_t failure_count = 0;

  for (uint32_t i = 0; i < node->child_count; i++) {
    BTStatus status = bt_tick_node(node->children[i], context, dt);

    if (status == BT_SUCCESS)
      success_count++;
    else if (status == BT_FAILURE)
      failure_count++;
  }

  if (success_count >= node->success_threshold)
    return BT_SUCCESS;
  if (failure_count >= node->failure_threshold)
    return BT_FAILURE;

  return BT_RUNNING;
}

static BTStatus bt_tick_node(BTNode *node, void *context, float dt) {
  if (!node)
    return BT_FAILURE;

  node->is_running = true;

  switch (node->type) {
  case BT_COMPOSITE_SEQUENCE:
    node->last_status = bt_tick_sequence(node, context, dt);
    break;

  case BT_COMPOSITE_SELECTOR:
    node->last_status = bt_tick_selector(node, context, dt);
    break;

  case BT_COMPOSITE_PARALLEL:
    node->last_status = bt_tick_parallel(node, context, dt);
    break;

  case BT_DECORATOR_INVERTER:
    if (node->child_count > 0) {
      BTStatus child_status = bt_tick_node(node->children[0], context, dt);
      if (child_status == BT_SUCCESS)
        node->last_status = BT_FAILURE;
      else if (child_status == BT_FAILURE)
        node->last_status = BT_SUCCESS;
      else
        node->last_status = BT_RUNNING;
    }
    break;

  case BT_DECORATOR_REPEATER:
    if (node->child_count > 0) {
      BTStatus status = bt_tick_node(node->children[0], context, dt);
      if (status != BT_RUNNING) {
        node->repeat_current++;
        if (node->repeat_current >= node->repeat_count) {
          node->repeat_current = 0;
          node->last_status = BT_SUCCESS;
        } else {
          node->last_status = BT_RUNNING;
        }
      } else {
        node->last_status = BT_RUNNING;
      }
    }
    break;

  case BT_DECORATOR_COOLDOWN:
    if (node->cooldown_remaining > 0) {
      node->cooldown_remaining -= dt;
      node->last_status = BT_FAILURE;
    } else if (node->child_count > 0) {
      BTStatus status = bt_tick_node(node->children[0], context, dt);
      if (status != BT_RUNNING) {
        node->cooldown_remaining = node->cooldown_time;
      }
      node->last_status = status;
    }
    break;

  case BT_LEAF_ACTION:
    if (node->action) {
      node->last_status = node->action(node, context, dt);
    } else {
      node->last_status = BT_FAILURE;
    }
    break;

  case BT_LEAF_CONDITION:
    if (node->condition) {
      node->last_status =
          node->condition(node, context) ? BT_SUCCESS : BT_FAILURE;
    } else {
      node->last_status = BT_FAILURE;
    }
    break;

  default:
    node->last_status = BT_FAILURE;
  }

  if (node->last_status != BT_RUNNING) {
    node->is_running = false;
  }

  return node->last_status;
}

// -----------------------------------------------------------------------------
// Tree Execution
// -----------------------------------------------------------------------------

BTStatus bt_tick(BehaviorTree *tree, float dt) {
  if (!tree || !tree->root)
    return BT_FAILURE;
  return bt_tick_node(tree->root, tree->context, dt);
}

void bt_reset(BehaviorTree *tree) {
  if (!tree)
    return;

  for (uint32_t i = 0; i < tree->node_count; i++) {
    BTNode *node = tree->nodes[i];
    node->current_child = 0;
    node->is_running = false;
    node->repeat_current = 0;
    node->cooldown_remaining = 0;
  }
}
