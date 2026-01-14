#ifndef ANIM_STATE_MACHINE_H
#define ANIM_STATE_MACHINE_H

#include "core/math/types.h"
#include <stdbool.h>

// Forward declarations
struct AnimStateMachine;
struct AnimState;
struct AnimTransition;
struct AnimLayer;

// =================================================================================================
// Enums & Constants
// =================================================================================================

#define ANIM_MAX_NAME_LENGTH 64
#define ANIM_MAX_CONDITIONS 8
#define ANIM_MAX_NOTIFIES 16
#define ANIM_MAX_LAYERS 8
#define ANIM_MAX_PARAMETERS 64

// Parameter types
typedef enum {
    ANIM_PARAM_FLOAT,
    ANIM_PARAM_INT,
    ANIM_PARAM_BOOL,
    ANIM_PARAM_TRIGGER
} AnimParamType;

// Condition types for transitions
typedef enum {
    ANIM_COND_GREATER,
    ANIM_COND_LESS,
    ANIM_COND_EQUALS,
    ANIM_COND_NOT_EQUALS,
    ANIM_COND_TRUE,
    ANIM_COND_FALSE,
    ANIM_COND_TRIGGERED,
    ANIM_COND_STATE_TIME,
    ANIM_COND_TIME_REMAINING
} AnimConditionType;

// Blend modes for layers
typedef enum {
    ANIM_LAYER_BLEND_OVERRIDE,
    ANIM_LAYER_BLEND_ADDITIVE,
    ANIM_LAYER_BLEND_MULTIPLY
} AnimLayerBlendMode;

// Transition blend curves
typedef enum {
    ANIM_BLEND_LINEAR,
    ANIM_BLEND_EASE_IN,
    ANIM_BLEND_EASE_OUT,
    ANIM_BLEND_EASE_IN_OUT,
    ANIM_BLEND_HERMITE
} AnimBlendCurve;

// Transition interruption sources
typedef enum {
    ANIM_INTERRUPT_NONE,
    ANIM_INTERRUPT_SOURCE,
    ANIM_INTERRUPT_DESTINATION,
    ANIM_INTERRUPT_ANY_SOURCE,
    ANIM_INTERRUPT_ANY_DESTINATION
} AnimInterruptionSource;

// =================================================================================================
// Callbacks & Events
// =================================================================================================

typedef void (*AnimStateCallback)(struct AnimStateMachine* sm, void* context);
typedef void (*AnimNotifyCallback)(struct AnimStateMachine* sm, const char* notify_name, void* context);
typedef bool (*AnimConditionCallback)(struct AnimStateMachine* sm, void* context);

// =================================================================================================
// Structures
// =================================================================================================

// Parameter definition
typedef struct {
    char name[ANIM_MAX_NAME_LENGTH];
    AnimParamType type;
    union {
        f32 float_val;
        i32 int_val;
        bool bool_val;
    } value;
    bool trigger_consumed;
} AnimParameter;

// Transition Condition
typedef struct {
    u32 param_index;
    AnimConditionType type;
    union {
        f32 float_threshold;
        i32 int_threshold;
    } threshold;

    // Optional custom callback condition
    AnimConditionCallback custom_condition;
} AnimTransitionCondition;

// State Transition
typedef struct AnimTransition {
    u32 from_state_id;
    u32 to_state_id;

    f32 duration;           // Blend duration in seconds
    f32 offset;             // Start offset in target state
    f32 exit_time;          // 0-1 normalized time to allow transition (if has_exit_time)
    bool has_exit_time;
    bool fixed_duration;    // If true, duration is in seconds, else normalized

    AnimTransitionCondition conditions[ANIM_MAX_CONDITIONS];
    u32 condition_count;

    AnimBlendCurve blend_curve;

    AnimInterruptionSource interruption_source;
    i32 priority;

    // Events
    AnimStateCallback on_transition_start;
    AnimStateCallback on_transition_end;
    AnimStateCallback on_transition_interrupt;
} AnimTransition;

// Animation Notify (Event on timeline)
typedef struct {
    char name[ANIM_MAX_NAME_LENGTH];
    f32 time;               // Normalized time (0-1) or seconds depending on context
    bool is_triggered;      // Runtime state
    AnimNotifyCallback callback;
    void* user_data;
} AnimNotify;

// Animation State
typedef struct AnimState {
    char name[ANIM_MAX_NAME_LENGTH];
    u32 id;

    // Animation Source
    u32 animation_clip_id;  // ID of the clip to play
    u32 blend_tree_id;      // ID of blend tree (if using blend tree)
    bool is_blend_tree;

    // Sub-State Machine
    struct AnimStateMachine* sub_state_machine;
    bool has_sub_machine;

    // Playback settings
    f32 speed;
    bool loop;
    f32 normalized_time;    // Runtime: Current progress 0-1
    f32 length;             // Runtime: Length in seconds

    // Notifies
    AnimNotify notifies[ANIM_MAX_NOTIFIES];
    u32 notify_count;

    // Callbacks
    AnimStateCallback on_enter;
    AnimStateCallback on_exit;
    AnimStateCallback on_update;

    void* user_data;
} AnimState;

// Animation Layer
typedef struct AnimLayer {
    char name[ANIM_MAX_NAME_LENGTH];
    u32 id;

    u32 current_state_id;
    u32 previous_state_id;

    // Blending
    f32 weight;
    AnimLayerBlendMode blend_mode;
    u32 avatar_mask_id;     // Optional bone mask

    // Transition Runtime Data
    bool is_transitioning;
    f32 transition_time;
    f32 transition_duration;
    struct AnimTransition* active_transition;

    // State machine reference for this layer (could be separate or shared)
    // For now, simple implementation: layers share the state machine definition but track state independently?
    // Usually layers run parallel state machines.
    // We will assume the AnimStateMachine struct contains the definition, and we might need "Instance" separation.
    // But for this task, let's keep it simple: The AnimStateMachine *IS* the instance.
} AnimLayer;

