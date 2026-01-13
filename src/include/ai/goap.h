#ifndef GOAP_H
#define GOAP_H

#include <core/types.h>
#include "engine/include/core/logger.h"
#include <math/vec3.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// GOAP System - Goal-Oriented Action Planning for AI

// Forward declarations
typedef struct GoapAgent GoapAgent;
typedef struct GoapAction GoapAction;
typedef struct GoapGoal GoapGoal;
typedef struct GoapWorldState GoapWorldState;
typedef struct GoapPlanner GoapPlanner;

// World state representation
#define GOAP_MAX_STATE_KEYS 64
#define GOAP_MAX_ACTIONS 32
#define GOAP_MAX_GOALS 16
#define GOAP_MAX_PLAN_LENGTH 32

typedef enum {
    GOAP_STATE_BOOL,
    GOAP_STATE_INT,
    GOAP_STATE_FLOAT,
    GOAP_STATE_VECTOR3
} GoapStateType;

typedef struct {
    char name[32];
    GoapStateType type;
    union {
        bool bool_val;
        int int_val;
        float float_val;
        Vec3 vector_val;
    } value;
} GoapStateKey;

struct GoapWorldState {
    GoapStateKey keys[GOAP_MAX_STATE_KEYS];
    int key_count;
    float timestamp;
};

// Action representation
typedef struct {
    char name[32];
    GoapStateType type;
    union {
        bool bool_val;
        int int_val;
        float float_val;
        Vec3 vector_val;
    } value;
} GoapStateCondition;

struct GoapAction {
    char name[32];
    float cost;
    float duration;
    
    // Preconditions (must be true to execute)
    GoapStateCondition preconditions[GOAP_MAX_STATE_KEYS];
    int precondition_count;
    
    // Effects (what changes after execution)
    GoapStateCondition effects[GOAP_MAX_STATE_KEYS];
    int effect_count;
    
    // Action validation callback
    bool (*can_execute)(GoapAgent* agent, const GoapWorldState* state);
    
    // Action execution callback
    void (*execute)(GoapAgent* agent, GoapWorldState* state);
    
    // Action completion check
    bool (*is_complete)(GoapAgent* agent);
};

// Goal representation
struct GoapGoal {
    char name[32];
    float priority;
    
    // Goal conditions (what we want to achieve)
    GoapStateCondition conditions[GOAP_MAX_STATE_KEYS];
    int condition_count;
    
    // Goal validation callback
    bool (*is_valid)(GoapAgent* agent, const GoapWorldState* state);
    
    // Goal priority calculation
    float (*calculate_priority)(GoapAgent* agent, const GoapWorldState* state);
};

// Agent representation
struct GoapAgent {
    uint32_t id;
    char name[32];
    
    // Current world state
    GoapWorldState current_state;
    
    // Available actions
    GoapAction* actions[GOAP_MAX_ACTIONS];
    int action_count;
    
    // Active goals
    GoapGoal* goals[GOAP_MAX_GOALS];
    int goal_count;
    
    // Current plan
    GoapAction* current_plan[GOAP_MAX_PLAN_LENGTH];
    int plan_length;
    int current_action_index;
    
    // Planning state
    bool needs_replan;
    float last_plan_time;
    float replan_interval;
    
    // Agent-specific data
    void* user_data;
};

// Planner representation
struct GoapPlanner {
    bool initialized;
    
    // A* planning data
    struct PlanNode* open_list;
    struct PlanNode* closed_list;
    
    // Configuration
    int max_plan_length;
    float max_search_time;

    // Statistics
    int nodes_expanded;
    float planning_time_ms;
    int plans_generated;
};

// Plan node for A* search
struct PlanNode {
    GoapWorldState state;
    float g_cost;  // Cost from start
    float h_cost;  // Heuristic cost to goal
    float f_cost;  // Total cost
    GoapAction* action;  // Action that led to this state
    struct PlanNode* parent;
    struct PlanNode* next;  // For open/closed list linked lists
    bool in_open;
    bool in_closed;
    int depth;
};

// Core GOAP functions
bool goap_init(GoapPlanner* planner);
void goap_shutdown(GoapPlanner* planner);

GoapAgent* goap_create_agent(uint32_t id, const char* name);
void goap_destroy_agent(GoapAgent* agent);

bool goap_add_action(GoapAgent* agent, GoapAction* action);
bool goap_add_goal(GoapAgent* agent, GoapGoal* goal);

// Planning functions
bool goap_plan(GoapPlanner* planner, GoapAgent* agent, const GoapWorldState* start, 
               const GoapGoal* goal, GoapAction** plan, int* plan_length);
bool goap_replan_if_needed(GoapPlanner* planner, GoapAgent* agent);

// Execution functions
void goap_update_agent(GoapAgent* agent, float delta_time);
bool goap_execute_current_action(GoapAgent* agent, float delta_time);

// World state functions
void goap_world_state_init(GoapWorldState* state);
void goap_world_state_copy(GoapWorldState* dest, const GoapWorldState* src);
bool goap_world_state_get(const GoapWorldState* state, const char* key, GoapStateKey* out_key);
bool goap_world_state_set(GoapWorldState* state, const char* key, GoapStateType type, void* value);
bool goap_world_state_meets_conditions(const GoapWorldState* state, 
                                      const GoapStateCondition* conditions, int count);

// Action functions
GoapAction* goap_create_action(const char* name, float cost);
void goap_action_add_precondition(GoapAction* action, const char* key, 
                                GoapStateType type, void* value);
void goap_action_add_effect(GoapAction* action, const char* key, 
                           GoapStateType type, void* value);
void goap_action_apply_effects(GoapAction* action, GoapWorldState* state);
bool goap_action_can_execute(const GoapAction* action, const GoapAgent* agent, 
                            const GoapWorldState* state);

// Goal functions
GoapGoal* goap_create_goal(const char* name, float priority);
void goap_goal_add_condition(GoapGoal* goal, const char* key, 
                             GoapStateType type, void* value);
bool goap_goal_is_achieved(const GoapGoal* goal, const GoapWorldState* state);
GoapGoal* goap_select_best_goal(GoapAgent* agent);

// Utility functions
float goap_calculate_heuristic(const GoapWorldState* current, const GoapGoal* goal);
void goap_debug_print_plan(const GoapAction** plan, int length);
void goap_debug_print_state(const GoapWorldState* state);

#ifdef __cplusplus
}
#endif

#endif // GOAP_H
