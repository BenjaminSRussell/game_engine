#ifndef GOAP_ENHANCED_H
#define GOAP_ENHANCED_H

#include <ai/goap.h>
#include <core/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// AGENT_AI_1 Phase 5-8: Enhanced GOAP with Bitfield World State

// === Phase 5: Optimized World State Representation ===

#define GOAP_MAX_STATE_ATOMS 64 // Must be <= 64 for bitfield optimization

// Pre-defined common world state atoms (can be extended)
typedef enum {
  // Combat
  GOAP_ATOM_HAS_WEAPON = 0,
  GOAP_ATOM_HAS_AMMO,
  GOAP_ATOM_WEAPON_LOADED,
  GOAP_ATOM_IN_COMBAT,
  GOAP_ATOM_TARGET_VISIBLE,
  GOAP_ATOM_TARGET_IN_RANGE,
  GOAP_ATOM_UNDER_FIRE,

  // Health & Resources
  GOAP_ATOM_HEALTHY, // Health > 70%
  GOAP_ATOM_WOUNDED, // Health < 30%
  GOAP_ATOM_HAS_MEDKIT,
  GOAP_ATOM_HAS_ARMOR,

  // Position & Movement
  GOAP_ATOM_AT_COVER,
  GOAP_ATOM_AT_OBJECTIVE,
  GOAP_ATOM_PATH_CLEAR,
  GOAP_ATOM_DOOR_OPEN,

  // Perception
  GOAP_ATOM_ENEMY_HEARD,
  GOAP_ATOM_ENEMY_SEEN,
  GOAP_ATOM_ALERT,
  GOAP_ATOM_INVESTIGATING,

  // Objectives
  GOAP_ATOM_OBJECTIVE_COMPLETE,
  GOAP_ATOM_ITEM_COLLECTED,
  GOAP_ATOM_AREA_SECURED,

  // Squad
  GOAP_ATOM_IN_FORMATION,
  GOAP_ATOM_FOLLOWING_LEADER,
  GOAP_ATOM_SQUAD_READY,

  // Count & User-defined atoms follow
  GOAP_ATOM_BUILTIN_COUNT,

  // Reserve space for user-defined atoms
  GOAP_ATOM_USER_0 = 32,
  GOAP_ATOM_USER_1,
  GOAP_ATOM_USER_2,
  // ... up to 64 total
} GoapAtomID;

// Bitfield-based world state (64 boolean atoms)
typedef struct {
  u64 bits;        // Bitfield for fast operations
  float timestamp; // When state was captured
  u32 state_hash;  // Hash for quick equality checks
} GoapWorldStateBits;

// Enhanced world state with both bitfield and extended data
typedef struct {
  GoapWorldStateBits bits; // Fast boolean state

  // Extended numeric/vector state (for non-boolean data)
  struct {
    GoapAtomID atom_id;
    GoapStateType type;
    union {
      int int_val;
      float float_val;
      Vec3 vector_val;
    } value;
  } extended_data[16];
  int extended_count;

  // Metadata
  char description[64]; // For debugging
} GoapWorldStateEnhanced;

// State difference result
typedef struct {
  u64 added_bits;   // Atoms that became true
  u64 removed_bits; // Atoms that became false
  u64 changed_bits; // Union of added and removed
  int change_count; // Number of atoms that changed
} GoapStateDiff;

// === Phase 5 API: World State Operations ===

// Bitfield operations (O(1))
void goap_state_set_atom(GoapWorldStateEnhanced *state, GoapAtomID atom,
                         bool value);
bool goap_state_get_atom(const GoapWorldStateEnhanced *state, GoapAtomID atom);
void goap_state_clear(GoapWorldStateEnhanced *state);
void goap_state_copy(GoapWorldStateEnhanced *dest,
                     const GoapWorldStateEnhanced *src);

// Bulk operations
void goap_state_set_multiple(GoapWorldStateEnhanced *state,
                             const GoapAtomID *atoms, const bool *values,
                             int count);
u64 goap_state_get_bits(const GoapWorldStateEnhanced *state);
void goap_state_set_bits(GoapWorldStateEnhanced *state, u64 bits);

// State comparison (O(1) for bitfields)
bool goap_state_equals(const GoapWorldStateEnhanced *a,
                       const GoapWorldStateEnhanced *b);
int goap_state_count_differences(const GoapWorldStateEnhanced *a,
                                 const GoapWorldStateEnhanced *b);
void goap_state_diff(const GoapWorldStateEnhanced *from,
                     const GoapWorldStateEnhanced *to, GoapStateDiff *diff);

// Precondition checking
bool goap_state_satisfies(const GoapWorldStateEnhanced *state,
                          const GoapWorldStateEnhanced *requirements);
bool goap_state_satisfies_any(const GoapWorldStateEnhanced *state,
                              const GoapWorldStateEnhanced *requirements);

// Hash & equality
u32 goap_state_compute_hash(const GoapWorldStateEnhanced *state);
void goap_state_update_hash(GoapWorldStateEnhanced *state);

