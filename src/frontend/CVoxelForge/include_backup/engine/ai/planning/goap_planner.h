// include/ai/planning/goap_planner.h
// Purpose: Goal Oriented Action Planning (GOAP) system for AI decision making

#ifndef GOAP_PLANNER_H
#define GOAP_PLANNER_H

#include "../ai_types.h"
#include "../../common.h"

// GOAP state representation using bitmask
#define GOAP_MAX_ATOMS 64
typedef u64 GOAPState;

// GOAP atom identifiers
typedef enum {
    GOAP_ATOM_HAS_WEAPON = 1ULL << 0,
    GOAP_ATOM_HAS_AMMO = 1ULL << 1,
    GOAP_ATOM_ENEMY_VISIBLE = 1ULL << 2,
    GOAP_ATOM_ENEMY_IN_RANGE = 1ULL << 3,
    GOAP_ATOM_HEALTH_LOW = 1ULL << 4,
    GOAP_ATOM_COVER_AVAILABLE = 1ULL << 5,
    GOAP_ATOM_TARGET_DEAD = 1ULL << 6,
    GOAP_ATOM_AT_OBJECTIVE = 1ULL << 7,
    GOAP_ATOM_HAS_HEALTH = 1ULL << 8,
    GOAP_ATOM_HAS_COVER = 1ULL << 9
} GOAPAtom;

// GOAP action structure
typedef struct {
    const char* name;
    GOAPState preconditions;    // Required state
    GOAPState effects;         // State changes
    f32 cost;                 // Action cost
    bool (*proc_check)(AIEntityID entity);  // Dynamic precondition check
    void (*execute)(AIEntityID entity);      // Action execution
    u32 id;                   // Unique action ID
} GoapAction;

// GOAP plan node for A* search
typedef struct {
    GOAPState state;
    f32 g_cost;              // Cost from start
    f32 h_cost;              // Heuristic cost to goal
    f32 f_cost;              // Total cost (g + h)
    u32 parent_action_id;    // Action that led to this state
    u32 depth;               // Depth in search tree
} GoapPlanNode;

// GOAP planner context
typedef struct {
    GoapAction* actions;
    u32 action_count;
    GoapPlanNode* open_list;
    u32 open_count;
    u32 open_capacity;
    GoapPlanNode* closed_list;
    u32 closed_count;
    u32 closed_capacity;
    u32* action_plan;
    u32 plan_length;
    bool planning_complete;
    bool plan_found;
    u32 max_plan_depth;
    u32 nodes_expanded;
} GoapPlanner;

// GOAP planner functions
GoapPlanner* goap_planner_create(u32 max_actions, u32 max_plan_depth);
void goap_planner_destroy(GoapPlanner* planner);

// Action management
void goap_planner_add_action(GoapPlanner* planner, const GoapAction* action);
GoapAction* goap_planner_get_action(GoapPlanner* planner, u32 action_id);
void goap_planner_clear_actions(GoapPlanner* planner);

// Planning functions
bool goap_plan_action_sequence(GoapPlanner* planner, GOAPState start_state, GOAPState goal_state, 
                              AIEntityID entity_id);
bool goap_is_planning_complete(const GoapPlanner* planner);
bool goap_was_plan_found(const GoapPlanner* planner);
const u32* goap_get_action_plan(const GoapPlanner* planner, u32* plan_length);

// State manipulation functions
GOAPState goap_create_state(u32 atom_bits[GOAP_MAX_ATOMS / 32]);
void goap_set_atom(GOAPState* state, GOAPAtom atom);
void goap_clear_atom(GOAPState* state, GOAPAtom atom);
bool goap_has_atom(GOAPState state, GOAPAtom atom);
GOAPState goap_apply_effects(GOAPState state, const GoapAction* action);
bool goap_meets_preconditions(GOAPState state, const GoapAction* action, AIEntityID entity_id);

// Heuristic functions
f32 goap_heuristic_distance(GOAPState current, GOAPState goal);
f32 goap_heuristic_weighted(GOAPState current, GOAPState goal, const f32* atom_weights);

// Plan execution functions
void goap_execute_plan(GoapPlanner* planner, AIEntityID entity_id);
void goap_reset_planner(GoapPlanner* planner);

// Utility functions
u32 goap_count_differences(GOAPState state1, GOAPState state2);
GOAPState goap_difference_mask(GOAPState state1, GOAPState state2);
void goap_print_state(GOAPState state);
void goap_print_plan(const GoapPlanner* planner, const GoapAction* actions);

// Debug and statistics
u32 goap_get_nodes_expanded(const GoapPlanner* planner);
u32 goap_get_plan_length(const GoapPlanner* planner);
f32 goap_get_plan_cost(const GoapPlanner* planner, const GoapAction* actions);

#endif // GOAP_PLANNER_H
