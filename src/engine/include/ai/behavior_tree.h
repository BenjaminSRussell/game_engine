#ifndef BEHAVIOR_TREE_H
#define BEHAVIOR_TREE_H

#include "../core/logger.h"
#include "../core/types.h"
#include "../math/vec2.h"
#include "../math/vec3.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct BehaviorTree BehaviorTree;
typedef struct BTNode BTNode;
typedef struct BTBlackboard BTBlackboard;
typedef struct BTContext BTContext;
typedef struct ParallelData ParallelData;

// Node types
typedef enum {
  BT_NODE_ACTION,    // Leaf node - performs an action
  BT_NODE_CONDITION, // Leaf node - checks a condition
  BT_NODE_SEQUENCE,  // Composite - executes children in sequence (AND)
  BT_NODE_SELECTOR,  // Composite - executes children until one succeeds (OR)
  BT_NODE_PARALLEL,  // Composite - executes all children
  BT_NODE_DECORATOR, // Single child with modification
  BT_NODE_ROOT,      // Root node

  // Enhanced node types
  BT_NODE_INVERTER,   // Decorator - inverts child result
  BT_NODE_REPEATER,   // Decorator - repeats child N times
  BT_NODE_COOLDOWN,   // Decorator - cooldown period
  BT_NODE_TIMER,      // Decorator - timer duration
  BT_NODE_WAIT,       // Action - wait for duration
  BT_NODE_MOVETO,     // Action - move to position
  BT_NODE_PERCEPTION, // Action - perception check

  // Advanced decorators
  BT_NODE_FORCE_SUCCESS,        // Decorator - always returns success
  BT_NODE_FORCE_FAILURE,        // Decorator - always returns failure
  BT_NODE_UNTIL_SUCCESS,        // Decorator - repeat until success
  BT_NODE_UNTIL_FAILURE,        // Decorator - repeat until failure
  BT_NODE_TIMEOUT,              // Decorator - fail if child takes too long
  BT_NODE_RETRY,                // Decorator - retry N times on failure
  BT_NODE_BLACKBOARD_CONDITION, // Decorator - check blackboard before execution

  // AGENT_AI_1 Phase 4: Utility AI Integration
  BT_NODE_UTILITY_SELECTOR, // Composite - uses utility scoring to select child
  BT_NODE_GOAP_GOAL,        // Action - Plans and executes a GOAP goal

  BT_NODE_COUNT // Total number of node types
} BTNodeType;

// Node states
typedef enum {
  BT_STATE_SUCCESS,
  BT_STATE_FAILURE,
  BT_STATE_RUNNING,
  BT_STATE_READY
} BTNodeState;

// Blackboard value types
typedef enum {
  BT_VALUE_BOOL,
  BT_VALUE_INT,
  BT_VALUE_FLOAT,
  BT_VALUE_STRING,
  BT_VALUE_VECTOR3,
  BT_VALUE_ENTITY,
  BT_VALUE_POINTER
} BTValueType;

// Advanced decorator data
typedef struct BTAdvancedDecoratorData {
  int retry_count;
  int max_retries;
  float timeout_duration;
  float start_time;
  float cooldown_time;
  float current_cooldown;
  float elapsed_time;
  bool until_success;
  bool until_failure;
  bool invert_condition;
  int current_retry;
  char blackboard_key[64];
  BTValueType expected_type;
  union {
    bool bool_val;
    int int_val;
    float float_val;
    char string_val[256];
    Vec3 vector_val;
    uint32_t entity_val;
    void *pointer_val;
  } expected_value;
} BTAdvancedDecoratorData;

// Perception integration
typedef struct BTPerceptionConfig {
  float sight_range;
  float hearing_range;
  float touch_range;
  bool sight_enabled;
  bool hearing_enabled;
  bool touch_enabled;
  uint32_t last_perception_tick;
} BTPerceptionConfig;

// Execution history entry
typedef struct {
  char node_name[64];
  BTNodeState state;
  float timestamp;
  float execution_time;
} BTExecutionEntry;

// Execution history buffer
typedef struct BTExecutionHistory {
  BTExecutionEntry entries[256];
  int write_index;
  int count;
} BTExecutionHistory;

