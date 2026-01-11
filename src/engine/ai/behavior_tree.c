// ai/behavior_tree.c
// Implementation matching the existing comprehensive header
#include "include/ai/behavior_tree.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// TODO: Implement BT decorators for Cooldowns, Blackboard filters, and Timeouts
// TODO: Add support for Parallel nodes with configurable success/failure
// policies
// TODO: Implement BT node pooling and memory-mapped storage for massive entity
// counts
// TODO: Add blackboard inheritance and scoping for nested behavior trees
// TODO: Implement real-time BT visualizer and debugger with state inspection
// TODO: Research and implement BT optimization using SSAO-style spatial
// pre-evaluation
// TODO: Add support for dynamic BT reloading from hot-compiled script nodes
// TODO: Implement BT event listeners for reactive behavior triggering
// TODO: Research AI-driven BT generation based on player behavior observation
// TODO: Add support for utility-based selectors within the BT hierarchy
// TODO: Implement node-level profiling to identify bottleneck behaviors
// TODO: Add support for asynchronous action node execution with state
// persistence
// TODO: Research integration with PCG systems for location-aware behavior
// selection
// TODO: Implement BT-driven animation state synchronization for NPCs
// TODO: Add logic for cooperative behaviors using shared blackboard data
// TODO: Research and implement pathfinding-aware behavior selection in BT nodes
// TODO: Implement BT node snapshotting for save-game restoration
// TODO: Add support for probabilistic node selection for more varied AI
// behavior
// TODO: Research multi-threaded BT evaluation with lock-free state updates
// TODO: Implement BT-driven crowd density management for performance scaling

BehaviorTree *bt_create_tree(const char *name) {
  BehaviorTree *tree = (BehaviorTree *)calloc(1, sizeof(BehaviorTree));
  strncpy(tree->name, name, sizeof(tree->name) - 1);
  tree->is_active = true;
  tree->version = 1;
  tree->owns_nodes = true;
  LOG_INFO("Behavior Tree created: %s", name);
  return tree;
}

void bt_destroy_tree(BehaviorTree *tree) {
  if (!tree)
    return;
  if (tree->root && tree->owns_nodes) {
    bt_destroy_node(tree->root);
  }
  free(tree);
}

BTNode *bt_create_node(const char *name, BTNodeType type) {
  BTNode *node = (BTNode *)calloc(1, sizeof(BTNode));
  strncpy(node->name, name, sizeof(node->name) - 1);
  node->type = type;
  node->state = BT_STATE_READY;
  return node;
}

void bt_destroy_node(BTNode *node) {
  if (!node)
    return;
  for (int i = 0; i < node->child_count; i++) {
    bt_destroy_node(node->children[i]);
  }
  // Data cleanup would go here if specific types used malloced data
  if (node->type == BT_NODE_REPEATER && node->data.decorator_data) {
    free(node->data.decorator_data);
  }
  free(node);
}

bool bt_add_child(BTNode *parent, BTNode *child) {
  if (!parent || !child || parent->child_count >= 16)
    return false;
  parent->children[parent->child_count++] = child;
  child->parent = parent;
  return true;
}

BTNode *bt_create_sequence(const char *name) {
  return bt_create_node(name, BT_NODE_SEQUENCE);
}

BTNode *bt_create_selector(const char *name) {
  return bt_create_node(name, BT_NODE_SELECTOR);
}

BTNode *bt_create_inverter(const char *name) {
  return bt_create_node(name, BT_NODE_INVERTER);
}

BTNode *bt_create_repeater(const char *name, int repeat_count) {
  BTNode *node = bt_create_node(name, BT_NODE_REPEATER);
  node->data.decorator_data =
      (BTAdvancedDecoratorData *)calloc(1, sizeof(BTAdvancedDecoratorData));
  node->data.decorator_data->max_retries =
      repeat_count; // Using max_retries as repeat count
  node->data.decorator_data->retry_count = 0;
  return node;
}

BTNode *bt_create_action(const char *name,
                         BTNodeState (*execute)(BTNode *, BTContext *)) {
  BTNode *node = bt_create_node(name, BT_NODE_ACTION);
  node->on_execute = execute;
  return node;
}

static BTNodeState bt_execute_node(BTNode *node, BTContext *context) {
  if (!node)
    return BT_STATE_FAILURE;

  node->state = BT_STATE_RUNNING;

  switch (node->type) {
  case BT_NODE_SEQUENCE:
    for (int i = 0; i < node->child_count; i++) {
      BTNodeState child_state = bt_execute_node(node->children[i], context);
      if (child_state == BT_STATE_RUNNING)
        return BT_STATE_RUNNING;
      if (child_state == BT_STATE_FAILURE)
        return BT_STATE_FAILURE;
    }
    return BT_STATE_SUCCESS;

  case BT_NODE_SELECTOR:
    for (int i = 0; i < node->child_count; i++) {
      BTNodeState child_state = bt_execute_node(node->children[i], context);
      if (child_state == BT_STATE_RUNNING)
        return BT_STATE_RUNNING;
      if (child_state == BT_STATE_SUCCESS)
        return BT_STATE_SUCCESS;
    }
    return BT_STATE_FAILURE;

  case BT_NODE_INVERTER:
    if (node->child_count > 0) {
      BTNodeState child_state = bt_execute_node(node->children[0], context);
      if (child_state == BT_STATE_SUCCESS)
        return BT_STATE_FAILURE;
      if (child_state == BT_STATE_FAILURE)
        return BT_STATE_SUCCESS;
      return BT_STATE_RUNNING;
    }
    return BT_STATE_SUCCESS; // Empty inverter treats as success (or failure?)

  case BT_NODE_REPEATER:
    if (node->child_count > 0) {
      int target = node->data.decorator_data
                       ? node->data.decorator_data->max_retries
                       : 1;
      int *count = node->data.decorator_data
                       ? &node->data.decorator_data->retry_count
                       : NULL;

      while (!count || *count < target) {
        BTNodeState child_state = bt_execute_node(node->children[0], context);
        if (child_state == BT_STATE_RUNNING)
          return BT_STATE_RUNNING;
        if (child_state == BT_STATE_FAILURE)
          return BT_STATE_FAILURE; // Abort on failure option?
        if (count)
          (*count)++;
      }
      if (count)
        *count = 0; // Reset
      return BT_STATE_SUCCESS;
    }
    return BT_STATE_SUCCESS;

  case BT_NODE_GOAP_GOAL:
    // TODO: Connect to GOAP planner here
    // For now, return success to simulate goal completion
    return BT_STATE_SUCCESS;

  case BT_NODE_UTILITY_SELECTOR:
    // TODO: Connect to Utility system
    // Select best child based on scroring
    if (node->child_count > 0) {
      // Placeholder: pick first running or new child
      return bt_execute_node(node->children[0], context);
    }
    return BT_STATE_FAILURE;

  case BT_NODE_ACTION:
    if (node->on_execute) {
      return node->on_execute(node, context);
    }
    return BT_STATE_SUCCESS;

  default:
    return BT_STATE_FAILURE;
  }
}

BTNodeState bt_tick(BehaviorTree *tree, BTContext *context) {
  if (!tree || !tree->root || !tree->is_active)
    return BT_STATE_FAILURE;
  context->tree = tree;
  return bt_execute_node(tree->root, context);
}

// Utility Node Creator
BTNode *bt_create_utility_selector(const char *name,
                                   UtilityAgent *utility_agent) {
  BTNode *node = bt_create_node(name, BT_NODE_UTILITY_SELECTOR);
  return node;
}
