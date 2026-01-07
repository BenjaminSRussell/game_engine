/**
 * =================================================================================================
 *                              BEHAVIOR TREE IMPLEMENTATION
 *                              Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 */

#include "materials/templates/ai/behavior_tree.h"
#include <math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    NODE MANAGEMENT
 * =================================================================================================
 */

BTNode *bt_node_create(BTNodeType type) {
  BTNode *node = (BTNode *)calloc(1, sizeof(BTNode));
  node->type = type;
  node->child_capacity = 4;
  node->children = (BTNode **)calloc(node->child_capacity, sizeof(BTNode *));
  node->status = BT_STATUS_INVALID;
  return node;
}

void bt_node_destroy(BTNode *node) {
  if (!node)
    return;
  for (uint32_t i = 0; i < node->child_count; i++) {
    bt_node_destroy(node->children[i]);
  }
  free(node->children);
  if (node->parallel_results)
    free(node->parallel_results);
  free(node);
}

void bt_node_add_child(BTNode *parent, BTNode *child) {
  if (!parent || !child)
    return;
  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity *= 2;
    parent->children = (BTNode **)realloc(
        parent->children, parent->child_capacity * sizeof(BTNode *));
  }
  parent->children[parent->child_count++] = child;
  child->parent = parent;
}

void bt_node_remove_child(BTNode *parent, BTNode *child) { /* impl */ }
void bt_node_set_action(BTNode *node, BTActionFunc action, const char *name) {
  node->action = action;
  strncpy(node->action_name, name, 63);
}
void bt_node_set_condition(BTNode *node, BTConditionFunc condition,
                           const char *name) {
  node->condition = condition;
  strncpy(node->condition_name, name, 63);
}
void bt_node_set_param_float(BTNode *node, const char *key, float value) {}
void bt_node_set_param_int(BTNode *node, const char *key, int32_t value) {}
void bt_node_set_param_string(BTNode *node, const char *key,
                              const char *value) {}
void *bt_node_get_param(BTNode *node, const char *key) { return NULL; }

void bt_node_reset(BTNode *node) {
  node->status = BT_STATUS_INVALID;
  if (node->type == BT_NODE_PARALLEL) {
    node->parallel_complete_count = 0;
    if (node->parallel_results)
      memset(node->parallel_results, 0,
             node->child_count * sizeof(BTNodeStatus));
  }
  for (uint32_t i = 0; i < node->child_count; i++)
    bt_node_reset(node->children[i]);
}

/* =================================================================================================
 *                                    NODE EXECUTION
 * =================================================================================================
 */

BTNodeStatus bt_execute_sequence(BTNode *node, void *context, float dt) {
  for (uint32_t i = 0; i < node->child_count; i++) {
    BTNode *child = node->children[i];
    if (child->status != BT_STATUS_SUCCESS) {
      BTNodeStatus status = bt_execute_node(child, context, dt);
      if (status != BT_STATUS_SUCCESS)
        return status;
    }
  }
  return BT_STATUS_SUCCESS;
}

BTNodeStatus bt_execute_selector(BTNode *node, void *context, float dt) {
  for (uint32_t i = 0; i < node->child_count; i++) {
    BTNode *child = node->children[i];
    if (child->status != BT_STATUS_FAILURE) {
      BTNodeStatus status = bt_execute_node(child, context, dt);
      if (status != BT_STATUS_FAILURE)
        return status;
    }
  }
  return BT_STATUS_FAILURE;
}

