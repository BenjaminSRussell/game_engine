/**
 * =================================================================================================
 *                              AI BEHAVIOR TREES
 *                              Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Complete behavior tree implementation for AI decision making.
 *
 * =================================================================================================
 */

#ifndef AI_BEHAVIOR_TREE_H
#define AI_BEHAVIOR_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* =================================================================================================
 *                                    NODE TYPES
 * =================================================================================================
 */

typedef enum BTNodeType {
  // Composite nodes
  BT_NODE_SEQUENCE,        // Run children in order until one fails
  BT_NODE_SELECTOR,        // Run children until one succeeds
  BT_NODE_PARALLEL,        // Run all children simultaneously
  BT_NODE_RANDOM_SELECTOR, // Random order selection
  BT_NODE_RANDOM_SEQUENCE, // Random order sequence

  // Decorator nodes
  BT_NODE_INVERTER, // Invert child result
  BT_NODE_REPEATER, // Repeat child N times
  BT_NODE_REPEAT_UNTIL_FAIL,
  BT_NODE_REPEAT_UNTIL_SUCCESS,
  BT_NODE_COOLDOWN, // Cooldown before running again
  BT_NODE_LIMIT,    // Max executions
  BT_NODE_FORCE_SUCCESS,
  BT_NODE_FORCE_FAILURE,
  BT_NODE_TIMEOUT,   // Fail after timeout
  BT_NODE_CONDITION, // Only run child if condition met

  // Leaf nodes
  BT_NODE_ACTION,          // Execute action
  BT_NODE_CONDITION_CHECK, // Check condition without action
  BT_NODE_WAIT,            // Wait for duration
  BT_NODE_SUBTREE,         // Run another behavior tree
} BTNodeType;

typedef enum BTNodeStatus {
  BT_STATUS_INVALID,
  BT_STATUS_SUCCESS,
  BT_STATUS_FAILURE,
  BT_STATUS_RUNNING,
} BTNodeStatus;

/* =================================================================================================
 *                                    BEHAVIOR TREE NODE
 * =================================================================================================
 */

typedef struct BTNode BTNode;

typedef BTNodeStatus (*BTActionFunc)(BTNode *node, void *context, float dt);
typedef bool (*BTConditionFunc)(BTNode *node, void *context);

struct BTNode {
  uint32_t id;
  char name[64];
  BTNodeType type;
  BTNodeStatus status;

  // Tree structure
  BTNode *parent;
  BTNode **children;
  uint32_t child_count;
  uint32_t child_capacity;

  // Decorator data
  union {
    struct {
      uint32_t repeat_count;
      uint32_t current_count;
    } repeater;
    struct {
      float cooldown;
      float current_time;
    } cooldown;
    struct {
      uint32_t max_count;
      uint32_t current_count;
    } limit;
    struct {
      float timeout;
      float elapsed;
    } timeout;
  } decorator_data;

  // Leaf data
  BTActionFunc action;
  BTConditionFunc condition;
  char action_name[64];
  char condition_name[64];

  // Parameters (generic key-value)
  struct {
    char key[32];
    float float_value;
    int32_t int_value;
    char string_value[64];
  } params[8];
  uint32_t param_count;

  // Subtree reference
  uint32_t subtree_id;

  // Parallel progress tracking
  BTNodeStatus *parallel_results;
  uint32_t parallel_complete_count;
  uint32_t parallel_success_threshold;
};

BTNode *bt_node_create(BTNodeType type);
void bt_node_destroy(BTNode *node);
void bt_node_add_child(BTNode *parent, BTNode *child);
void bt_node_remove_child(BTNode *parent, BTNode *child);
void bt_node_set_action(BTNode *node, BTActionFunc action, const char *name);
void bt_node_set_condition(BTNode *node, BTConditionFunc condition,
                           const char *name);
void bt_node_set_param_float(BTNode *node, const char *key, float value);
void bt_node_set_param_int(BTNode *node, const char *key, int32_t value);
void bt_node_set_param_string(BTNode *node, const char *key, const char *value);
void *bt_node_get_param(BTNode *node, const char *key);
void bt_node_reset(BTNode *node);

/* =================================================================================================
 *                                    NODE EXECUTION
 * =================================================================================================
 */

