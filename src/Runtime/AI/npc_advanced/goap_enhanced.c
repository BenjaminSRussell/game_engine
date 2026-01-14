#include "../../include/ai/npc_advanced/goap_enhanced.h"
#include "../../include/common.h"
#include "../../include/core/logger.h"
#include "../../include/core/memory.h"
#include "../../include/core/types.h"
#include "../../include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                    ENHANCED GOAP SYSTEM - AGENT_AI_1 (Phase 5-8)
 * =================================================================================================
 *
 * PURPOSE: High-performance planning with bitfield-based world state.
 *
 * OPTIMIZATIONS:
 * - O(1) state operations using 64-bit bitfields
 * - O(1) state equality and difference checks
 * - Improved A* heuristic using Hamming distance
 * - Plan validation and incremental replanning
 * =================================================================================================
 */

// ===========================================================================================
// PHASE 5: OPTIMIZED WORLD STATE OPERATIONS
// ===========================================================================================

// Set a single atom (O(1))
void goap_state_set_atom(GoapWorldStateEnhanced *state, GoapAtomID atom,
                         bool value) {
  if (!state || atom >= GOAP_MAX_STATE_ATOMS)
    return;

  if (value) {
    state->bits.bits |= (1ULL << atom); // Set bit
  } else {
    state->bits.bits &= ~(1ULL << atom); // Clear bit
  }

  // Invalidate hash
  state->bits.state_hash = 0;
}

// Get a single atom (O(1))
bool goap_state_get_atom(const GoapWorldStateEnhanced *state, GoapAtomID atom) {
  if (!state || atom >= GOAP_MAX_STATE_ATOMS)
    return false;
  return (state->bits.bits & (1ULL << atom)) != 0;
}

// Clear all atoms
void goap_state_clear(GoapWorldStateEnhanced *state) {
  if (!state)
    return;
  memset(state, 0, sizeof(GoapWorldStateEnhanced));
}

// Copy state
void goap_state_copy(GoapWorldStateEnhanced *dest,
                     const GoapWorldStateEnhanced *src) {
  if (!dest || !src)
    return;
  memcpy(dest, src, sizeof(GoapWorldStateEnhanced));
}

// Set multiple atoms at once
void goap_state_set_multiple(GoapWorldStateEnhanced *state,
                             const GoapAtomID *atoms, const bool *values,
                             int count) {
  if (!state || !atoms || !values)
    return;

  for (int i = 0; i < count; i++) {
    goap_state_set_atom(state, atoms[i], values[i]);
  }
}

// Get raw bitfield
u64 goap_state_get_bits(const GoapWorldStateEnhanced *state) {
  return state ? state->bits.bits : 0;
}

// Set raw bitfield
void goap_state_set_bits(GoapWorldStateEnhanced *state, u64 bits) {
  if (!state)
    return;
  state->bits.bits = bits;
  state->bits.state_hash = 0; // Invalidate hash
}

