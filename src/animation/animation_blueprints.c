// animation/animation_blueprints.c
// Animation blueprint implementation
#include "include/animation/animation_blueprints.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

AnimationBlueprint *anim_bp_create(const char *name) {
  AnimationBlueprint *bp =
      (AnimationBlueprint *)calloc(1, sizeof(AnimationBlueprint));
  strncpy(bp->name, name, sizeof(bp->name) - 1);

  bp->current_state = 0;
  bp->default_state = 0;
  bp->is_transitioning = false;

  // Initialize parameters
  bp->speed = 0.0f;
  bp->direction = 0.0f;
  bp->is_jumping = false;
  bp->is_crouching = false;

  LOG_INFO("Created animation blueprint '%s'", name);
  return bp;
}

void anim_bp_destroy(AnimationBlueprint *blueprint) {
  if (!blueprint)
    return;
  free(blueprint);
}

u32 anim_bp_add_state(AnimationBlueprint *bp, const char *name,
                      AnimationClip *clip) {
  if (!bp || bp->state_count >= ANIM_BP_MAX_STATES) {
    LOG_ERROR("Cannot add state: max states reached");
    return 0;
  }

  u32 state_id = bp->state_count++;
  AnimBPState *state = &bp->states[state_id];

  strncpy(state->name, name, sizeof(state->name) - 1);
  state->animation = clip;
  state->playback_speed = 1.0f;
  state->loops = true;

  LOG_INFO("Added animation state '%s' (ID: %u)", name, state_id);
  return state_id;
}

void anim_bp_add_transition(AnimationBlueprint *bp, u32 from_state,
                            u32 to_state, bool (*condition)(void *),
                            f32 blend_time) {
  if (!bp || bp->transition_count >= ANIM_BP_MAX_TRANSITIONS) {
    LOG_ERROR("Cannot add transition: max transitions reached");
    return;
  }

  AnimBPTransition *trans = &bp->transitions[bp->transition_count++];
  trans->from_state_id = from_state;
  trans->to_state_id = to_state;
  trans->condition = condition;
  trans->blend_time = blend_time;

  LOG_INFO("Added transition %u -> %u (blend: %.2fs)", from_state, to_state,
           blend_time);
}

void anim_bp_set_default_state(AnimationBlueprint *bp, u32 state_id) {
  if (!bp || state_id >= bp->state_count)
    return;
  bp->default_state = state_id;
  bp->current_state = state_id;
}

void anim_bp_update(AnimationBlueprint *bp, f32 delta_time, void *context) {
  if (!bp)
    return;

  bp->current_time += delta_time;

  // Check for state transitions
  if (!bp->is_transitioning) {
    for (u32 i = 0; i < bp->transition_count; i++) {
      AnimBPTransition *trans = &bp->transitions[i];

      if (trans->from_state_id == bp->current_state) {
        if (trans->condition && trans->condition(context)) {
          // Start transition
          bp->is_transitioning = true;
          bp->transition_target = trans->to_state_id;
          bp->transition_blend_alpha = 0.0f;

          // Call exit event
          AnimBPState *current = &bp->states[bp->current_state];
          if (current->on_exit) {
            current->on_exit(context);
          }

          LOG_DEBUG("Transitioning: %u -> %u", bp->current_state,
                    trans->to_state_id);
          break;
        }
      }
    }
  }

  // Update transition
  if (bp->is_transitioning) {
    bp->transition_blend_alpha += delta_time / 0.3f; // 0.3s blend time

    if (bp->transition_blend_alpha >= 1.0f) {
      bp->current_state = bp->transition_target;
      bp->is_transitioning = false;

      // Call entry event
      AnimBPState *new_state = &bp->states[bp->current_state];
      if (new_state->on_enter) {
        new_state->on_enter(context);
      }
    }
  }
}

void anim_bp_set_float(AnimationBlueprint *bp, const char *param_name,
                       f32 value) {
  if (!bp)
    return;

  if (strcmp(param_name, "Speed") == 0) {
    bp->speed = value;
  } else if (strcmp(param_name, "Direction") == 0) {
    bp->direction = value;
  }
}

void anim_bp_set_bool(AnimationBlueprint *bp, const char *param_name,
                      bool value) {
  if (!bp)
    return;

  if (strcmp(param_name, "IsJumping") == 0) {
    bp->is_jumping = value;
  } else if (strcmp(param_name, "IsCrouching") == 0) {
    bp->is_crouching = value;
  }
}
