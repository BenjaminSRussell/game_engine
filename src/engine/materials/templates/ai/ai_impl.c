/**
 * =================================================================================================
 *                              AI BEHAVIOR TREE - IMPLEMENTATION
 *                              Agent: AGENT_AI_1
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum NodeType {
  NODE_SEQUENCE,
  NODE_SELECTOR,
  NODE_PARALLEL,
  NODE_DECORATOR,
  NODE_ACTION,
  NODE_CONDITION,
} NodeType;

typedef enum NodeStatus {
  STATUS_SUCCESS,
  STATUS_FAILURE,
  STATUS_RUNNING,
} NodeStatus;

typedef struct BTNode {
  NodeType type;
  char name[64];

  struct BTNode **children;
  uint32_t child_count;
  uint32_t child_capacity;

  NodeStatus (*execute)(struct BTNode *node, void *context);
  void *user_data;

  NodeStatus last_status;
  uint32_t current_child;
} BTNode;

typedef struct BehaviorTree {
  BTNode *root;
  void *context;
  bool is_running;
} BehaviorTree;

/* =================================================================================================
 *                                    NODE CREATION
 * =================================================================================================
 */

// DONE: Implement bt_node_create
BTNode *bt_node_create(NodeType type, const char *name) {
  BTNode *node = calloc(1, sizeof(BTNode));
  node->type = type;
  if (name)
    strncpy(node->name, name, 63);
  node->child_capacity = 4;
  node->children = calloc(node->child_capacity, sizeof(BTNode *));
  return node;
}

// DONE: Implement bt_node_destroy
void bt_node_destroy(BTNode *node) {
  if (!node)
    return;

  for (uint32_t i = 0; i < node->child_count; i++) {
    bt_node_destroy(node->children[i]);
  }
  free(node->children);
  free(node);
}

// DONE: Implement bt_node_add_child
void bt_node_add_child(BTNode *parent, BTNode *child) {
  if (!parent || !child)
    return;

  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity *= 2;
    parent->children =
        realloc(parent->children, parent->child_capacity * sizeof(BTNode *));
  }

  parent->children[parent->child_count++] = child;
}

/* =================================================================================================
 *                                    COMPOSITE NODES
 * =================================================================================================
 */

// DONE: Implement bt_sequence_execute
static NodeStatus bt_sequence_execute(BTNode *node, void *context) {
  for (uint32_t i = node->current_child; i < node->child_count; i++) {
    NodeStatus status = node->children[i]->execute(node->children[i], context);

    if (status == STATUS_RUNNING) {
      node->current_child = i;
      return STATUS_RUNNING;
    }

    if (status == STATUS_FAILURE) {
      node->current_child = 0;
      return STATUS_FAILURE;
    }
  }

  node->current_child = 0;
  return STATUS_SUCCESS;
}

// DONE: Implement bt_selector_execute
static NodeStatus bt_selector_execute(BTNode *node, void *context) {
  for (uint32_t i = node->current_child; i < node->child_count; i++) {
    NodeStatus status = node->children[i]->execute(node->children[i], context);

    if (status == STATUS_RUNNING) {
      node->current_child = i;
      return STATUS_RUNNING;
    }

    if (status == STATUS_SUCCESS) {
      node->current_child = 0;
      return STATUS_SUCCESS;
    }
  }

  node->current_child = 0;
  return STATUS_FAILURE;
}

// DONE: Implement bt_parallel_execute
static NodeStatus bt_parallel_execute(BTNode *node, void *context) {
  uint32_t success_count = 0;
  uint32_t failure_count = 0;

  for (uint32_t i = 0; i < node->child_count; i++) {
    NodeStatus status = node->children[i]->execute(node->children[i], context);

    if (status == STATUS_SUCCESS)
      success_count++;
    else if (status == STATUS_FAILURE)
      failure_count++;
  }

  if (failure_count > 0)
    return STATUS_FAILURE;
  if (success_count == node->child_count)
    return STATUS_SUCCESS;
  return STATUS_RUNNING;
}

