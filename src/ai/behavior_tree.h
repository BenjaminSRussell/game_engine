#ifndef BEHAVIOR_TREE_H
#define BEHAVIOR_TREE_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Entity Entity;
typedef struct World World;

// Behavior Tree Node Types
typedef enum {
    BT_NODE_SELECTOR,     // OR logic - tries children in order until one succeeds
    BT_NODE_SEQUENCE,    // AND logic - executes children in order, all must succeed
    BT_NODE_PARALLEL,    // Executes all children simultaneously
    BT_NODE_CONDITION,   // Returns success/failure based on condition
    BT_NODE_ACTION,      // Performs an action, returns success/failure
    BT_NODE_DECORATOR,   // Modifies child behavior (inverter, repeater, etc.)
    BT_NODE_CUSTOM       // Custom node implementation
} BTNodeType;

// Behavior Node Status
typedef enum {
    BT_STATUS_SUCCESS,
    BT_STATUS_FAILURE,
    BT_STATUS_RUNNING
} BTNodeStatus;

// Forward declarations
struct BehaviorTree;
struct BehaviorNode;

// Condition function pointer
typedef BTNodeStatus (*BTConditionFunc)(Entity* entity, void* user_data);

// Action function pointer
typedef BTNodeStatus (*BTActionFunc)(Entity* entity, void* user_data, f32 delta_time);

// Custom node update function
typedef BTNodeStatus (*BTCustomUpdateFunc)(struct BehaviorNode* node, Entity* entity, void* user_data, f32 delta_time);

// Behavior Node structure
typedef struct BehaviorNode {
    BTNodeType type;
    char name[64];
    
    // Node-specific data
    union {
        struct {
            struct BehaviorNode** children;
            u32 child_count;
            u32 current_child;
        } composite;
        
        struct {
            BTConditionFunc condition;
            void* user_data;
        } condition;
        
        struct {
            BTActionFunc action;
            void* user_data;
            f32 timer;
        } action;
        
        struct {
            struct BehaviorNode* child;
            BTNodeStatus (*decorate)(BTNodeStatus child_status);
        } decorator;
        
        struct {
            BTCustomUpdateFunc update_func;
            void* user_data;
        } custom;
    };
    
    // Runtime state
    BTNodeStatus last_status;
    void* runtime_data;
    
} BehaviorNode;

// Behavior Tree structure
typedef struct BehaviorTree {
    BehaviorNode* root;
    char name[64];
    bool enabled;
    void* blackboard;
} BehaviorTree;

// Blackboard for shared data between nodes
typedef struct {
    // Target tracking
    Entity target_entity;
    Vec3 target_position;
    bool has_target;
    
    // Memory
    Vec3 last_known_position;
    f32 last_seen_time;
    
    // State
    bool is_alerted;
    bool is_hungry;
    bool is_thirsty;
    f32 health_percentage;
    
    // Custom data
    void* custom_data;
    size_t custom_data_size;
} BTBlackboard;

// MARK: - Tree Management

BehaviorTree* bt_create_tree(const char* name);
void bt_destroy_tree(BehaviorTree* tree);
void bt_set_root(BehaviorTree* tree, BehaviorNode* root);
void bt_set_enabled(BehaviorTree* tree, bool enabled);

// MARK: - Node Creation

BehaviorNode* bt_create_selector(const char* name);
BehaviorNode* bt_create_sequence(const char* name);
BehaviorNode* bt_create_parallel(const char* name, bool succeed_on_one, bool fail_on_one);

BehaviorNode* bt_create_condition(const char* name, BTConditionFunc condition, void* user_data);
BehaviorNode* bt_create_action(const char* name, BTActionFunc action, void* user_data);

BehaviorNode* bt_create_inverter(const char* name, BehaviorNode* child);
BehaviorNode* bt_create_repeater(const char* name, BehaviorNode* child, u32 repeat_count);
BehaviorNode* bt_create_timer(const char* name, BehaviorNode* child, f32 duration);

BehaviorNode* bt_create_custom(const char* name, BTCustomUpdateFunc update_func, void* user_data);

// MARK: - Node Composition

void bt_add_child(BehaviorNode* parent, BehaviorNode* child);
void bt_remove_child(BehaviorNode* parent, BehaviorNode* child);
void bt_clear_children(BehaviorNode* node);

// MARK: - Tree Execution

BTNodeStatus bt_update_tree(BehaviorTree* tree, Entity* entity, f32 delta_time);
void bt_reset_tree(BehaviorTree* tree);

// MARK: - Blackboard Management

BTBlackboard* bt_get_blackboard(BehaviorTree* tree);
void bt_set_blackboard_data(BehaviorTree* tree, void* data, size_t size);

// MARK: - Built-in Conditions

BTNodeStatus bt_condition_has_target(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_health_low(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_enemy_nearby(Entity* entity, void* user_data);
BTNodeStatus bt_condition_can_see_target(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_hungry(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_thirsty(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_daytime(Entity* entity, void* user_data);
BTNodeStatus bt_condition_is_nighttime(Entity* entity, void* user_data);

// MARK: - Built-in Actions

BTNodeStatus bt_action_wander(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_chase_target(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_flee_from_target(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_attack_target(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_find_food(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_find_water(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_rest(Entity* entity, void* user_data, f32 delta_time);
BTNodeStatus bt_action_patrol(Entity* entity, void* user_data, f32 delta_time);

// MARK: - Utility Functions

const char* bt_get_node_status_string(BTNodeStatus status);
const char* bt_get_node_type_string(BTNodeType type);
void bt_print_tree(BehaviorTree* tree);

#endif // BEHAVIOR_TREE_H