// Parallel node data
typedef struct ParallelData {
    uint32_t success_threshold;
    uint32_t failure_threshold;
    uint32_t success_count;
    uint32_t failure_count;
    bool completed;
} ParallelData;

// Enhanced node-specific data
struct BTNode {
  char name[64];
  BTNodeType type;
  BTNodeState state;

  // Node hierarchy
  BTNode *parent;
  BTNode *children[16]; // Maximum 16 children
  int child_count;

  // Node-specific data
  union {
    void *node_data;
    BTAdvancedDecoratorData *decorator_data;
    BTPerceptionConfig *perception_data;
    void *action_data;
    ParallelData *parallel_data;
  } data;

  // Callbacks
  BTNodeState (*on_enter)(BTNode *node, BTContext *context);
  BTNodeState (*on_execute)(BTNode *node, BTContext *context);
  BTNodeState (*on_exit)(BTNode *node, BTContext *context);
  BTNodeState (*on_check)(BTNode *node, BTContext *context);

  // Timing
  float execution_time;
  float cooldown_time;
  float last_execution;

  // Flags
  bool is_active;
  bool reset_on_success;
  bool reset_on_failure;
  bool is_breakpoint;

  // Debug info
  int execution_count;
  float total_execution_time;
  float average_execution_time;

  // Visual debugging
  Vec2 position;
  Vec2 size;
  bool is_selected;
  bool is_highlighted;
  
  // Utility AI integration
  void *utility_agent;
};

// Enhanced Behavior Tree context
struct BTContext {
  BehaviorTree *tree;
  BTBlackboard *blackboard;
  void *agent;
  float delta_time;
  uint32_t frame_number;
  BTExecutionHistory *history;
  void *perception_system;
  void *job_system;
  void *user_data;
};

// Visual debugging state
typedef struct {
  bool enabled;
  bool break_on_node;
  char breakpoint_node[64];
  bool step_mode;
  BTExecutionHistory history;
  float current_highlight_time;
  char current_node[64];
} BTVisualDebug;

// Node pool for fast allocation
typedef struct {
  BTNode *nodes;
  int *free_list;
  int free_count;
  int total_count;
  int capacity;
} BTNodePool;

// Template data
typedef struct {
  char name[64];
  char description[256];
  BTNodeType root_type;
  char json_data[2048];
  int version;
} BTTemplate;

// Template library
typedef struct {
  BTTemplate templates[32];
  int count;
  int max_templates;
} BTTemplateLibrary;

// Enhanced Behavior Tree
struct BehaviorTree {
  char name[64];
  BTNode *root;
  bool is_active;
  bool is_paused;
  BTNode *current_node;
  uint32_t execution_count;
  float total_execution_time;
  bool owns_nodes;
  BTNodePool *node_pool;
  BTVisualDebug *visual_debug;
  BTTemplateLibrary *template_library;
  BTPerceptionConfig *perception_config;
  int version;
  char last_file_path[512];
  bool auto_reload;
  time_t last_modified;
  bool debug_mode;
  char last_error[256];
};

// --- API ---

// Core functions
BehaviorTree *bt_create_tree(const char *name);
void bt_destroy_tree(BehaviorTree *tree);
BTNode *bt_create_node(const char *name, BTNodeType type);
void bt_destroy_node(BTNode *node);
bool bt_add_child(BTNode *parent, BTNode *child);
bool bt_remove_child(BTNode *parent, BTNode *child);
BTNodeState bt_tick(BehaviorTree *tree, BTContext *context);

// Decorators
BTNode *bt_create_inverter(const char *name);
BTNode *bt_create_repeater(const char *name, int repeat_count);

// Action nodes
BTNode *bt_create_action(const char *name,
                         BTNodeState (*execute)(BTNode *, BTContext *));
BTNode *bt_create_wait_action(const char *name, float duration);
BTNode *bt_create_move_to_action(const char *name, const Vec3 *target,
                                 float tolerance);

// Composite nodes
BTNode *bt_create_sequence(const char *name);
BTNode *bt_create_selector(const char *name);

// Utility nodes (AGENT_AI_1 Phase 4)
struct UtilityAgent;
typedef struct UtilityAgent UtilityAgent;
BTNode *bt_create_utility_selector(const char *name,
                                   UtilityAgent *utility_agent);

#ifdef __cplusplus
}
#endif

#endif // BEHAVIOR_TREE_H