BTNodeStatus bt_execute_sequence(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_selector(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_parallel(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_random_selector(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_random_sequence(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_inverter(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_repeater(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_cooldown(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_limit(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_timeout(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_condition(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_action(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_wait(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_subtree(BTNode *node, void *context, float dt);
BTNodeStatus bt_execute_node(BTNode *node, void *context, float dt);

/* =================================================================================================
 *                                    BEHAVIOR TREE
 * =================================================================================================
 */

typedef struct BehaviorTree {
  uint32_t id;
  char name[64];
  char description[256];

  BTNode *root;

  // Blackboard for shared data
  struct {
    char key[32];
    void *value;
    size_t size;
  } blackboard[32];
  uint32_t blackboard_count;

  // State
  bool is_running;
  BTNodeStatus last_status;
  float tick_interval;
  float time_since_tick;
} BehaviorTree;

BehaviorTree *behavior_tree_create(const char *name);
void behavior_tree_destroy(BehaviorTree *tree);
void behavior_tree_set_root(BehaviorTree *tree, BTNode *root);
void behavior_tree_tick(BehaviorTree *tree, void *context, float dt);
void behavior_tree_reset(BehaviorTree *tree);
void behavior_tree_abort(BehaviorTree *tree);
void behavior_tree_pause(BehaviorTree *tree);
void behavior_tree_resume(BehaviorTree *tree);
void behavior_tree_blackboard_set(BehaviorTree *tree, const char *key,
                                  void *value, size_t size);
void *behavior_tree_blackboard_get(BehaviorTree *tree, const char *key);
void behavior_tree_blackboard_clear(BehaviorTree *tree);
void behavior_tree_serialize(BehaviorTree *tree, const char *path);
void behavior_tree_deserialize(BehaviorTree *tree, const char *path);
BehaviorTree *behavior_tree_load_json(const char *path);
void behavior_tree_save_json(BehaviorTree *tree, const char *path);
bool behavior_tree_validate(BehaviorTree *tree);

/* =================================================================================================
 *                                    COMMON ACTIONS
 * =================================================================================================
 */

BTNodeStatus action_move_to(BTNode *node, void *context, float dt);
BTNodeStatus action_move_to_target(BTNode *node, void *context, float dt);
BTNodeStatus action_move_random(BTNode *node, void *context, float dt);
BTNodeStatus action_patrol(BTNode *node, void *context, float dt);
BTNodeStatus action_follow(BTNode *node, void *context, float dt);
BTNodeStatus action_flee(BTNode *node, void *context, float dt);
BTNodeStatus action_attack(BTNode *node, void *context, float dt);
BTNodeStatus action_use_ability(BTNode *node, void *context, float dt);
BTNodeStatus action_heal(BTNode *node, void *context, float dt);
BTNodeStatus action_take_cover(BTNode *node, void *context, float dt);
BTNodeStatus action_interact(BTNode *node, void *context, float dt);
BTNodeStatus action_play_animation(BTNode *node, void *context, float dt);
BTNodeStatus action_play_sound(BTNode *node, void *context, float dt);
BTNodeStatus action_speak(BTNode *node, void *context, float dt);
BTNodeStatus action_look_at(BTNode *node, void *context, float dt);
BTNodeStatus action_face_target(BTNode *node, void *context, float dt);
BTNodeStatus action_set_state(BTNode *node, void *context, float dt);

/* =================================================================================================
 *                                    COMMON CONDITIONS
 * =================================================================================================
 */

bool condition_has_target(BTNode *node, void *context);
bool condition_target_in_range(BTNode *node, void *context);
bool condition_target_visible(BTNode *node, void *context);
bool condition_health_above(BTNode *node, void *context);
bool condition_health_below(BTNode *node, void *context);
bool condition_has_ammo(BTNode *node, void *context);
bool condition_ability_ready(BTNode *node, void *context);
bool condition_in_combat(BTNode *node, void *context);
bool condition_is_alerted(BTNode *node, void *context);
bool condition_random_chance(BTNode *node, void *context);
bool condition_blackboard_has(BTNode *node, void *context);
bool condition_blackboard_compare(BTNode *node, void *context);
bool condition_time_of_day(BTNode *node, void *context);
bool condition_has_path(BTNode *node, void *context);

/* =================================================================================================
 *                                    BEHAVIOR TREE LIBRARY
 * =================================================================================================
 */

typedef struct BTLibrary {
  BehaviorTree *trees;
  uint32_t tree_count;
  uint32_t tree_capacity;

  BTActionFunc *action_registry;
  char **action_names;
  uint32_t action_count;

  BTConditionFunc *condition_registry;
  char **condition_names;
  uint32_t condition_count;
} BTLibrary;

void bt_library_init(void);
void bt_library_shutdown(void);
void bt_library_register_tree(BehaviorTree *tree);
BehaviorTree *bt_library_get_tree(const char *name);
void bt_library_register_action(const char *name, BTActionFunc func);
void bt_library_register_condition(const char *name, BTConditionFunc func);
BTActionFunc bt_library_get_action(const char *name);
BTConditionFunc bt_library_get_condition(const char *name);
void bt_library_load_directory(const char *path);

/* =================================================================================================
 *                                    VISUAL DEBUGGER
 * =================================================================================================
 */

void bt_debugger_init(void);
void bt_debugger_render(BehaviorTree *tree);
void bt_debugger_render_node(BTNode *node, float x, float y);
void bt_debugger_highlight_path(BehaviorTree *tree);
void bt_debugger_show_values(BTNode *node);
void bt_debugger_breakpoint(BTNode *node);
void bt_debugger_step(void);

#endif // AI_BEHAVIOR_TREE_H
