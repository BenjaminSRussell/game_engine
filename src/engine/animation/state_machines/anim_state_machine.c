/**
 * =================================================================================================
 *                          ANIMATION STATE MACHINE
 *                          Phase 3: Animation System
 * =================================================================================================
 *
 * PURPOSE: Hierarchical state machine for animation control
 * =================================================================================================
 */

#include "animation/state_machines/anim_state_machine.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

static f32 apply_easing(f32 t, AnimEasingType type) {
    switch (type) {
        case EASE_LINEAR:
            return t;
        case EASE_IN_QUAD:
            return t * t;
        case EASE_OUT_QUAD:
            return t * (2.0f - t);
        case EASE_IN_OUT_QUAD:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case EASE_IN_CUBIC:
            return t * t * t;
        case EASE_OUT_CUBIC:
            t--;
            return t * t * t + 1.0f;
        default:
            return t;
    }
}

// -----------------------------------------------------------------------------
// State Machine Creation
// -----------------------------------------------------------------------------

AnimStateMachine* anim_state_machine_create(const char* name) {
    AnimStateMachine* sm = (AnimStateMachine*)calloc(1, sizeof(AnimStateMachine));
    if (!sm) return NULL;

    if (name) {
        strncpy(sm->name, name, MAX_NAME_LENGTH - 1);
    }

    // Create default layer
    sm->layer_count = 1;
    strcpy(sm->layers[0].name, "Base Layer");
    sm->layers[0].weight = 1.0f;
    sm->layers[0].blend_mode = BLEND_OVERRIDE;

    sm->initialized = true;
    return sm;
}

void anim_state_machine_destroy(AnimStateMachine* sm) {
    if (sm) {
        // Destroy sub-state machines recursively
        for (u32 i = 0; i < sm->state_count; ++i) {
            if (sm->states[i].sub_state_machine) {
                anim_state_machine_destroy(sm->states[i].sub_state_machine);
            }
        }
        free(sm);
    }
}

// -----------------------------------------------------------------------------
// Parameter Management
// -----------------------------------------------------------------------------

u32 anim_add_parameter_float(AnimStateMachine* sm, const char* name, f32 default_val) {
    if (!sm || sm->param_count >= MAX_PARAMETERS) return UINT32_MAX;

    AnimParameter* p = &sm->parameters[sm->param_count];
    strncpy(p->name, name, MAX_NAME_LENGTH - 1);
    p->type = PARAM_FLOAT;
    p->value.float_val = default_val;

    return sm->param_count++;
}

u32 anim_add_parameter_int(AnimStateMachine* sm, const char* name, i32 default_val) {
    if (!sm || sm->param_count >= MAX_PARAMETERS) return UINT32_MAX;

    AnimParameter* p = &sm->parameters[sm->param_count];
    strncpy(p->name, name, MAX_NAME_LENGTH - 1);
    p->type = PARAM_INT;
    p->value.int_val = default_val;

    return sm->param_count++;
}

u32 anim_add_parameter_bool(AnimStateMachine* sm, const char* name, bool default_val) {
    if (!sm || sm->param_count >= MAX_PARAMETERS) return UINT32_MAX;

    AnimParameter* p = &sm->parameters[sm->param_count];
    strncpy(p->name, name, MAX_NAME_LENGTH - 1);
    p->type = PARAM_BOOL;
    p->value.bool_val = default_val;

    return sm->param_count++;
}

u32 anim_add_parameter_trigger(AnimStateMachine* sm, const char* name) {
    if (!sm || sm->param_count >= MAX_PARAMETERS) return UINT32_MAX;

    AnimParameter* p = &sm->parameters[sm->param_count];
    strncpy(p->name, name, MAX_NAME_LENGTH - 1);
    p->type = PARAM_TRIGGER;
    p->value.bool_val = false;
    p->trigger_consumed = true;

    return sm->param_count++;
}

void anim_set_float(AnimStateMachine* sm, u32 param_id, f32 value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == PARAM_FLOAT) {
        sm->parameters[param_id].value.float_val = value;
    }
}

void anim_set_int(AnimStateMachine* sm, u32 param_id, i32 value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == PARAM_INT) {
        sm->parameters[param_id].value.int_val = value;
    }
}

void anim_set_bool(AnimStateMachine* sm, u32 param_id, bool value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == PARAM_BOOL) {
        sm->parameters[param_id].value.bool_val = value;
    }
}

void anim_set_trigger(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == PARAM_TRIGGER) {
        sm->parameters[param_id].value.bool_val = true;
        sm->parameters[param_id].trigger_consumed = false;
    }
}

