#include "ai/npc_advanced/goap_enhanced.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <string.h>

/**
 * GOAP Enhanced Planner & Executor (Phase 7-8)
 * Separate file for A* planning and plan execution
 */

// ===========================================================================================
// PHASE 7: ENHANCED A* PLANNER
// ===========================================================================================

GoapPlannerState *goap_planner_create_state(int capacity) {
  GoapPlannerState *planner = (GoapPlannerState *)MALLOC_TAGGED(
      sizeof(GoapPlannerState), MEMORY_TAG_AI);
  if (!planner)
    return NULL;

  memset(planner, 0, sizeof(GoapPlannerState));
  planner->open_capacity = capacity;
  planner->closed_capacity = capacity;

  planner->open_list = (GoapPlannerNode *)MALLOC_TAGGED(
      sizeof(GoapPlannerNode) * capacity, MEMORY_TAG_AI);
  planner->closed_list = (GoapPlannerNode *)MALLOC_TAGGED(
      sizeof(GoapPlannerNode) * capacity, MEMORY_TAG_AI);

  if (!planner->open_list || !planner->closed_list) {
    goap_planner_destroy_state(planner);
    return NULL;
  }

  return planner;
}

void goap_planner_destroy_state(GoapPlannerState *planner) {
  if (!planner)
    return;

  if (planner->open_list)
    FREE(0);
  if (planner->closed_list)
    FREE(0);
  FREE(0);
}

float goap_heuristic_enhanced(const GoapWorldStateEnhanced *current,
                              const GoapWorldStateEnhanced *goal) {
  if (!current || !goal)
    return 1000.0f;

  int mismatches = goap_state_count_differences(current, goal);
  return (float)mismatches;
}

static int find_in_closed_list(const GoapPlannerState *planner,
                               const GoapWorldStateEnhanced *state) {
  for (int i = 0; i < planner->closed_count; i++) {
    if (goap_state_equals(&planner->closed_list[i].state, state)) {
      return i;
    }
  }
  return -1;
}

static int find_best_open_node(const GoapPlannerState *planner) {
  if (planner->open_count == 0)
    return -1;

  int best_idx = 0;
  float best_f = planner->open_list[0].f_cost;

  for (int i = 1; i < planner->open_count; i++) {
    if (planner->open_list[i].f_cost < best_f) {
      best_f = planner->open_list[i].f_cost;
      best_idx = i;
    }
  }

  return best_idx;
}

GoapPlan *goap_plan_enhanced(GoapPlannerState *planner,
                             const GoapActionEnhanced **available_actions,
                             int action_count,
                             const GoapWorldStateEnhanced *start_state,
                             const GoapWorldStateEnhanced *goal_state,
                             int max_plan_length) {
  if (!planner || !available_actions || !start_state || !goal_state)
    return NULL;

  // Reset planner
  planner->open_count = 0;
  planner->closed_count = 0;
  planner->nodes_expanded = 0;
  planner->nodes_generated = 0;

  // Add start node
  GoapPlannerNode start_node;
  goap_state_copy(&start_node.state, start_state);
  start_node.action = NULL;
  start_node.parent_index = -1;
  start_node.g_cost = 0.0f;
  start_node.h_cost = goap_heuristic_enhanced(start_state, goal_state);
  start_node.f_cost = start_node.g_cost + start_node.h_cost;
  start_node.depth = 0;

  planner->open_list[planner->open_count++] = start_node;
  planner->nodes_generated++;

  // A* loop
  while (planner->open_count > 0) {
    int best_idx = find_best_open_node(planner);
    GoapPlannerNode current = planner->open_list[best_idx];

    // Move to closed
    planner->open_list[best_idx] = planner->open_list[planner->open_count - 1];
    planner->open_count--;

    int current_idx = planner->closed_count;
    planner->closed_list[planner->closed_count++] = current;
    planner->nodes_expanded++;

    // Goal check
    if (goap_state_satisfies(&current.state, goal_state)) {
      // Reconstruct plan
      GoapPlan *plan =
          (GoapPlan *)MALLOC_TAGGED(sizeof(GoapPlan), MEMORY_TAG_AI);

      int plan_length = 0;
      int idx = current_idx;
      while (planner->closed_list[idx].parent_index != -1) {
        plan_length++;
        idx = planner->closed_list[idx].parent_index;
      }

      plan->actions = (const GoapActionEnhanced **)MALLOC_TAGGED(
          sizeof(GoapActionEnhanced *) * plan_length, MEMORY_TAG_AI);
      plan->action_count = plan_length;
      plan->max_plan_length = max_plan_length;

      idx = current_idx;
      int plan_idx = plan_length - 1;
      while (planner->closed_list[idx].parent_index != -1) {
        plan->actions[plan_idx--] = planner->closed_list[idx].action;
        idx = planner->closed_list[idx].parent_index;
      }

      return plan;
    }

    // Expand
    for (int i = 0; i < action_count; i++) {
      const GoapActionEnhanced *action = available_actions[i];

      if (!goap_action_can_run(action, &current.state))
        continue;

      GoapWorldStateEnhanced next_state;
      goap_state_copy(&next_state, &current.state);
      goap_action_apply(action, &next_state);

      if (find_in_closed_list(planner, &next_state) >= 0)
        continue;

      if (current.depth + 1 > max_plan_length)
        continue;

      if (planner->open_count >= planner->open_capacity)
        break;

      GoapPlannerNode neighbor;
      goap_state_copy(&neighbor.state, &next_state);
      neighbor.action = action;
      neighbor.parent_index = current_idx;
      neighbor.g_cost = current.g_cost + action->cost;
      neighbor.h_cost = goap_heuristic_enhanced(&next_state, goal_state);
      neighbor.f_cost = neighbor.g_cost + neighbor.h_cost;
      neighbor.depth = current.depth + 1;

      planner->open_list[planner->open_count++] = neighbor;
      planner->nodes_generated++;
    }
  }

  return NULL;
}