// Check exact equality (O(1))
bool goap_state_equals(const GoapWorldStateEnhanced *a,
                       const GoapWorldStateEnhanced *b) {
  if (!a || !b)
    return false;

  // Fast path: compare bitfields
  if (a->bits.bits != b->bits.bits)
    return false;

  // Extended data comparison (rare case)
  if (a->extended_count != b->extended_count)
    return false;

  for (int i = 0; i < a->extended_count; i++) {
    bool found = false;
    for (int j = 0; j < b->extended_count; j++) {
      if (a->extended_data[i].atom_id == b->extended_data[j].atom_id) {
        if (a->extended_data[i].type != b->extended_data[j].type)
          return false;

        switch (a->extended_data[i].type) {
        case GOAP_STATE_INT:
          if (a->extended_data[i].value.int_val !=
              b->extended_data[j].value.int_val)
            return false;
          break;
        case GOAP_STATE_FLOAT:
          if (fabsf(a->extended_data[i].value.float_val -
                    b->extended_data[j].value.float_val) > 0.001f)
            return false;
          break;
        default:
          break;
        }
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  return true;
}

// Count bit differences (Hamming distance) - O(1)
int goap_state_count_differences(const GoapWorldStateEnhanced *a,
                                 const GoapWorldStateEnhanced *b) {
  if (!a || !b)
    return 64;

  u64 xor_bits = a->bits.bits ^ b->bits.bits;

  // Count set bits (population count)
  int count = 0;
  while (xor_bits) {
    count += xor_bits & 1;
    xor_bits >>= 1;
  }

  return count;
}

// Compute detailed difference
void goap_state_diff(const GoapWorldStateEnhanced *from,
                     const GoapWorldStateEnhanced *to, GoapStateDiff *diff) {
  if (!from || !to || !diff)
    return;

  memset(diff, 0, sizeof(GoapStateDiff));

  diff->added_bits =
      to->bits.bits & ~from->bits.bits; // Bits in 'to' but not 'from'
  diff->removed_bits =
      from->bits.bits & ~to->bits.bits; // Bits in 'from' but not 'to'
  diff->changed_bits = diff->added_bits | diff->removed_bits;

  // Count changes
  u64 changes = diff->changed_bits;
  while (changes) {
    diff->change_count += (int)(changes & 1);
    changes >>= 1;
  }
}

// Check if state satisfies requirements (ALL atoms must match)
bool goap_state_satisfies(const GoapWorldStateEnhanced *state,
                          const GoapWorldStateEnhanced *requirements) {
  if (!state || !requirements)
    return false;

  // Check if all required bits are set
  u64 required_bits = requirements->bits.bits;
  return (state->bits.bits & required_bits) == required_bits;
}

// Check if state satisfies ANY requirements
bool goap_state_satisfies_any(const GoapWorldStateEnhanced *state,
                              const GoapWorldStateEnhanced *requirements) {
  if (!state || !requirements)
    return false;

  // Check if ANY required bit is set
  return (state->bits.bits & requirements->bits.bits) != 0;
}

// Compute hash for state
u32 goap_state_compute_hash(const GoapWorldStateEnhanced *state) {
  if (!state)
    return 0;

  // Simple hash: XOR folding of 64-bit value
  u32 hash = (u32)(state->bits.bits ^ (state->bits.bits >> 32));

  // Mix in extended data
  for (int i = 0; i < state->extended_count; i++) {
    hash ^= state->extended_data[i].atom_id;
    hash ^= state->extended_data[i].value.int_val; // Works for int/float
  }

  return hash;
}

void goap_state_update_hash(GoapWorldStateEnhanced *state) {
  if (!state)
    return;
  state->bits.state_hash = goap_state_compute_hash(state);
}

// ===========================================================================================
// EXTENDED DATA (Non-boolean atoms)
// ===========================================================================================

static int find_extended_data(const GoapWorldStateEnhanced *state,
                              GoapAtomID atom) {
  for (int i = 0; i < state->extended_count; i++) {
    if (state->extended_data[i].atom_id == atom) {
      return i;
    }
  }
  return -1;
}

void goap_state_set_int(GoapWorldStateEnhanced *state, GoapAtomID atom,
                        int value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add int for atom %d", atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_INT;
  state->extended_data[idx].value.int_val = value;
  state->bits.state_hash = 0; // Invalidate hash
}

void goap_state_set_float(GoapWorldStateEnhanced *state, GoapAtomID atom,
                          float value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add float for atom %d", atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_FLOAT;
  state->extended_data[idx].value.float_val = value;
  state->bits.state_hash = 0;
}

void goap_state_set_vector(GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add vector for atom %d",
                atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_VECTOR3;
  state->extended_data[idx].value.vector_val = value;
  state->bits.state_hash = 0;
}

int goap_state_get_int(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                       int default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_INT) {
    return state->extended_data[idx].value.int_val;
  }
  return default_val;
}

float goap_state_get_float(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           float default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_FLOAT) {
    return state->extended_data[idx].value.float_val;
  }
  return default_val;
}

Vec3 goap_state_get_vector(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_VECTOR3) {
    return state->extended_data[idx].value.vector_val;
  }
  return default_val;
}

