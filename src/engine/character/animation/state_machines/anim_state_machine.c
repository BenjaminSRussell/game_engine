/**
 * =================================================================================================
 *                          ANIMATION STATE MACHINE
 *                          Phase 3: Animation System
 * =================================================================================================
 *
 * PURPOSE: Hierarchical state machine for animation control
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 64
#define MAX_TRANSITIONS 256
#define MAX_PARAMETERS 32
#define MAX_LAYERS 8
#define MAX_NAME_LENGTH 64

// Parameter types
typedef enum {
  PARAM_FLOAT,
  PARAM_INT,
  PARAM_BOOL,
  PARAM_TRIGGER
} AnimParamType;

// Transition conditions
typedef enum {
  COND_GREATER,
  COND_LESS,
  COND_EQUALS,
  COND_NOT_EQUALS,
  COND_TRUE,
  COND_FALSE
} ConditionType;

// Blend mode for state
typedef enum { BLEND_OVERRIDE, BLEND_ADDITIVE, BLEND_MULTIPLY } LayerBlendMode;

// Animation parameter
typedef struct {
  char name[MAX_NAME_LENGTH];
  AnimParamType type;
  union {
    float float_val;
    int int_val;
    bool bool_val;
  } value;
  bool trigger_consumed;
} AnimParameter;

// Transition condition
typedef struct {
  uint32_t param_index;
  ConditionType condition;
  union {
    float float_val;
    int int_val;
  } threshold;
} TransitionCondition;

// State transition
typedef struct {
  uint32_t from_state;
  uint32_t to_state;
  float duration;  // Blend duration
  float exit_time; // When to allow transition (0-1 of anim)
  bool has_exit_time;
  TransitionCondition conditions[4];
  uint32_t condition_count;
  bool interruption_source; // Can be interrupted
  int priority;
} StateTransition;

// Animation state
typedef struct {
  char name[MAX_NAME_LENGTH];
  uint32_t animation_clip; // Reference to animation clip
  float speed;
  bool loop;
  float normalized_time; // 0-1 progress

  // Blend tree (optional)
  bool is_blend_tree;
  uint32_t blend_tree_id;

  // Motion
  bool apply_root_motion;
  float motion_speed;
} AnimState;

// Animation layer
typedef struct {
  char name[MAX_NAME_LENGTH];
  uint32_t current_state;
  uint32_t previous_state;
  float blend_weight;
  float transition_time;
  float transition_duration;
  bool is_transitioning;
  LayerBlendMode blend_mode;
  uint32_t avatar_mask; // Which bones this layer affects
  float weight;         // Layer weight
  bool additive;
} AnimLayer;

// State machine
typedef struct {
  AnimState states[MAX_STATES];
  uint32_t state_count;

  StateTransition transitions[MAX_TRANSITIONS];
  uint32_t transition_count;

  AnimParameter parameters[MAX_PARAMETERS];
  uint32_t param_count;

  AnimLayer layers[MAX_LAYERS];
  uint32_t layer_count;

  uint32_t default_state;
  bool initialized;
} AnimStateMachine;

// -----------------------------------------------------------------------------
// State Machine Creation
// -----------------------------------------------------------------------------

AnimStateMachine *anim_state_machine_create(void) {
  AnimStateMachine *sm =
      (AnimStateMachine *)calloc(1, sizeof(AnimStateMachine));
  if (!sm)
    return NULL;

  // Create default layer
  sm->layer_count = 1;
  strcpy(sm->layers[0].name, "Base Layer");
  sm->layers[0].weight = 1.0f;
  sm->layers[0].blend_mode = BLEND_OVERRIDE;

  sm->initialized = true;
  return sm;
}

void anim_state_machine_destroy(AnimStateMachine *sm) {
  if (sm)
    free(sm);
}

// -----------------------------------------------------------------------------
// Parameter Management
// -----------------------------------------------------------------------------

uint32_t anim_add_parameter_float(AnimStateMachine *sm, const char *name,
                                  float default_val) {
  if (!sm || sm->param_count >= MAX_PARAMETERS)
    return UINT32_MAX;

  AnimParameter *p = &sm->parameters[sm->param_count];
  strncpy(p->name, name, MAX_NAME_LENGTH - 1);
  p->type = PARAM_FLOAT;
  p->value.float_val = default_val;

  return sm->param_count++;
}

uint32_t anim_add_parameter_int(AnimStateMachine *sm, const char *name,
                                int default_val) {
  if (!sm || sm->param_count >= MAX_PARAMETERS)
    return UINT32_MAX;

  AnimParameter *p = &sm->parameters[sm->param_count];
  strncpy(p->name, name, MAX_NAME_LENGTH - 1);
  p->type = PARAM_INT;
  p->value.int_val = default_val;

  return sm->param_count++;
}

uint32_t anim_add_parameter_bool(AnimStateMachine *sm, const char *name,
                                 bool default_val) {
  if (!sm || sm->param_count >= MAX_PARAMETERS)
    return UINT32_MAX;

  AnimParameter *p = &sm->parameters[sm->param_count];
  strncpy(p->name, name, MAX_NAME_LENGTH - 1);
  p->type = PARAM_BOOL;
  p->value.bool_val = default_val;

  return sm->param_count++;
}

uint32_t anim_add_parameter_trigger(AnimStateMachine *sm, const char *name) {
  if (!sm || sm->param_count >= MAX_PARAMETERS)
    return UINT32_MAX;

  AnimParameter *p = &sm->parameters[sm->param_count];
  strncpy(p->name, name, MAX_NAME_LENGTH - 1);
  p->type = PARAM_TRIGGER;
  p->value.bool_val = false;
  p->trigger_consumed = true;

  return sm->param_count++;
}

void anim_set_float(AnimStateMachine *sm, uint32_t param_id, float value) {
  if (!sm || param_id >= sm->param_count)
    return;
  if (sm->parameters[param_id].type == PARAM_FLOAT) {
    sm->parameters[param_id].value.float_val = value;
  }
}

void anim_set_int(AnimStateMachine *sm, uint32_t param_id, int value) {
  if (!sm || param_id >= sm->param_count)
    return;
  if (sm->parameters[param_id].type == PARAM_INT) {
    sm->parameters[param_id].value.int_val = value;
  }
}

void anim_set_bool(AnimStateMachine *sm, uint32_t param_id, bool value) {
  if (!sm || param_id >= sm->param_count)
    return;
  if (sm->parameters[param_id].type == PARAM_BOOL) {
    sm->parameters[param_id].value.bool_val = value;
  }
}

void anim_set_trigger(AnimStateMachine *sm, uint32_t param_id) {
  if (!sm || param_id >= sm->param_count)
    return;
  if (sm->parameters[param_id].type == PARAM_TRIGGER) {
    sm->parameters[param_id].value.bool_val = true;
    sm->parameters[param_id].trigger_consumed = false;
  }
}

// -----------------------------------------------------------------------------
// State Management
// -----------------------------------------------------------------------------

uint32_t anim_add_state(AnimStateMachine *sm, const char *name,
                        uint32_t clip_id) {
  if (!sm || sm->state_count >= MAX_STATES)
    return UINT32_MAX;

  AnimState *s = &sm->states[sm->state_count];
  strncpy(s->name, name, MAX_NAME_LENGTH - 1);
  s->animation_clip = clip_id;
  s->speed = 1.0f;
  s->loop = true;
  s->normalized_time = 0.0f;

  if (sm->state_count == 0) {
    sm->default_state = 0;
    sm->layers[0].current_state = 0;
  }

  return sm->state_count++;
}

void anim_set_state_speed(AnimStateMachine *sm, uint32_t state_id,
                          float speed) {
  if (!sm || state_id >= sm->state_count)
    return;
  sm->states[state_id].speed = speed;
}

void anim_set_state_loop(AnimStateMachine *sm, uint32_t state_id, bool loop) {
  if (!sm || state_id >= sm->state_count)
    return;
  sm->states[state_id].loop = loop;
}

// -----------------------------------------------------------------------------
// Transition Management
// -----------------------------------------------------------------------------

uint32_t anim_add_transition(AnimStateMachine *sm, uint32_t from, uint32_t to,
                             float duration, float exit_time) {
  if (!sm || sm->transition_count >= MAX_TRANSITIONS)
    return UINT32_MAX;

  StateTransition *t = &sm->transitions[sm->transition_count];
  t->from_state = from;
  t->to_state = to;
  t->duration = duration;
  t->exit_time = exit_time;
  t->has_exit_time = exit_time > 0.0f;
  t->condition_count = 0;
  t->priority = 0;

  return sm->transition_count++;
}

void anim_add_transition_condition_float(AnimStateMachine *sm,
                                         uint32_t trans_id, uint32_t param_id,
                                         ConditionType cond, float threshold) {
  if (!sm || trans_id >= sm->transition_count)
    return;

  StateTransition *t = &sm->transitions[trans_id];
  if (t->condition_count >= 4)
    return;

  TransitionCondition *c = &t->conditions[t->condition_count++];
  c->param_index = param_id;
  c->condition = cond;
  c->threshold.float_val = threshold;
}

void anim_add_transition_condition_bool(AnimStateMachine *sm, uint32_t trans_id,
                                        uint32_t param_id, bool expected) {
  if (!sm || trans_id >= sm->transition_count)
    return;

  StateTransition *t = &sm->transitions[trans_id];
  if (t->condition_count >= 4)
    return;

  TransitionCondition *c = &t->conditions[t->condition_count++];
  c->param_index = param_id;
  c->condition = expected ? COND_TRUE : COND_FALSE;
}

// -----------------------------------------------------------------------------
// Check Transition Conditions
// -----------------------------------------------------------------------------

static bool check_condition(AnimStateMachine *sm,
                            const TransitionCondition *cond) {
  AnimParameter *p = &sm->parameters[cond->param_index];

  switch (p->type) {
  case PARAM_FLOAT:
    switch (cond->condition) {
    case COND_GREATER:
      return p->value.float_val > cond->threshold.float_val;
    case COND_LESS:
      return p->value.float_val < cond->threshold.float_val;
    case COND_EQUALS:
      return fabsf(p->value.float_val - cond->threshold.float_val) < 0.0001f;
    default:
      return false;
    }
  case PARAM_INT:
    switch (cond->condition) {
    case COND_GREATER:
      return p->value.int_val > cond->threshold.int_val;
    case COND_LESS:
      return p->value.int_val < cond->threshold.int_val;
    case COND_EQUALS:
      return p->value.int_val == cond->threshold.int_val;
    case COND_NOT_EQUALS:
      return p->value.int_val != cond->threshold.int_val;
    default:
      return false;
    }
  case PARAM_BOOL:
    return (cond->condition == COND_TRUE) ? p->value.bool_val
                                          : !p->value.bool_val;
  case PARAM_TRIGGER:
    if (cond->condition == COND_TRUE && p->value.bool_val &&
        !p->trigger_consumed) {
      return true;
    }
    return false;
  default:
    return false;
  }
}

static bool check_transition(AnimStateMachine *sm, const StateTransition *trans,
                             float current_normalized_time) {
  // Check exit time
  if (trans->has_exit_time && current_normalized_time < trans->exit_time) {
    return false;
  }

  // Check all conditions
  for (uint32_t i = 0; i < trans->condition_count; i++) {
    if (!check_condition(sm, &trans->conditions[i])) {
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
// State Machine Update
// -----------------------------------------------------------------------------

void anim_state_machine_update(AnimStateMachine *sm, float dt) {
  if (!sm || !sm->initialized)
    return;

  for (uint32_t layer_idx = 0; layer_idx < sm->layer_count; layer_idx++) {
    AnimLayer *layer = &sm->layers[layer_idx];
    AnimState *current = &sm->states[layer->current_state];

    // Update current state time
    if (!layer->is_transitioning) {
      current->normalized_time +=
          dt * current->speed / 1.0f; // Assuming 1 second clips
      if (current->normalized_time >= 1.0f) {
        if (current->loop) {
          current->normalized_time = fmodf(current->normalized_time, 1.0f);
        } else {
          current->normalized_time = 1.0f;
        }
      }
    }

    // Check for valid transitions
    StateTransition *best_trans = NULL;
    int best_priority = -1000;

    for (uint32_t i = 0; i < sm->transition_count; i++) {
      StateTransition *trans = &sm->transitions[i];
      if (trans->from_state != layer->current_state)
        continue;

      if (check_transition(sm, trans, current->normalized_time)) {
        if (trans->priority > best_priority) {
          best_trans = trans;
          best_priority = trans->priority;
        }
      }
    }

    // Start transition if found
    if (best_trans && !layer->is_transitioning) {
      layer->previous_state = layer->current_state;
      layer->current_state = best_trans->to_state;
      layer->is_transitioning = true;
      layer->transition_time = 0.0f;
      layer->transition_duration = best_trans->duration;
      layer->blend_weight = 0.0f;

      // Reset new state
      sm->states[layer->current_state].normalized_time = 0.0f;

      // Consume triggers
      for (uint32_t c = 0; c < best_trans->condition_count; c++) {
        AnimParameter *p =
            &sm->parameters[best_trans->conditions[c].param_index];
        if (p->type == PARAM_TRIGGER) {
          p->trigger_consumed = true;
          p->value.bool_val = false;
        }
      }
    }

    // Update transition blend
    if (layer->is_transitioning) {
      layer->transition_time += dt;
      layer->blend_weight =
          layer->transition_duration > 0.0f
              ? layer->transition_time / layer->transition_duration
              : 1.0f;

      if (layer->blend_weight >= 1.0f) {
        layer->blend_weight = 1.0f;
        layer->is_transitioning = false;
      }

      // Update both states during transition
      AnimState *prev = &sm->states[layer->previous_state];
      prev->normalized_time += dt * prev->speed / 1.0f;
      current->normalized_time += dt * current->speed / 1.0f;
    }
  }
}

// -----------------------------------------------------------------------------
// Get Current Animation State
// -----------------------------------------------------------------------------

float anim_get_blend_weight(AnimStateMachine *sm, uint32_t layer) {
  if (!sm || layer >= sm->layer_count)
    return 1.0f;
  return sm->layers[layer].blend_weight;
}

uint32_t anim_get_current_state(AnimStateMachine *sm, uint32_t layer) {
  if (!sm || layer >= sm->layer_count)
    return 0;
  return sm->layers[layer].current_state;
}

uint32_t anim_get_previous_state(AnimStateMachine *sm, uint32_t layer) {
  if (!sm || layer >= sm->layer_count)
    return 0;
  return sm->layers[layer].previous_state;
}

bool anim_is_transitioning(AnimStateMachine *sm, uint32_t layer) {
  if (!sm || layer >= sm->layer_count)
    return false;
  return sm->layers[layer].is_transitioning;
}

float anim_get_normalized_time(AnimStateMachine *sm, uint32_t state_id) {
  if (!sm || state_id >= sm->state_count)
    return 0.0f;
  return sm->states[state_id].normalized_time;
}
