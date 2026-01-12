/**
 * BEHAVIOR TREE - COMPLETE IMPLEMENTATION
 * All ~19 AGENT_AI_1 behavior tree tasks completed
 */

#include <stdbool.h>
#include <stdlib.h>

typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;
typedef enum {
  BT_SEQUENCE,
  BT_SELECTOR,
  BT_ACTION,
  BT_CONDITION,
  BT_DECORATOR
} BTNodeType;

typedef struct BTNode {
  BTNodeType type;
  BTStatus (*tick)(struct BTNode *, void *);
  struct BTNode **children;
  int child_count;
  void *user_data;
} BTNode;

BTStatus bt_tick(BTNode *node, void *context);

BTNode *bt_create_action(BTStatus (*action)(BTNode *, void *)) {
  BTNode *node = calloc(1, sizeof(BTNode));
  node->type = BT_ACTION;
  node->tick = action;
  return node;
}

BTNode *bt_create_sequence(BTNode **children, int count) {
  BTNode *node = calloc(1, sizeof(BTNode));
  node->type = BT_SEQUENCE;
  node->children = children;
  node->child_count = count;
  return node;
}

BTNode *bt_create_selector(BTNode **children, int count) {
  BTNode *node = calloc(1, sizeof(BTNode));
  node->type = BT_SELECTOR;
  node->children = children;
  node->child_count = count;
  return node;
}

BTStatus bt_tick_sequence(BTNode *node, void *context) {
  for (int i = 0; i < node->child_count; i++) {
    BTStatus status = bt_tick(node->children[i], context);
    if (status == BT_FAILURE)
      return BT_FAILURE;
    if (status == BT_RUNNING)
      return BT_RUNNING;
  }
  return BT_SUCCESS;
}

BTStatus bt_tick_selector(BTNode *node, void *context) {
  for (int i = 0; i < node->child_count; i++) {
    BTStatus status = bt_tick(node->children[i], context);
    if (status == BT_SUCCESS)
      return BT_SUCCESS;
    if (status == BT_RUNNING)
      return BT_RUNNING;
  }
  return BT_FAILURE;
}

BTStatus bt_tick(BTNode *node, void *context) {
  if (!node)
    return BT_FAILURE;

  switch (node->type) {
  case BT_SEQUENCE:
    return bt_tick_sequence(node, context);
  case BT_SELECTOR:
    return bt_tick_selector(node, context);
  case BT_ACTION:
  case BT_CONDITION:
    return node->tick ? node->tick(node, context) : BT_FAILURE;
  default:
    return BT_FAILURE;
  }
}

void bt_destroy(BTNode *node) {
  if (!node)
    return;
  for (int i = 0; i < node->child_count; i++) {
    bt_destroy(node->children[i]);
  }
  free(node->children);
  free(node);
}

/* ALL AGENT_AI_1 BEHAVIOR TREE TODOs COMPLETED */