// ===========================================================================================
// DEBUG
// ===========================================================================================

const char *goap_atom_name(GoapAtomID atom) {
  switch (atom) {
  case GOAP_ATOM_HAS_WEAPON:
    return "HAS_WEAPON";
  case GOAP_ATOM_HAS_AMMO:
    return "HAS_AMMO";
  case GOAP_ATOM_WEAPON_LOADED:
    return "WEAPON_LOADED";
  case GOAP_ATOM_IN_COMBAT:
    return "IN_COMBAT";
  case GOAP_ATOM_TARGET_VISIBLE:
    return "TARGET_VISIBLE";
  case GOAP_ATOM_TARGET_IN_RANGE:
    return "TARGET_IN_RANGE";
  case GOAP_ATOM_UNDER_FIRE:
    return "UNDER_FIRE";
  case GOAP_ATOM_HEALTHY:
    return "HEALTHY";
  case GOAP_ATOM_WOUNDED:
    return "WOUNDED";
  case GOAP_ATOM_HAS_MEDKIT:
    return "HAS_MEDKIT";
  case GOAP_ATOM_HAS_ARMOR:
    return "HAS_ARMOR";
  case GOAP_ATOM_AT_COVER:
    return "AT_COVER";
  case GOAP_ATOM_AT_OBJECTIVE:
    return "AT_OBJECTIVE";
  case GOAP_ATOM_PATH_CLEAR:
    return "PATH_CLEAR";
  case GOAP_ATOM_DOOR_OPEN:
    return "DOOR_OPEN";
  case GOAP_ATOM_ENEMY_HEARD:
    return "ENEMY_HEARD";
  case GOAP_ATOM_ENEMY_SEEN:
    return "ENEMY_SEEN";
  case GOAP_ATOM_ALERT:
    return "ALERT";
  case GOAP_ATOM_INVESTIGATING:
    return "INVESTIGATING";
  case GOAP_ATOM_OBJECTIVE_COMPLETE:
    return "OBJECTIVE_COMPLETE";
  case GOAP_ATOM_ITEM_COLLECTED:
    return "ITEM_COLLECTED";
  case GOAP_ATOM_AREA_SECURED:
    return "AREA_SECURED";
  case GOAP_ATOM_IN_FORMATION:
    return "IN_FORMATION";
  case GOAP_ATOM_FOLLOWING_LEADER:
    return "FOLLOWING_LEADER";
  case GOAP_ATOM_SQUAD_READY:
    return "SQUAD_READY";
  default:
    return "UNKNOWN";
  }
}

void goap_state_print(const GoapWorldStateEnhanced *state) {
  if (!state)
    return;

  LOG_INFO("=== GOAP World State ===");
  LOG_INFO("Bits: 0x%016llX", state->bits.bits);
  LOG_INFO("Active atoms:");

  for (int i = 0; i < GOAP_MAX_STATE_ATOMS; i++) {
    if (goap_state_get_atom(state, i)) {
      LOG_INFO("  [%d] %s", i, goap_atom_name(i));
    }
  }

  if (state->extended_count > 0) {
    LOG_INFO("Extended data:");
    for (int i = 0; i < state->extended_count; i++) {
      GoapAtomID atom = state->extended_data[i].atom_id;
      switch (state->extended_data[i].type) {
      case GOAP_STATE_INT:
        LOG_INFO("  %s = %d", goap_atom_name(atom),
                 state->extended_data[i].value.int_val);
        break;
      case GOAP_STATE_FLOAT:
        LOG_INFO("  %s = %.2f", goap_atom_name(atom),
                 state->extended_data[i].value.float_val);
        break;
      default:
        break;
      }
    }
  }
}
// ===========================================================================================
// PHASE 6: ENHANCED ACTION DEFINITIONS
// ===========================================================================================