void goap_plan_destroy(GoapPlan *plan) {
  if (!plan)
    return;

  if (plan->actions)
    FREE(0);
  FREE(0);
}

void goap_plan_print(const GoapPlan *plan) {
  if (!plan)
    return;

  LOG_INFO("=== GOAP Plan (%d actions) ===", plan->action_count);
  for (int i = 0; i < plan->action_count; i++) {
    LOG_INFO("  %d. %s (cost: %.1f)", i + 1, plan->actions[i]->name,
             plan->actions[i]->cost);
  }
}

// ===========================================================================================
// PHASE 8: PLAN EXECUTION & VALIDATION
// ===========================================================================================

GoapPlanExecutor *goap_executor_create(GoapPlan *plan) {
  if (!plan)
    return NULL;

  GoapPlanExecutor *executor = (GoapPlanExecutor *)MALLOC_TAGGED(
      sizeof(GoapPlanExecutor), MEMORY_TAG_AI);
  if (!executor)
    return NULL;

  memset(executor, 0, sizeof(GoapPlanExecutor));
  executor->plan = plan;
  executor->status = GOAP_EXEC_IDLE;
  executor->validate_each_step = true;
  executor->replan_threshold = 0.3f; // 30% state drift triggers replan

  return executor;
}

void goap_executor_destroy(GoapPlanExecutor *executor) {
  if (!executor)
    return;
  FREE(executor);
}

bool goap_executor_validate_plan(const GoapPlanExecutor *executor,
                                 const GoapWorldStateEnhanced *current_state) {
  if (!executor || !executor->plan || !current_state)
    return false;

  // Check if expected state roughly matches current state
  int differences =
      goap_state_count_differences(&executor->expected_state, current_state);

  // If too many differences, plan is invalid
  return differences <=
         (int)(executor->replan_threshold * GOAP_MAX_STATE_ATOMS);
}

void goap_executor_trigger_replan(GoapPlanExecutor *executor) {
  if (!executor)
    return;
  executor->needs_replan = true;
  executor->status = GOAP_EXEC_INVALID;
  executor->replans_triggered++;
}

GoapExecutionStatus goap_executor_tick(GoapPlanExecutor *executor, void *agent,
                                       GoapWorldStateEnhanced *current_state,
                                       float delta_time) {
  if (!executor || !executor->plan || !current_state) {
    return GOAP_EXEC_FAILED;
  }

  // Validate plan
  if (executor->validate_each_step) {
    if (!goap_executor_validate_plan(executor, current_state)) {
      goap_executor_trigger_replan(executor);
      return GOAP_EXEC_INVALID;
    }
  }

  // Check if plan complete
  if (executor->current_step >= executor->plan->action_count) {
    executor->status = GOAP_EXEC_SUCCESS;
    return GOAP_EXEC_SUCCESS;
  }

  // Get current action
  const GoapActionEnhanced *action =
      executor->plan->actions[executor->current_step];

  // Execute action
  if (action->execute) {
    action->execute(agent, delta_time);
  }

  executor->execution_time += delta_time;

  // Check if complete
  bool action_complete = true;
  if (action->is_complete) {
    action_complete = action->is_complete(agent);
  }

  if (action_complete) {
    executor->current_step++;
    executor->steps_completed++;

    // Update expected state
    goap_action_apply(action, &executor->expected_state);
  }

  executor->status = GOAP_EXEC_RUNNING;
  return GOAP_EXEC_RUNNING;
}