// Extended data (for numeric/vector atoms)
void goap_state_set_int(GoapWorldStateEnhanced *state, GoapAtomID atom,
                        int value);
void goap_state_set_float(GoapWorldStateEnhanced *state, GoapAtomID atom,
                          float value);
void goap_state_set_vector(GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 value);

int goap_state_get_int(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                       int default_val);
float goap_state_get_float(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           float default_val);
Vec3 goap_state_get_vector(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 default_val);

// === Phase 6: Enhanced Action Definitions ===

typedef struct {
  char name[64];
  float cost;
  float duration;

  // Preconditions & effects using bitfields
  GoapWorldStateEnhanced preconditions;
  GoapWorldStateEnhanced effects;

  // Procedural checks
  bool (*check_precondition)(void *agent,
                             const GoapWorldStateEnhanced *world_state);
  void (*apply_effects)(void *agent, GoapWorldStateEnhanced *world_state);

  // Execution
  bool (*can_execute)(void *agent);
  void (*execute)(void *agent, float delta_time);
  bool (*is_complete)(void *agent);

  // Metadata
  int execution_count;
  float total_execution_time;
  float average_duration;
} GoapActionEnhanced;

// Action API
GoapActionEnhanced *goap_action_create_enhanced(const char *name, float cost);
void goap_action_destroy_enhanced(GoapActionEnhanced *action);

void goap_action_add_precondition_bit(GoapActionEnhanced *action,
                                      GoapAtomID atom, bool value);
void goap_action_add_effect_bit(GoapActionEnhanced *action, GoapAtomID atom,
                                bool value);

bool goap_action_can_run(const GoapActionEnhanced *action,
                         const GoapWorldStateEnhanced *state);
void goap_action_apply(const GoapActionEnhanced *action,
                       GoapWorldStateEnhanced *state);

// === Phase 7: Enhanced A* Planner ===

typedef struct {
  GoapWorldStateEnhanced state;
  const GoapActionEnhanced *action;
  int parent_index;
  float g_cost; // Cost from start
  float h_cost; // Heuristic to goal
  float f_cost; // Total cost (g + h)
  int depth;
} GoapPlannerNode;

typedef struct GoapPlannerState {
  GoapPlannerNode *open_list;
  GoapPlannerNode *closed_list;
  int open_count;
  int closed_count;
  int open_capacity;
  int closed_capacity;

  // Statistics
  int nodes_expanded;
  int nodes_generated;
  float planning_time;
  int max_depth_reached;
} GoapPlannerState;

typedef struct {
  const GoapActionEnhanced **actions;
  int action_count;
  int max_plan_length;
} GoapPlan;

// Planner API
GoapPlannerState *goap_planner_create_state(int capacity);
void goap_planner_destroy_state(GoapPlannerState *planner);

GoapPlan *goap_plan_enhanced(GoapPlannerState *planner,
                             const GoapActionEnhanced **available_actions,
                             int action_count,
                             const GoapWorldStateEnhanced *start_state,
                             const GoapWorldStateEnhanced *goal_state,
                             int max_plan_length);

void goap_plan_destroy(GoapPlan *plan);

// Heuristic function (Phase 7)
float goap_heuristic_enhanced(const GoapWorldStateEnhanced *current,
                              const GoapWorldStateEnhanced *goal);

// === Phase 8: Plan Execution & Validation ===

typedef enum {
  GOAP_EXEC_IDLE,
  GOAP_EXEC_RUNNING,
  GOAP_EXEC_SUCCESS,
  GOAP_EXEC_FAILED,
  GOAP_EXEC_INVALID // Plan no longer valid
} GoapExecutionStatus;

typedef struct {
  GoapPlan *plan;
  int current_step;
  GoapExecutionStatus status;

  // Validation
  GoapWorldStateEnhanced expected_state; // What we expect after each action
  bool validate_each_step;

  // Replan triggers
  float replan_threshold; // State drift threshold
  bool needs_replan;

  // Statistics
  float execution_time;
  int steps_completed;
  int replans_triggered;
} GoapPlanExecutor;

// Plan execution API
GoapPlanExecutor *goap_executor_create(GoapPlan *plan);
void goap_executor_destroy(GoapPlanExecutor *executor);

GoapExecutionStatus goap_executor_tick(GoapPlanExecutor *executor, void *agent,
                                       GoapWorldStateEnhanced *current_state,
                                       float delta_time);

bool goap_executor_validate_plan(const GoapPlanExecutor *executor,
                                 const GoapWorldStateEnhanced *current_state);

void goap_executor_trigger_replan(GoapPlanExecutor *executor);

// Debug
void goap_state_print(const GoapWorldStateEnhanced *state);
void goap_plan_print(const GoapPlan *plan);
const char *goap_atom_name(GoapAtomID atom);

#ifdef __cplusplus
}
#endif

#endif // GOAP_ENHANCED_H