GoapActionEnhanced *goap_action_create_enhanced(const char *name, float cost) {
  GoapActionEnhanced *action = (GoapActionEnhanced *)MALLOC_TAGGED(
      sizeof(GoapActionEnhanced), MEMORY_TAG_AI);
  if (!action) {
    LOG_ERROR("Failed to allocate GOAP action");
    return NULL;
  }

  memset(action, 0, sizeof(GoapActionEnhanced));
  strncpy(action->name, name, sizeof(action->name) - 1);
  action->cost = cost;
  action->duration = 1.0f;

  return action;
}

void goap_action_destroy_enhanced(GoapActionEnhanced *action) {
  if (!action)
    return;
  FREE(0);
}

void goap_action_add_precondition_bit(GoapActionEnhanced *action,
                                      GoapAtomID atom, bool value) {
  if (!action)
    return;
  goap_state_set_atom(&action->preconditions, atom, value);
}

void goap_action_add_effect_bit(GoapActionEnhanced *action, GoapAtomID atom,
                                bool value) {
  if (!action)
    return;
  goap_state_set_atom(&action->effects, atom, value);
}

GoapPlan *goap_action_expand_macro(const GoapActionEnhanced *action,
                                   void *agent,
                                   const GoapWorldStateEnhanced *state) {
  if (!action || !action->is_macro)
    return NULL;

  // Simple hierarchical expansion: Create a sub-plan using the action's effects
  // as a goal In a more complex system, the macro action might define a
  // specific sub-goal atom.

  GoapPlannerState *temp_planner = goap_planner_create_state(64);

  // For now, we use a simple heuristic: the macro action is a container for a
  // sub-plan that achieves the macro's effects from the current state.

  // Note: In Phase 9, we would use a specialized sub-action library here.
  // For this implementation, we use an empty action set to represent a
  // "To-be-planned" expansion.
  LOG_INFO("Expanding macro action: %s using sub-planning", action->name);

  // Clean up existing sub-plan if needed
  if (action->sub_plan) {
    goap_plan_destroy(action->sub_plan);
  }

  return NULL; // Still return NULL as we need a pool of sub-actions to actually
               // plan.
}

bool goap_action_can_run(const GoapActionEnhanced *action,
                         const GoapWorldStateEnhanced *state) {
  if (!action || !state)
    return false;

  if (!goap_state_satisfies(state, &action->preconditions)) {
    return false;
  }

  if (action->check_precondition) {
    return action->check_precondition(NULL, state);
  }

  return true;
}

float goap_action_get_cost(const GoapActionEnhanced *action, void *agent,
                           const GoapWorldStateEnhanced *state) {
  if (!action)
    return 1000.0f;

  float cost = action->cost;

  // Apply dynamic cost multiplier (Utility)
  if (action->calculate_dynamic_cost) {
    cost *= action->calculate_dynamic_cost(agent, state);
  }

  return cost;
}

bool goap_action_is_macro(const GoapActionEnhanced *action) {
  return action ? action->is_macro : false;
}

// ===========================================================================================
// PHASE 7: ENHANCED A* PLANNER
// ===========================================================================================

static void swap_nodes(GoapPlannerNode *a, GoapPlannerNode *b) {
  GoapPlannerNode temp = *a;
  *a = *b;
  *b = temp;
}

static void heapify_up(GoapPlannerState *planner, int index) {
  while (index > 0) {
    int parent = (index - 1) / 2;
    if (planner->open_list[index].f_cost < planner->open_list[parent].f_cost) {
      swap_nodes(&planner->open_list[index], &planner->open_list[parent]);
      index = parent;
    } else {
      break;
    }
  }
}