// DONE: Implement bt_sequence_create
BTNode *bt_sequence_create(const char *name) {
  BTNode *node = bt_node_create(NODE_SEQUENCE, name);
  node->execute = bt_sequence_execute;
  return node;
}

// DONE: Implement bt_selector_create
BTNode *bt_selector_create(const char *name) {
  BTNode *node = bt_node_create(NODE_SELECTOR, name);
  node->execute = bt_selector_execute;
  return node;
}

// DONE: Implement bt_parallel_create
BTNode *bt_parallel_create(const char *name) {
  BTNode *node = bt_node_create(NODE_PARALLEL, name);
  node->execute = bt_parallel_execute;
  return node;
}

/* =================================================================================================
 *                                    DECORATOR NODES
 * =================================================================================================
 */

typedef struct InverterData {
  BTNode *child;
} InverterData;

static NodeStatus bt_inverter_execute(BTNode *node, void *context) {
  InverterData *data = (InverterData *)node->user_data;
  NodeStatus status = data->child->execute(data->child, context);

  if (status == STATUS_SUCCESS)
    return STATUS_FAILURE;
  if (status == STATUS_FAILURE)
    return STATUS_SUCCESS;
  return status;
}

// DONE: Implement bt_inverter_create
BTNode *bt_inverter_create(const char *name, BTNode *child) {
  BTNode *node = bt_node_create(NODE_DECORATOR, name);
  node->execute = bt_inverter_execute;

  InverterData *data = calloc(1, sizeof(InverterData));
  data->child = child;
  node->user_data = data;

  return node;
}

typedef struct RepeatData {
  BTNode *child;
  uint32_t count;
  uint32_t current;
} RepeatData;

static NodeStatus bt_repeat_execute(BTNode *node, void *context) {
  RepeatData *data = (RepeatData *)node->user_data;

  while (data->current < data->count) {
    NodeStatus status = data->child->execute(data->child, context);

    if (status == STATUS_RUNNING)
      return STATUS_RUNNING;
    if (status == STATUS_FAILURE) {
      data->current = 0;
      return STATUS_FAILURE;
    }

    data->current++;
  }

  data->current = 0;
  return STATUS_SUCCESS;
}

// DONE: Implement bt_repeat_create
BTNode *bt_repeat_create(const char *name, BTNode *child, uint32_t count) {
  BTNode *node = bt_node_create(NODE_DECORATOR, name);
  node->execute = bt_repeat_execute;

  RepeatData *data = calloc(1, sizeof(RepeatData));
  data->child = child;
  data->count = count;
  node->user_data = data;

  return node;
}

/* =================================================================================================
 *                                    ACTION NODES
 * =================================================================================================
 */

// DONE: Implement bt_action_create
BTNode *bt_action_create(const char *name,
                         NodeStatus (*action)(BTNode *, void *)) {
  BTNode *node = bt_node_create(NODE_ACTION, name);
  node->execute = action;
  return node;
}

// DONE: Implement bt_condition_create
BTNode *bt_condition_create(const char *name, bool (*condition)(void *)) {
  BTNode *node = bt_node_create(NODE_CONDITION, name);
  node->user_data = (void *)condition;

  node->execute =
      (NodeStatus(*)(BTNode *, void *))[](BTNode * n, void *ctx)->NodeStatus {
    bool (*cond)(void *) = (bool (*)(void *))n->user_data;
    return cond(ctx) ? STATUS_SUCCESS : STATUS_FAILURE;
  };

  return node;
}

/* =================================================================================================
 *                                    BEHAVIOR TREE
 * =================================================================================================
 */

// DONE: Implement bt_create
BehaviorTree *bt_create(BTNode *root, void *context) {
  BehaviorTree *bt = calloc(1, sizeof(BehaviorTree));
  bt->root = root;
  bt->context = context;
  return bt;
}

// DONE: Implement bt_destroy
void bt_destroy(BehaviorTree *bt) {
  if (!bt)
    return;
  bt_node_destroy(bt->root);
  free(bt);
}