// State Machine Instance
typedef struct AnimStateMachine {
    char name[ANIM_MAX_NAME_LENGTH];

    // Data Storage (fixed size for simplicity/performance, or dynamic)
    // Using dynamic arrays in implementation, pointers here?
    // Let's use pointers to dynamic data to allow growth.

    AnimState* states;
    u32 state_count;
    u32 state_capacity;

    AnimTransition* transitions;
    u32 transition_count;
    u32 transition_capacity;

    AnimParameter* parameters;
    u32 param_count;
    u32 param_capacity;

    AnimLayer* layers;
    u32 layer_count;
    u32 layer_capacity;

    // Runtime Context
    void* context_data;     // User data passed to callbacks
    bool paused;
    f32 global_speed;

    // Hierarchy
    struct AnimStateMachine* parent_machine;

} AnimStateMachine;

// =================================================================================================
// Debug & Tools
// =================================================================================================

typedef struct {
    u32 state_id;
    f32 weight;
    f32 normalized_time;
} AnimBlendState;

typedef struct {
    AnimBlendState* states;
    u32 count;
} AnimBlendData;

// =================================================================================================
// API Functions
// =================================================================================================

// Creation & Destruction
AnimStateMachine* anim_state_machine_create(const char* name);
void anim_state_machine_destroy(AnimStateMachine* sm);
void anim_sm_reset(AnimStateMachine* sm);

// Parameters
u32 anim_sm_add_parameter_float(AnimStateMachine* sm, const char* name, f32 default_value);
u32 anim_sm_add_parameter_int(AnimStateMachine* sm, const char* name, i32 default_value);
u32 anim_sm_add_parameter_bool(AnimStateMachine* sm, const char* name, bool default_value);
u32 anim_sm_add_parameter_trigger(AnimStateMachine* sm, const char* name);

void anim_sm_set_float(AnimStateMachine* sm, u32 param_id, f32 value);
void anim_sm_set_int(AnimStateMachine* sm, u32 param_id, i32 value);
void anim_sm_set_bool(AnimStateMachine* sm, u32 param_id, bool value);
void anim_sm_set_trigger(AnimStateMachine* sm, u32 param_id);
void anim_sm_reset_trigger(AnimStateMachine* sm, u32 param_id);

f32 anim_sm_get_float(AnimStateMachine* sm, u32 param_id);
i32 anim_sm_get_int(AnimStateMachine* sm, u32 param_id);
bool anim_sm_get_bool(AnimStateMachine* sm, u32 param_id);

u32 anim_sm_get_param_id(AnimStateMachine* sm, const char* name);

// States
u32 anim_sm_add_state(AnimStateMachine* sm, const char* name);
AnimState* anim_sm_get_state(AnimStateMachine* sm, u32 state_id);
void anim_sm_set_state_clip(AnimStateMachine* sm, u32 state_id, u32 clip_id);
void anim_sm_set_state_blend_tree(AnimStateMachine* sm, u32 state_id, u32 blend_tree_id);
void anim_sm_set_state_sub_machine(AnimStateMachine* sm, u32 state_id, AnimStateMachine* sub_machine);

// Callbacks
void anim_sm_set_state_callbacks(AnimStateMachine* sm, u32 state_id,
                                 AnimStateCallback on_enter,
                                 AnimStateCallback on_exit,
                                 AnimStateCallback on_update);

// Notifies
void anim_sm_add_state_notify(AnimStateMachine* sm, u32 state_id, const char* name, f32 time, AnimNotifyCallback callback);

// Transitions
u32 anim_sm_add_transition(AnimStateMachine* sm, u32 from_state, u32 to_state, f32 duration);
void anim_sm_add_condition_float(AnimStateMachine* sm, u32 transition_id, u32 param_id, AnimConditionType type, f32 threshold);
void anim_sm_add_condition_int(AnimStateMachine* sm, u32 transition_id, u32 param_id, AnimConditionType type, i32 threshold);
void anim_sm_add_condition_bool(AnimStateMachine* sm, u32 transition_id, u32 param_id, bool expected);
void anim_sm_add_condition_trigger(AnimStateMachine* sm, u32 transition_id, u32 param_id);
void anim_sm_add_condition_state_time(AnimStateMachine* sm, u32 transition_id, f32 time, AnimConditionType type);
void anim_sm_add_condition_time_remaining(AnimStateMachine* sm, u32 transition_id, f32 time, AnimConditionType type);
void anim_sm_add_condition_custom(AnimStateMachine* sm, u32 transition_id, AnimConditionCallback callback);

// Layers
u32 anim_sm_add_layer(AnimStateMachine* sm, const char* name, f32 weight);
void anim_sm_set_layer_mask(AnimStateMachine* sm, u32 layer_id, u32 mask_id);

// Runtime
void anim_sm_update(AnimStateMachine* sm, f32 dt);
void anim_sm_set_context(AnimStateMachine* sm, void* context);
u32 anim_sm_get_current_state(AnimStateMachine* sm, u32 layer_id);
f32 anim_sm_get_current_time(AnimStateMachine* sm, u32 layer_id);

// Debug
void anim_sm_get_debug_info(AnimStateMachine* sm, char* buffer, u32 buffer_size);
void anim_sm_get_blend_data(AnimStateMachine* sm, u32 layer_id, AnimBlendData* out_data);

#endif // ANIM_STATE_MACHINE_H