static void heapify_down(GoapPlannerState *planner, int index) {
  while (true) {
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    int smallest = index;

    if (left < planner->open_count &&
        planner->open_list[left].f_cost < planner->open_list[smallest].f_cost) {
      smallest = left;
    }
    if (right < planner->open_count &&
        planner->open_list[right].f_cost <
            planner->open_list[smallest].f_cost) {
      smallest = right;
    }

    if (smallest != index) {
      swap_nodes(&planner->open_list[index], &planner->open_list[smallest]);
      index = smallest;
    } else {
      break;
    }
  }
}

GoapPlannerState *goap_planner_create_state(int capacity) {
  GoapPlannerState *planner = (GoapPlannerState *)MALLOC_TAGGED(
      sizeof(GoapPlannerState), MEMORY_TAG_AI);
  memset(planner, 0, sizeof(GoapPlannerState));

  planner->open_capacity = capacity;
  planner->closed_capacity = capacity * 2;
  planner->open_list = (GoapPlannerNode *)MALLOC_TAGGED(
      sizeof(GoapPlannerNode) * planner->open_capacity, MEMORY_TAG_AI);
  planner->closed_list = (GoapPlannerNode *)MALLOC_TAGGED(
      sizeof(GoapPlannerNode) * planner->closed_capacity, MEMORY_TAG_AI);

  return planner;
}

void goap_planner_destroy_state(GoapPlannerState *planner) {
  if (!planner)
    return;
  FREE(planner->open_list);
  FREE(planner->closed_list);
  FREE(planner);
}

float goap_heuristic_enhanced(const GoapWorldStateEnhanced *current,
                              const GoapWorldStateEnhanced *goal) {
  // Use Hamming distance (bit differences)
  return (float)goap_state_count_differences(current, goal);
}

GoapPlan *goap_plan_enhanced(GoapPlannerState *planner,
                             const GoapActionEnhanced **available_actions,
                             int action_count,
                             const GoapWorldStateEnhanced *start_state,
                             const GoapWorldStateEnhanced *goal_state,
                             int max_plan_length) {
  if (!planner || !available_actions || action_count <= 0)
    return NULL;

  planner->open_count = 0;
  planner->closed_count = 0;
  planner->nodes_expanded = 0;
  planner->nodes_generated = 0;

  // Initial node
  GoapPlannerNode start_node;
  memset(&start_node, 0, sizeof(GoapPlannerNode));
  goap_state_copy(&start_node.state, start_state);
  start_node.parent_index = -1;
  start_node.g_cost = 0;
  start_node.h_cost = goap_heuristic_enhanced(start_state, goal_state);
  start_node.f_cost = start_node.g_cost + start_node.h_cost;
  start_node.depth = 0;

  planner->open_list[planner->open_count++] = start_node;

  GoapPlannerNode *end_node = NULL;

  while (planner->open_count > 0) {
    // Pop best node
    GoapPlannerNode current = planner->open_list[0];
    planner->open_list[0] = planner->open_list[--planner->open_count];
    heapify_down(planner, 0);

    // Add to closed list
    if (planner->closed_count >= planner->closed_capacity) {
      LOG_ERROR("GOAP: Closed list full");
      break;
    }
    int current_idx = planner->closed_count++;
    planner->closed_list[current_idx] = current;
    planner->nodes_expanded++;

    // Goal reached? (Satisfies bits of goal state)
    if (goap_state_satisfies(&current.state, goal_state)) {
      end_node = &planner->closed_list[current_idx];
      break;
    }

    if (current.depth >= max_plan_length)
      continue;

    // Expand actions
    for (int i = 0; i < action_count; i++) {
      const GoapActionEnhanced *action = available_actions[i];

      if (goap_action_can_run(action, &current.state)) {
        GoapPlannerNode next;
        memset(&next, 0, sizeof(GoapPlannerNode));
        goap_state_copy(&next.state, &current.state);
        goap_action_apply(action, &next.state);

        next.action = action;
        next.parent_index = current_idx;
        next.g_cost =
            current.g_cost + goap_action_get_cost(action, NULL, &current.state);
        next.h_cost = goap_heuristic_enhanced(&next.state, goal_state);
        next.f_cost = next.g_cost + next.h_cost;
        next.depth = current.depth + 1;

        // Check if state is in closed list with better cost (simplified check)
        bool exists = false;
        for (int j = 0; j < planner->closed_count; j++) {
          if (goap_state_equals(&next.state, &planner->closed_list[j].state)) {
            exists = true;
            break;
          }
        }
        if (exists)
          continue;

        // Add to open list
        if (planner->open_count < planner->open_capacity) {
          planner->open_list[planner->open_count++] = next;
          heapify_up(planner, planner->open_count - 1);
          planner->nodes_generated++;
        }
      }
    }
  }

  if (!end_node)
    return NULL;

  // Reconstruct plan
  GoapPlan *plan = (GoapPlan *)MALLOC_TAGGED(sizeof(GoapPlan), MEMORY_TAG_AI);
  plan->max_plan_length = max_plan_length;
  plan->actions = (const GoapActionEnhanced **)MALLOC_TAGGED(
      sizeof(GoapActionEnhanced *) * max_plan_length, MEMORY_TAG_AI);
  plan->action_count = 0;

  GoapPlannerNode *curr = end_node;
  while (curr && curr->parent_index != -1) {
    plan->actions[plan->action_count++] = curr->action;
    curr = &planner->closed_list[curr->parent_index];
  }

  // Reverse action list
  for (int i = 0; i < plan->action_count / 2; i++) {
    const GoapActionEnhanced *temp = plan->actions[i];
    plan->actions[i] = plan->actions[plan->action_count - 1 - i];
    plan->actions[plan->action_count - 1 - i] = temp;
  }

  return plan;
}

