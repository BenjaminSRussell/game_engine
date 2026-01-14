// include/ai/planning/htn_planner.h
// Purpose: Hierarchical Task Network (HTN) planner for complex AI task decomposition

#ifndef HTN_PLANNER_H
#define HTN_PLANNER_H

#include "ai/ai_types.h"
#include <common.h>

// HTN Task types
typedef enum {
    HTN_TASK_PRIMITIVE,    // Directly executable action
    HTN_TASK_COMPOUND     // Needs decomposition into subtasks
} HtnTaskType;

// HTN Task structure
typedef struct {
    const char* name;
    HtnTaskType type;
    u32 id;               // Unique task identifier
    // For primitive tasks
    void (*execute)(AIEntityID entity);
    f32 cost;
    // For compound tasks
    u32* method_ids;      // Available methods for decomposition
    u32 method_count;
} HtnTask;

// HTN Method structure (decomposition rule for compound tasks)
typedef struct {
    const char* name;
    u32 task_id;          // Parent compound task this method decomposes
    u32* subtask_ids;     // Ordered list of subtasks
    u32 subtask_count;
    GOAPState preconditions;  // Conditions for this method to be valid
    bool (*proc_check)(AIEntityID entity);  // Dynamic precondition check
    f32 priority;         // Method priority (higher = preferred)
    u32 id;               // Unique method identifier
} HtnMethod;

// HTN Domain (collection of tasks and methods)
typedef struct {
    HtnTask* tasks;
    u32 task_count;
    u32 max_tasks;
    HtnMethod* methods;
    u32 method_count;
    u32 max_methods;
    const char* name;
} HtnDomain;

// HTN Planning state record
typedef struct {
    GOAPState world_state;
    u32* task_sequence;   // Current task sequence
    u32 task_count;
    u32 max_tasks;
    f32 total_cost;
    u32 depth;            // Current planning depth
    u32 parent_method_id; // Method that created this state
} HtnPlanState;

// HTN Planner
typedef struct {
    HtnDomain* domain;
    HtnPlanState* current_state;
    HtnPlanState* best_state;
    HtnPlanState* state_stack;
    u32 stack_capacity;
    u32 stack_depth;
    u32 max_depth;
    u32 max_cost;
    bool planning_complete;
    bool plan_found;
    u32 iterations;
    u32 max_iterations;
    bool early_rejection_enabled;
    f32 cost_threshold;
} HtnPlanner;

// HTN Planner functions
HtnPlanner* htn_planner_create(HtnDomain* domain, u32 max_depth, u32 max_cost);
void htn_planner_destroy(HtnPlanner* planner);

// Planning functions
bool htn_planner_decompose_task(HtnPlanner* planner, u32 task_id, GOAPState initial_state, AIEntityID entity_id);
bool htn_is_planning_complete(const HtnPlanner* planner);
bool htn_was_plan_found(const HtnPlanner* planner);
const u32* htn_get_task_plan(const HtnPlanner* planner, u32* plan_length);

// Domain management
HtnDomain* htn_domain_create(const char* name, u32 max_tasks, u32 max_methods);
void htn_domain_destroy(HtnDomain* domain);
u32 htn_domain_add_task(HtnDomain* domain, const HtnTask* task);
u32 htn_domain_add_method(HtnDomain* domain, const HtnMethod* method);
HtnTask* htn_domain_get_task(HtnDomain* domain, u32 task_id);
HtnMethod* htn_domain_get_method(HtnDomain* domain, u32 method_id);

// Task and method creation helpers
u32 htn_create_primitive_task(HtnDomain* domain, const char* name, void (*execute)(AIEntityID), f32 cost);
u32 htn_create_compound_task(HtnDomain* domain, const char* name);
u32 htn_create_method(HtnDomain* domain, u32 task_id, const u32* subtask_ids, u32 subtask_count, 
                      GOAPState preconditions, f32 priority);

// Plan execution
void htn_execute_plan(HtnPlanner* planner, AIEntityID entity_id);
void htn_reset_planner(HtnPlanner* planner);

// Utility functions
bool htn_is_task_primitive(const HtnTask* task);
bool htn_is_task_complete(const HtnPlanState* state, u32 task_id);
f32 htn_calculate_plan_cost(const HtnPlanState* state, const HtnDomain* domain);
void htn_print_plan(const HtnPlanner* planner);
void htn_print_domain(const HtnDomain* domain);

// Advanced features
void htn_enable_early_rejection(HtnPlanner* planner, bool enabled, f32 cost_threshold);
bool htn_partial_plan_slicing(HtnPlanner* planner, u32 frames_per_update);
void htn_serialize_domain(const HtnDomain* domain, const char* filename);
HtnDomain* htn_deserialize_domain(const char* filename);

// Debug and statistics
u32 htn_get_iterations(const HtnPlanner* planner);
u32 htn_get_plan_depth(const HtnPlanner* planner);
f32 htn_get_plan_cost(const HtnPlanner* planner);
u32 htn_get_stack_depth(const HtnPlanner* planner);

#endif // HTN_PLANNER_H
