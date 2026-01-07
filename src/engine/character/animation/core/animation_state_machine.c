/**
 * @file animation_state_machine.c
 * @brief Graph-based Animation State Machine (ASM).
 *
 * Manages transitions between animation states (Idle -> Walk -> Run).
 * Supports conditions, blend times, and event triggers.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <character/animation/animation_state_machine.h>
#include <string.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct AnimCondition {
  char parameter_name[32];
  enum { COND_GREATER, COND_LESS, COND_EQUAL, COND_TRUE } type;
  float threshold;
} AnimCondition;

typedef struct AnimTransition {
  uint32_t target_state_id;
  float duration; // Blending time
  AnimCondition conditions[4];
  int condition_count;
} AnimTransition;

typedef struct AnimState {
  uint32_t id;
  char name[32];
  uint32_t clip_id; // or BlendSpace ID
  bool is_looping;
  AnimTransition transitions[8];
  int transition_count;
} AnimState;

typedef struct AnimGraph {
  AnimState states[64];
  uint32_t current_state_idx;
  uint32_t entry_state_idx;

  // Blending state
  uint32_t prev_state_idx;
  float blend_weight; // 0.0 (Prev) -> 1.0 (Current)
  float blend_duration;
  float blend_timer;
} AnimGraph;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Evaluates transitions for the current state.
 */
void asm_update(AnimGraph *graph, Blackboard *blackboard, float dt) {
  AnimState *current = &graph->states[graph->current_state_idx];

  // Check transitions
  for (int i = 0; i < current->transition_count; i++) {
    AnimTransition *trans = &current->transitions[i];
    bool all_met = true;

    for (int c = 0; c < trans->condition_count; c++) {
      AnimCondition *cond = &trans->conditions[c];
      float val = blackboard_get_float(blackboard, cond->parameter_name);

      if (cond->type == COND_GREATER && val <= cond->threshold)
        all_met = false;
      if (cond->type == COND_LESS && val >= cond->threshold)
        all_met = false;
      if (cond->type == COND_EQUAL && fabsf(val - cond->threshold) > 0.001f)
        all_met = false;
    }

    if (all_met) {
      // Trigger transition
      graph->prev_state_idx = graph->current_state_idx;
      graph->current_state_idx = trans->target_state_id;
      graph->blend_duration = trans->duration;
      graph->blend_timer = 0.0f;
      graph->blend_weight = 0.0f;
      break;
    }
  }

  // Update blending
  if (graph->blend_timer < graph->blend_duration) {
    graph->blend_timer += dt;
    graph->blend_weight = graph->blend_timer / graph->blend_duration;
    if (graph->blend_weight > 1.0f)
      graph->blend_weight = 1.0f;
  } else {
    graph->blend_weight = 1.0f; // Fully in new state
  }
}

/**
 * @brief Samples the graph to get final pose.
 */
void asm_evaluate_pose(AnimGraph *graph, float dt, Pose *out_pose) {
  AnimState *current = &graph->states[graph->current_state_idx];

  Pose current_pose;
  // ... Sample clip(current->clip_id) into current_pose ...

  if (graph->blend_weight < 1.0f) {
    AnimState *prev = &graph->states[graph->prev_state_idx];
    Pose prev_pose;
    // ... Sample clip(prev->clip_id) into prev_pose ...

    // Blend
    pose_blend(&prev_pose, &current_pose, graph->blend_weight, out_pose);
  } else {
    *out_pose = current_pose;
  }
}