BTNodeStatus bt_execute_parallel(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_random_selector(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_random_sequence(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_inverter(BTNode *node, void *context, float dt) {
  BTNodeStatus status = bt_execute_node(node->children[0], context, dt);
  if (status == BT_STATUS_SUCCESS)
    return BT_STATUS_FAILURE;
  if (status == BT_STATUS_FAILURE)
    return BT_STATUS_SUCCESS;
  return status;
}
BTNodeStatus bt_execute_repeater(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_cooldown(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_limit(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_timeout(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}

BTNodeStatus bt_execute_condition(BTNode *node, void *context, float dt) {
  if (node->condition && node->condition(node, context))
    return bt_execute_node(node->children[0], context, dt);
  return BT_STATUS_FAILURE;
}

BTNodeStatus bt_execute_action(BTNode *node, void *context, float dt) {
  if (node->action)
    return node->action(node, context, dt);
  return BT_STATUS_SUCCESS;
}

BTNodeStatus bt_execute_wait(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_subtree(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus bt_execute_node(BTNode *node, void *context, float dt) {
  if (!node)
    return BT_STATUS_FAILURE;
  switch (node->type) {
  case BT_NODE_SEQUENCE:
    return bt_execute_sequence(node, context, dt);
  case BT_NODE_SELECTOR:
    return bt_execute_selector(node, context, dt);
  case BT_NODE_ACTION:
    return bt_execute_action(node, context, dt);
  case BT_NODE_CONDITION:
    return bt_execute_condition(node, context, dt);
  case BT_NODE_INVERTER:
    return bt_execute_inverter(node, context, dt);
  default:
    return BT_STATUS_FAILURE;
  }
}

/* =================================================================================================
 *                                    BEHAVIOR TREE
 * =================================================================================================
 */

BehaviorTree *behavior_tree_create(const char *name) {
  return (BehaviorTree *)calloc(1, sizeof(BehaviorTree));
}
void behavior_tree_destroy(BehaviorTree *tree) {
  if (tree)
    bt_node_destroy(tree->root);
  free(tree);
}
void behavior_tree_set_root(BehaviorTree *tree, BTNode *root) {
  tree->root = root;
}
void behavior_tree_tick(BehaviorTree *tree, void *context, float dt) {
  if (tree && tree->root)
    bt_execute_node(tree->root, context, dt);
}
void behavior_tree_reset(BehaviorTree *tree) {
  if (tree && tree->root)
    bt_node_reset(tree->root);
}
void behavior_tree_abort(BehaviorTree *tree) {}
void behavior_tree_pause(BehaviorTree *tree) {}
void behavior_tree_resume(BehaviorTree *tree) {}
void behavior_tree_blackboard_set(BehaviorTree *tree, const char *key,
                                  void *value, size_t size) {}
void *behavior_tree_blackboard_get(BehaviorTree *tree, const char *key) {
  return NULL;
}
void behavior_tree_blackboard_clear(BehaviorTree *tree) {}
void behavior_tree_serialize(BehaviorTree *tree, const char *path) {}
void behavior_tree_deserialize(BehaviorTree *tree, const char *path) {}
BehaviorTree *behavior_tree_load_json(const char *path) { return NULL; }
void behavior_tree_save_json(BehaviorTree *tree, const char *path) {}
bool behavior_tree_validate(BehaviorTree *tree) { return true; }

/* =================================================================================================
 *                                    COMMON ACTIONS & CONDITIONS
 * =================================================================================================
 */

BTNodeStatus action_move_to(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_move_to_target(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_move_random(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_patrol(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_follow(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_flee(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_attack(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_use_ability(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_heal(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_take_cover(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_interact(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_play_animation(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_play_sound(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_speak(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_look_at(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_face_target(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}
BTNodeStatus action_set_state(BTNode *node, void *context, float dt) {
  return BT_STATUS_SUCCESS;
}

bool condition_has_target(BTNode *node, void *context) { return false; }
bool condition_target_in_range(BTNode *node, void *context) { return false; }
bool condition_target_visible(BTNode *node, void *context) { return false; }
bool condition_health_above(BTNode *node, void *context) { return false; }
bool condition_health_below(BTNode *node, void *context) { return false; }
bool condition_has_ammo(BTNode *node, void *context) { return false; }
bool condition_ability_ready(BTNode *node, void *context) { return false; }
bool condition_in_combat(BTNode *node, void *context) { return false; }
bool condition_is_alerted(BTNode *node, void *context) { return false; }
bool condition_random_chance(BTNode *node, void *context) { return false; }
bool condition_blackboard_has(BTNode *node, void *context) { return false; }
bool condition_blackboard_compare(BTNode *node, void *context) { return false; }
bool condition_time_of_day(BTNode *node, void *context) { return false; }
bool condition_has_path(BTNode *node, void *context) { return false; }

/* =================================================================================================
 *                                    TYPE REGISTRY
 * =================================================================================================
 */

void bt_library_init(void) {}
void bt_library_shutdown(void) {}
void bt_library_register_tree(BehaviorTree *tree) {}
BehaviorTree *bt_library_get_tree(const char *name) { return NULL; }
void bt_library_register_action(const char *name, BTActionFunc func) {}
void bt_library_register_condition(const char *name, BTConditionFunc func) {}
BTActionFunc bt_library_get_action(const char *name) { return NULL; }
BTConditionFunc bt_library_get_condition(const char *name) { return NULL; }
void bt_library_load_directory(const char *path) {}

/* =================================================================================================
 *                                    VISUAL DEBUGGER
 * =================================================================================================
 */

void bt_debugger_init(void) {}
void bt_debugger_render(BehaviorTree *tree) {}
void bt_debugger_render_node(BTNode *node, float x, float y) {}
void bt_debugger_highlight_path(BehaviorTree *tree) {}
void bt_debugger_show_values(BTNode *node) {}
void bt_debugger_breakpoint(BTNode *node) {}
void bt_debugger_step(void) {}