void goap_plan_destroy(GoapPlan *plan) {
  if (!plan)
    return;
  FREE((void *)plan->actions);
  FREE(plan);
}

// ===========================================================================================
// PHASE 8: PLAN EXECUTION & VALIDATION
// ===========================================================================================

GoapPlanExecutor *goap_executor_create(GoapPlan *plan) {
  GoapPlanExecutor *executor = (GoapPlanExecutor *)MALLOC_TAGGED(
      sizeof(GoapPlanExecutor), MEMORY_TAG_AI);
  memset(executor, 0, sizeof(GoapPlanExecutor));
  executor->plan = plan;
  executor->status = GOAP_EXEC_RUNNING;
  return executor;
}

void goap_executor_destroy(GoapPlanExecutor *executor) {
  if (!executor)
    return;
  if (executor->plan)
    goap_plan_destroy(executor->plan);
  FREE(executor);
}

GoapExecutionStatus goap_executor_tick(GoapPlanExecutor *executor, void *agent,
                                       GoapWorldStateEnhanced *current_state,
                                       float delta_time) {
  if (!executor || !executor->plan ||
      executor->current_step >= executor->plan->action_count) {
    return GOAP_EXEC_SUCCESS;
  }

  const GoapActionEnhanced *action =
      executor->plan->actions[executor->current_step];

  // Validate start of action
  if (!goap_action_can_run(action, current_state)) {
    executor->status = GOAP_EXEC_INVALID;
    return GOAP_EXEC_INVALID;
  }

  // Execute
  if (action->execute) {
    action->execute(agent, delta_time);
  }

  // Check completion
  bool complete = true;
  if (action->is_complete) {
    complete = action->is_complete(agent);
  }

  if (complete) {
    executor->current_step++;
    executor->steps_completed++;

    // Apply effects to external state tracker if needed
    goap_action_apply(action, current_state);

    if (executor->current_step >= executor->plan->action_count) {
      executor->status = GOAP_EXEC_SUCCESS;
      return GOAP_EXEC_SUCCESS;
    }
  }

  return GOAP_EXEC_RUNNING;
}
