#ifndef ANIM_STATE_MACHINE_H
#define ANIM_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_STATES 64
#define MAX_TRANSITIONS 256
#define MAX_PARAMETERS 32
#define MAX_LAYERS 8
#define MAX_NAME_LENGTH 64
#define MAX_CONDITIONS_PER_TRANSITION 8

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
  COND_GREATER_EQUAL,
  COND_LESS_EQUAL,
  COND_TRUE,
  COND_FALSE
} ConditionType;

// Blend mode for state
typedef enum {
  BLEND_OVERRIDE,
  BLEND_ADDITIVE,
  BLEND_MULTIPLY
} LayerBlendMode;

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
  TransitionCondition conditions[MAX_CONDITIONS_PER_TRANSITION];
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

// Creation/Destruction
AnimStateMachine *anim_state_machine_create(void);
void anim_state_machine_destroy(AnimStateMachine *sm);

// Parameter Management
uint32_t anim_add_parameter_float(AnimStateMachine *sm, const char *name, float default_val);
uint32_t anim_add_parameter_int(AnimStateMachine *sm, const char *name, int default_val);
uint32_t anim_add_parameter_bool(AnimStateMachine *sm, const char *name, bool default_val);
uint32_t anim_add_parameter_trigger(AnimStateMachine *sm, const char *name);

void anim_set_float(AnimStateMachine *sm, uint32_t param_id, float value);
void anim_set_int(AnimStateMachine *sm, uint32_t param_id, int value);
void anim_set_bool(AnimStateMachine *sm, uint32_t param_id, bool value);
void anim_set_trigger(AnimStateMachine *sm, uint32_t param_id);

// State Management
uint32_t anim_add_state(AnimStateMachine *sm, const char *name, uint32_t clip_id);
void anim_set_state_speed(AnimStateMachine *sm, uint32_t state_id, float speed);
void anim_set_state_loop(AnimStateMachine *sm, uint32_t state_id, bool loop);

// Transition Management
uint32_t anim_add_transition(AnimStateMachine *sm, uint32_t from, uint32_t to, float duration, float exit_time);
void anim_add_transition_condition_float(AnimStateMachine *sm, uint32_t trans_id, uint32_t param_id, ConditionType cond, float threshold);
void anim_add_transition_condition_int(AnimStateMachine *sm, uint32_t trans_id, uint32_t param_id, ConditionType cond, int threshold);
void anim_add_transition_condition_bool(AnimStateMachine *sm, uint32_t trans_id, uint32_t param_id, bool expected);

// Update
void anim_state_machine_update(AnimStateMachine *sm, float dt);

// Queries
float anim_get_blend_weight(AnimStateMachine *sm, uint32_t layer);
uint32_t anim_get_current_state(AnimStateMachine *sm, uint32_t layer);
uint32_t anim_get_previous_state(AnimStateMachine *sm, uint32_t layer);
bool anim_is_transitioning(AnimStateMachine *sm, uint32_t layer);
float anim_get_normalized_time(AnimStateMachine *sm, uint32_t state_id);

#endif // ANIM_STATE_MACHINE_H
