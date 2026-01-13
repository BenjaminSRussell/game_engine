#ifndef ANIM_STATE_MACHINE_H
#define ANIM_STATE_MACHINE_H

#include "common.h"

// Constants
#define MAX_STATES 64
#define MAX_TRANSITIONS 256
#define MAX_PARAMETERS 32
#define MAX_LAYERS 8
#define MAX_NAME_LENGTH 64
#define MAX_CONDITIONS_PER_TRANSITION 4

// Forward declarations
struct AnimStateMachine;

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
} AnimConditionType;

// Easing types for transitions
typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC
} AnimEasingType;

// Blend mode for layers
typedef enum {
    BLEND_OVERRIDE,
    BLEND_ADDITIVE,
    BLEND_MULTIPLY
} AnimLayerBlendMode;

// Animation parameter
typedef struct {
    char name[MAX_NAME_LENGTH];
    AnimParamType type;
    union {
        f32 float_val;
        i32 int_val;
        bool bool_val;
    } value;
    bool trigger_consumed;
} AnimParameter;

// Transition condition
typedef struct {
    u32 param_index;
    AnimConditionType condition;
    union {
        f32 float_val;
        i32 int_val;
    } threshold;
} AnimTransitionCondition;

// State transition
typedef struct {
    u32 from_state;
    u32 to_state;
    f32 duration;           // Blend duration in seconds
    f32 exit_time;          // Normalized time (0-1) to allow transition
    bool has_exit_time;
    AnimTransitionCondition conditions[MAX_CONDITIONS_PER_TRANSITION];
    u32 condition_count;
    bool interruption_source; // Can be interrupted
    i32 priority;
    AnimEasingType easing;
    f32 offset;             // Start offset in the destination state
} AnimStateTransition;

// Callback types
typedef void (*AnimStateCallback)(struct AnimStateMachine* sm, void* context);

// Animation state
typedef struct {
    char name[MAX_NAME_LENGTH];
    char tag[MAX_NAME_LENGTH];
    u32 animation_clip;      // Reference to animation clip ID
    f32 speed;
    bool loop;
    f32 normalized_time;     // 0-1 progress

    // Blend tree (optional)
    bool is_blend_tree;
    u32 blend_tree_id;

    // Sub-State Machine (optional)
    struct AnimStateMachine* sub_state_machine;

    // Motion
    bool apply_root_motion;
    f32 motion_speed;

    // Events/Callbacks
    AnimStateCallback on_enter;
    AnimStateCallback on_exit;
    AnimStateCallback on_update;
    void* user_data;
} AnimState;

// Animation layer
typedef struct {
    char name[MAX_NAME_LENGTH];
    u32 current_state;
    u32 previous_state;
    f32 blend_weight;
    f32 transition_time;
    f32 transition_duration;
    bool is_transitioning;
    AnimLayerBlendMode blend_mode;
    u32 avatar_mask;        // Mask ID for bone filtering
    f32 weight;             // Layer weight (0-1)
    bool additive;
    AnimStateTransition* active_transition;
} AnimLayer;

// State machine
typedef struct AnimStateMachine {
    char name[MAX_NAME_LENGTH];

    AnimState states[MAX_STATES];
    u32 state_count;

    AnimStateTransition transitions[MAX_TRANSITIONS];
    u32 transition_count;

    AnimParameter parameters[MAX_PARAMETERS];
    u32 param_count;

    AnimLayer layers[MAX_LAYERS];
    u32 layer_count;

    u32 default_state;
    bool initialized;

    void* context; // User context passed to callbacks
} AnimStateMachine;

// API Functions

// Creation/Destruction
AnimStateMachine* anim_state_machine_create(const char* name);
void anim_state_machine_destroy(AnimStateMachine* sm);

// Parameters
u32 anim_add_parameter_float(AnimStateMachine* sm, const char* name, f32 default_val);
u32 anim_add_parameter_int(AnimStateMachine* sm, const char* name, i32 default_val);
u32 anim_add_parameter_bool(AnimStateMachine* sm, const char* name, bool default_val);
u32 anim_add_parameter_trigger(AnimStateMachine* sm, const char* name);
void anim_set_float(AnimStateMachine* sm, u32 param_id, f32 value);
void anim_set_int(AnimStateMachine* sm, u32 param_id, i32 value);
void anim_set_bool(AnimStateMachine* sm, u32 param_id, bool value);
void anim_set_trigger(AnimStateMachine* sm, u32 param_id);
u32 anim_get_parameter_index(AnimStateMachine* sm, const char* name);

// States
u32 anim_add_state(AnimStateMachine* sm, const char* name, u32 clip_id);
void anim_set_state_speed(AnimStateMachine* sm, u32 state_id, f32 speed);
void anim_set_state_loop(AnimStateMachine* sm, u32 state_id, bool loop);
void anim_set_state_tag(AnimStateMachine* sm, u32 state_id, const char* tag);
void anim_set_state_callbacks(AnimStateMachine* sm, u32 state_id, AnimStateCallback on_enter, AnimStateCallback on_exit, AnimStateCallback on_update);
AnimState* anim_get_state(AnimStateMachine* sm, u32 state_id);
AnimState* anim_get_state_by_name(AnimStateMachine* sm, const char* name);

// Transitions
u32 anim_add_transition(AnimStateMachine* sm, u32 from, u32 to, f32 duration, f32 exit_time);
void anim_add_transition_condition_float(AnimStateMachine* sm, u32 trans_id, u32 param_id, AnimConditionType cond, f32 threshold);
void anim_add_transition_condition_bool(AnimStateMachine* sm, u32 trans_id, u32 param_id, bool expected);
void anim_set_transition_easing(AnimStateMachine* sm, u32 trans_id, AnimEasingType easing);
void anim_set_transition_offset(AnimStateMachine* sm, u32 trans_id, f32 offset);

// Update
void anim_state_machine_update(AnimStateMachine* sm, f32 dt);

// Queries
f32 anim_get_blend_weight(AnimStateMachine* sm, u32 layer);
u32 anim_get_current_state_id(AnimStateMachine* sm, u32 layer);
u32 anim_get_previous_state_id(AnimStateMachine* sm, u32 layer);
bool anim_is_transitioning(AnimStateMachine* sm, u32 layer);
f32 anim_get_normalized_time(AnimStateMachine* sm, u32 state_id);

#endif // ANIM_STATE_MACHINE_H