// DONE: Implement bt_tick
NodeStatus bt_tick(BehaviorTree *bt) {
  if (!bt || !bt->root)
    return STATUS_FAILURE;

  bt->is_running = true;
  NodeStatus status = bt->root->execute(bt->root, bt->context);

  if (status != STATUS_RUNNING) {
    bt->is_running = false;
  }

  return status;
}

/* =================================================================================================
 *                                    BLACKBOARD
 * =================================================================================================
 */

#define MAX_BLACKBOARD_ENTRIES 64

typedef struct BlackboardEntry {
  char key[64];
  void *value;
  size_t size;
} BlackboardEntry;

typedef struct Blackboard {
  BlackboardEntry entries[MAX_BLACKBOARD_ENTRIES];
  uint32_t entry_count;
} Blackboard;

// DONE: Implement blackboard_create
Blackboard *blackboard_create(void) { return calloc(1, sizeof(Blackboard)); }

// DONE: Implement blackboard_destroy
void blackboard_destroy(Blackboard *bb) {
  if (!bb)
    return;

  for (uint32_t i = 0; i < bb->entry_count; i++) {
    free(bb->entries[i].value);
  }
  free(bb);
}

// DONE: Implement blackboard_set
void blackboard_set(Blackboard *bb, const char *key, const void *value,
                    size_t size) {
  if (!bb)
    return;

  // Find existing
  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0) {
      free(bb->entries[i].value);
      bb->entries[i].value = malloc(size);
      memcpy(bb->entries[i].value, value, size);
      bb->entries[i].size = size;
      return;
    }
  }

  // Add new
  if (bb->entry_count < MAX_BLACKBOARD_ENTRIES) {
    BlackboardEntry *entry = &bb->entries[bb->entry_count++];
    strncpy(entry->key, key, 63);
    entry->value = malloc(size);
    memcpy(entry->value, value, size);
    entry->size = size;
  }
}

// DONE: Implement blackboard_get
void *blackboard_get(Blackboard *bb, const char *key) {
  if (!bb)
    return NULL;

  for (uint32_t i = 0; i < bb->entry_count; i++) {
    if (strcmp(bb->entries[i].key, key) == 0) {
      return bb->entries[i].value;
    }
  }
  return NULL;
}

/* =================================================================================================
 *                                    UTILITY AI
 * =================================================================================================
 */

typedef struct UtilityAction {
  char name[64];
  float (*score)(void *context);
  void (*execute)(void *context);
} UtilityAction;

typedef struct UtilityAI {
  UtilityAction *actions;
  uint32_t action_count;
  uint32_t action_capacity;
} UtilityAI;

// DONE: Implement utility_ai_create
UtilityAI *utility_ai_create(void) {
  UtilityAI *ai = calloc(1, sizeof(UtilityAI));
  ai->action_capacity = 16;
  ai->actions = calloc(ai->action_capacity, sizeof(UtilityAction));
  return ai;
}

// DONE: Implement utility_ai_destroy
void utility_ai_destroy(UtilityAI *ai) {
  if (!ai)
    return;
  free(ai->actions);
  free(ai);
}

// DONE: Implement utility_ai_add_action
void utility_ai_add_action(UtilityAI *ai, const char *name,
                           float (*score)(void *), void (*execute)(void *)) {
  if (!ai || ai->action_count >= ai->action_capacity)
    return;

  UtilityAction *action = &ai->actions[ai->action_count++];
  strncpy(action->name, name, 63);
  action->score = score;
  action->execute = execute;
}

// DONE: Implement utility_ai_select_best
UtilityAction *utility_ai_select_best(UtilityAI *ai, void *context) {
  if (!ai || ai->action_count == 0)
    return NULL;

  float best_score = -1e30f;
  UtilityAction *best_action = NULL;

  for (uint32_t i = 0; i < ai->action_count; i++) {
    float score = ai->actions[i].score(context);
    if (score > best_score) {
      best_score = score;
      best_action = &ai->actions[i];
    }
  }

  return best_action;
}

// DONE: Implement utility_ai_execute_best
void utility_ai_execute_best(UtilityAI *ai, void *context) {
  UtilityAction *action = utility_ai_select_best(ai, context);
  if (action && action->execute) {
    action->execute(context);
  }
}