u32 anim_get_parameter_index(AnimStateMachine* sm, const char* name) {
    if (!sm) return UINT32_MAX;
    for (u32 i = 0; i < sm->param_count; ++i) {
        if (strcmp(sm->parameters[i].name, name) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

// -----------------------------------------------------------------------------
// State Management
// -----------------------------------------------------------------------------

u32 anim_add_state(AnimStateMachine* sm, const char* name, u32 clip_id) {
    if (!sm || sm->state_count >= MAX_STATES) return UINT32_MAX;

    AnimState* s = &sm->states[sm->state_count];
    strncpy(s->name, name, MAX_NAME_LENGTH - 1);
    s->animation_clip = clip_id;
    s->speed = 1.0f;
    s->loop = true;
    s->normalized_time = 0.0f;
    s->motion_speed = 1.0f;

    if (sm->state_count == 0) {
        sm->default_state = 0;
        sm->layers[0].current_state = 0;
    }

    return sm->state_count++;
}

void anim_set_state_speed(AnimStateMachine* sm, u32 state_id, f32 speed) {
    if (!sm || state_id >= sm->state_count) return;
    sm->states[state_id].speed = speed;
}

void anim_set_state_loop(AnimStateMachine* sm, u32 state_id, bool loop) {
    if (!sm || state_id >= sm->state_count) return;
    sm->states[state_id].loop = loop;
}

void anim_set_state_tag(AnimStateMachine* sm, u32 state_id, const char* tag) {
    if (!sm || state_id >= sm->state_count || !tag) return;
    strncpy(sm->states[state_id].tag, tag, MAX_NAME_LENGTH - 1);
}

void anim_set_state_callbacks(AnimStateMachine* sm, u32 state_id, AnimStateCallback on_enter, AnimStateCallback on_exit, AnimStateCallback on_update) {
    if (!sm || state_id >= sm->state_count) return;
    sm->states[state_id].on_enter = on_enter;
    sm->states[state_id].on_exit = on_exit;
    sm->states[state_id].on_update = on_update;
}

AnimState* anim_get_state(AnimStateMachine* sm, u32 state_id) {
    if (!sm || state_id >= sm->state_count) return NULL;
    return &sm->states[state_id];
}

AnimState* anim_get_state_by_name(AnimStateMachine* sm, const char* name) {
    if (!sm || !name) return NULL;
    for (u32 i = 0; i < sm->state_count; ++i) {
        if (strcmp(sm->states[i].name, name) == 0) {
            return &sm->states[i];
        }
    }
    return NULL;
}

// -----------------------------------------------------------------------------
// Transition Management
// -----------------------------------------------------------------------------

u32 anim_add_transition(AnimStateMachine* sm, u32 from, u32 to, f32 duration, f32 exit_time) {
    if (!sm || sm->transition_count >= MAX_TRANSITIONS) return UINT32_MAX;

    AnimStateTransition* t = &sm->transitions[sm->transition_count];
    t->from_state = from;
    t->to_state = to;
    t->duration = duration;
    t->exit_time = exit_time;
    t->has_exit_time = exit_time > 0.0f;
    t->condition_count = 0;
    t->priority = 0;
    t->easing = EASE_LINEAR;
    t->offset = 0.0f;

    return sm->transition_count++;
}

void anim_add_transition_condition_float(AnimStateMachine* sm, u32 trans_id, u32 param_id, AnimConditionType cond, f32 threshold) {
    if (!sm || trans_id >= sm->transition_count) return;

    AnimStateTransition* t = &sm->transitions[trans_id];
    if (t->condition_count >= MAX_CONDITIONS_PER_TRANSITION) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->condition = cond;
    c->threshold.float_val = threshold;
}

void anim_add_transition_condition_bool(AnimStateMachine* sm, u32 trans_id, u32 param_id, bool expected) {
    if (!sm || trans_id >= sm->transition_count) return;

    AnimStateTransition* t = &sm->transitions[trans_id];
    if (t->condition_count >= MAX_CONDITIONS_PER_TRANSITION) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->condition = expected ? COND_TRUE : COND_FALSE;
}

void anim_set_transition_easing(AnimStateMachine* sm, u32 trans_id, AnimEasingType easing) {
    if (!sm || trans_id >= sm->transition_count) return;
    sm->transitions[trans_id].easing = easing;
}

void anim_set_transition_offset(AnimStateMachine* sm, u32 trans_id, f32 offset) {
    if (!sm || trans_id >= sm->transition_count) return;
    sm->transitions[trans_id].offset = offset;
}

// -----------------------------------------------------------------------------
// Check Transition Conditions
// -----------------------------------------------------------------------------

static bool check_condition(AnimStateMachine* sm, const AnimTransitionCondition* cond) {
    AnimParameter* p = &sm->parameters[cond->param_index];

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

static bool check_transition(AnimStateMachine* sm, const AnimStateTransition* trans, f32 current_normalized_time) {
    // Check exit time
    if (trans->has_exit_time && current_normalized_time < trans->exit_time) {
        return false;
    }

    // Check all conditions
    for (u32 i = 0; i < trans->condition_count; i++) {
        if (!check_condition(sm, &trans->conditions[i])) {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// State Machine Update
// -----------------------------------------------------------------------------

void anim_state_machine_update(AnimStateMachine* sm, f32 dt) {
    if (!sm || !sm->initialized) return;

    for (u32 layer_idx = 0; layer_idx < sm->layer_count; layer_idx++) {
        AnimLayer* layer = &sm->layers[layer_idx];
        AnimState* current = &sm->states[layer->current_state];

        // Process Sub-State Machine
        if (current->sub_state_machine) {
            anim_state_machine_update(current->sub_state_machine, dt);
        }

        // State Update Callback
        if (current->on_update) {
            current->on_update(sm, sm->context);
        }

        // Update current state time
        if (!layer->is_transitioning) {
            // Assume 1.0 duration if no clip info available, or derived from speed
            f32 clip_duration = 1.0f; // Placeholder, should come from clip

            current->normalized_time += dt * current->speed / clip_duration;

            if (current->normalized_time >= 1.0f) {
                if (current->loop) {
                    current->normalized_time = fmodf(current->normalized_time, 1.0f);
                } else {
                    current->normalized_time = 1.0f;
                }
            }
        }

        // Check for valid transitions
        AnimStateTransition* best_trans = NULL;
        int best_priority = -1000;

        for (u32 i = 0; i < sm->transition_count; i++) {
            AnimStateTransition* trans = &sm->transitions[i];
            if (trans->from_state != layer->current_state) continue;

            if (check_transition(sm, trans, current->normalized_time)) {
                if (trans->priority > best_priority) {
                    best_trans = trans;
                    best_priority = trans->priority;
                }
            }
        }

        // Start transition if found
        bool just_started_transition = false;
        if (best_trans && !layer->is_transitioning) {
            // Exit Callback for old state
            if (current->on_exit) {
                current->on_exit(sm, sm->context);
            }

            layer->previous_state = layer->current_state;
            layer->current_state = best_trans->to_state;
            layer->is_transitioning = true;
            layer->transition_time = 0.0f;
            layer->transition_duration = best_trans->duration;
            layer->blend_weight = 0.0f;
            layer->active_transition = best_trans;

            // Reset new state
            AnimState* new_state = &sm->states[layer->current_state];
            new_state->normalized_time = best_trans->offset;

            // Enter Callback for new state
            if (new_state->on_enter) {
                new_state->on_enter(sm, sm->context);
            }

            // Consume triggers
            for (u32 c = 0; c < best_trans->condition_count; c++) {
                AnimParameter* p = &sm->parameters[best_trans->conditions[c].param_index];
                if (p->type == PARAM_TRIGGER) {
                    p->trigger_consumed = true;
                    p->value.bool_val = false;
                }
            }
            just_started_transition = true;
        }

        // Update transition blend
        if (layer->is_transitioning) {
            layer->transition_time += dt;

            f32 t = layer->transition_duration > 0.0f
                    ? layer->transition_time / layer->transition_duration
                    : 1.0f;

            if (t >= 1.0f) {
                t = 1.0f;
            }

            // Apply easing
            if (layer->active_transition) {
                layer->blend_weight = apply_easing(t, layer->active_transition->easing);
            } else {
                layer->blend_weight = t;
            }

            if (t >= 1.0f) {
                layer->is_transitioning = false;
                layer->active_transition = NULL;
            }

            if (!just_started_transition) {
                // Update both states during transition
                AnimState* prev = &sm->states[layer->previous_state];
                // Update previous state time (continued playback)
                prev->normalized_time += dt * prev->speed; // / duration

                // Update current state time
                current = &sm->states[layer->current_state];
                current->normalized_time += dt * current->speed; // / duration

                // Process Sub-State Machine for the entering state too
                if (current->sub_state_machine) {
                    anim_state_machine_update(current->sub_state_machine, dt);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Get Current Animation State
// -----------------------------------------------------------------------------

f32 anim_get_blend_weight(AnimStateMachine* sm, u32 layer) {
    if (!sm || layer >= sm->layer_count) return 1.0f;
    return sm->layers[layer].blend_weight;
}

u32 anim_get_current_state_id(AnimStateMachine* sm, u32 layer) {
    if (!sm || layer >= sm->layer_count) return 0;
    return sm->layers[layer].current_state;
}

u32 anim_get_previous_state_id(AnimStateMachine* sm, u32 layer) {
    if (!sm || layer >= sm->layer_count) return 0;
    return sm->layers[layer].previous_state;
}

bool anim_is_transitioning(AnimStateMachine* sm, u32 layer) {
    if (!sm || layer >= sm->layer_count) return false;
    return sm->layers[layer].is_transitioning;
}

f32 anim_get_normalized_time(AnimStateMachine* sm, u32 state_id) {
    if (!sm || state_id >= sm->state_count) return 0.0f;
    return sm->states[state_id].normalized_time;
}
