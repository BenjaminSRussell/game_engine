/**
 * =================================================================================================
 *                          ANIMATION STATE MACHINE
 *                          Phase 3: Animation System
 * =================================================================================================
 *
 * PURPOSE: Hierarchical state machine for animation control with event support
 * =================================================================================================
 */

#include "include/animation/state_machines/anim_state_machine.h"
// #include "include/core/memory.h" // DISABLED: Header has conflicting definitions with unified_memory_allocator.h
#include "include/core/logger.h"
#include <math.h>
#include <string.h>
#include <stdlib.h> // For qsort if needed

#define ANIM_INVALID_ID ((u32)-1)

// =================================================================================================
// Internal Helpers
// =================================================================================================

static void* anim_alloc(size_t size) {
#ifdef MALLOC_GAMEPLAY
    return MALLOC_GAMEPLAY(size);
#else
    return calloc(1, size);
#endif
}

static void* anim_realloc(void* ptr, size_t size) {
#ifdef REALLOC
    return REALLOC(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

static void anim_free(void* ptr) {
#ifdef FREE
    FREE(ptr);
#else
    free(ptr);
#endif
}

static void ensure_state_capacity(AnimStateMachine* sm) {
    if (sm->state_count >= sm->state_capacity) {
        sm->state_capacity = (sm->state_capacity == 0) ? 16 : sm->state_capacity * 2;
        sm->states = (AnimState*)anim_realloc(sm->states, sm->state_capacity * sizeof(AnimState));
        // Initialize new memory to 0
        memset(sm->states + sm->state_count, 0, (sm->state_capacity - sm->state_count) * sizeof(AnimState));
    }
}

static void ensure_transition_capacity(AnimStateMachine* sm) {
    if (sm->transition_count >= sm->transition_capacity) {
        sm->transition_capacity = (sm->transition_capacity == 0) ? 32 : sm->transition_capacity * 2;
        sm->transitions = (AnimTransition*)anim_realloc(sm->transitions, sm->transition_capacity * sizeof(AnimTransition));
        memset(sm->transitions + sm->transition_count, 0, (sm->transition_capacity - sm->transition_count) * sizeof(AnimTransition));
    }
}

static void ensure_param_capacity(AnimStateMachine* sm) {
    if (sm->param_count >= sm->param_capacity) {
        sm->param_capacity = (sm->param_capacity == 0) ? 16 : sm->param_capacity * 2;
        sm->parameters = (AnimParameter*)anim_realloc(sm->parameters, sm->param_capacity * sizeof(AnimParameter));
        memset(sm->parameters + sm->param_count, 0, (sm->param_capacity - sm->param_count) * sizeof(AnimParameter));
    }
}

static void ensure_layer_capacity(AnimStateMachine* sm) {
    if (sm->layer_count >= sm->layer_capacity) {
        sm->layer_capacity = (sm->layer_capacity == 0) ? 4 : sm->layer_capacity * 2;
        sm->layers = (AnimLayer*)anim_realloc(sm->layers, sm->layer_capacity * sizeof(AnimLayer));
        memset(sm->layers + sm->layer_count, 0, (sm->layer_capacity - sm->layer_count) * sizeof(AnimLayer));
    }
}

// =================================================================================================
// Creation & Destruction
// =================================================================================================

AnimStateMachine* anim_state_machine_create(const char* name) {
    AnimStateMachine* sm = (AnimStateMachine*)anim_alloc(sizeof(AnimStateMachine));
    if (!sm) return NULL;

    if (name) {
        strncpy(sm->name, name, ANIM_MAX_NAME_LENGTH - 1);
    }

    sm->global_speed = 1.0f;

    // Create default layer
    anim_sm_add_layer(sm, "Base Layer", 1.0f);

    return sm;
}

void anim_state_machine_destroy(AnimStateMachine* sm) {
    if (!sm) return;

    if (sm->states) anim_free(sm->states);
    if (sm->transitions) anim_free(sm->transitions);
    if (sm->parameters) anim_free(sm->parameters);
    if (sm->layers) anim_free(sm->layers);

    anim_free(sm);
}

// =================================================================================================
// Parameter Management
// =================================================================================================

u32 anim_sm_add_parameter_float(AnimStateMachine* sm, const char* name, f32 default_value) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_param_capacity(sm);

    u32 id = sm->param_count++;
    AnimParameter* p = &sm->parameters[id];
    strncpy(p->name, name, ANIM_MAX_NAME_LENGTH - 1);
    p->type = ANIM_PARAM_FLOAT;
    p->value.float_val = default_value;

    return id;
}

u32 anim_sm_add_parameter_int(AnimStateMachine* sm, const char* name, i32 default_value) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_param_capacity(sm);

    u32 id = sm->param_count++;
    AnimParameter* p = &sm->parameters[id];
    strncpy(p->name, name, ANIM_MAX_NAME_LENGTH - 1);
    p->type = ANIM_PARAM_INT;
    p->value.int_val = default_value;

    return id;
}

u32 anim_sm_add_parameter_bool(AnimStateMachine* sm, const char* name, bool default_value) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_param_capacity(sm);

    u32 id = sm->param_count++;
    AnimParameter* p = &sm->parameters[id];
    strncpy(p->name, name, ANIM_MAX_NAME_LENGTH - 1);
    p->type = ANIM_PARAM_BOOL;
    p->value.bool_val = default_value;

    return id;
}

u32 anim_sm_add_parameter_trigger(AnimStateMachine* sm, const char* name) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_param_capacity(sm);

    u32 id = sm->param_count++;
    AnimParameter* p = &sm->parameters[id];
    strncpy(p->name, name, ANIM_MAX_NAME_LENGTH - 1);
    p->type = ANIM_PARAM_TRIGGER;
    p->value.bool_val = false;
    p->trigger_consumed = true; // Consumed by default (inactive)

    return id;
}

void anim_sm_set_float(AnimStateMachine* sm, u32 param_id, f32 value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == ANIM_PARAM_FLOAT) {
        sm->parameters[param_id].value.float_val = value;
    }
}

void anim_sm_set_int(AnimStateMachine* sm, u32 param_id, i32 value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == ANIM_PARAM_INT) {
        sm->parameters[param_id].value.int_val = value;
    }
}

void anim_sm_set_bool(AnimStateMachine* sm, u32 param_id, bool value) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == ANIM_PARAM_BOOL) {
        sm->parameters[param_id].value.bool_val = value;
    }
}

void anim_sm_set_trigger(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == ANIM_PARAM_TRIGGER) {
        sm->parameters[param_id].value.bool_val = true;
        sm->parameters[param_id].trigger_consumed = false;
    }
}

void anim_sm_reset_trigger(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return;
    if (sm->parameters[param_id].type == ANIM_PARAM_TRIGGER) {
        sm->parameters[param_id].value.bool_val = false;
        sm->parameters[param_id].trigger_consumed = true;
    }
}

f32 anim_sm_get_float(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return 0.0f;
    return (sm->parameters[param_id].type == ANIM_PARAM_FLOAT) ? sm->parameters[param_id].value.float_val : 0.0f;
}

i32 anim_sm_get_int(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return 0;
    return (sm->parameters[param_id].type == ANIM_PARAM_INT) ? sm->parameters[param_id].value.int_val : 0;
}

bool anim_sm_get_bool(AnimStateMachine* sm, u32 param_id) {
    if (!sm || param_id >= sm->param_count) return false;
    return (sm->parameters[param_id].type == ANIM_PARAM_BOOL || sm->parameters[param_id].type == ANIM_PARAM_TRIGGER)
           ? sm->parameters[param_id].value.bool_val : false;
}

u32 anim_sm_get_param_id(AnimStateMachine* sm, const char* name) {
    if (!sm) return ANIM_INVALID_ID;
    for (u32 i = 0; i < sm->param_count; i++) {
        if (strncmp(sm->parameters[i].name, name, ANIM_MAX_NAME_LENGTH) == 0) {
            return i;
        }
    }
    return ANIM_INVALID_ID;
}

// =================================================================================================
// State Management
// =================================================================================================

u32 anim_sm_add_state(AnimStateMachine* sm, const char* name) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_state_capacity(sm);

    u32 id = sm->state_count++;
    AnimState* s = &sm->states[id];
    s->id = id;
    if (name) strncpy(s->name, name, ANIM_MAX_NAME_LENGTH - 1);
    s->speed = 1.0f;
    s->loop = true;
    s->length = 1.0f; // Default length (would be updated from animation clip in full system)

    return id;
}

AnimState* anim_sm_get_state(AnimStateMachine* sm, u32 state_id) {
    if (!sm || state_id >= sm->state_count) return NULL;
    return &sm->states[state_id];
}

void anim_sm_set_state_clip(AnimStateMachine* sm, u32 state_id, u32 clip_id) {
    AnimState* s = anim_sm_get_state(sm, state_id);
    if (s) {
        s->animation_clip_id = clip_id;
        s->is_blend_tree = false;
        // In a real system, we would look up clip duration here
    }
}

void anim_sm_set_state_blend_tree(AnimStateMachine* sm, u32 state_id, u32 blend_tree_id) {
    AnimState* s = anim_sm_get_state(sm, state_id);
    if (s) {
        s->blend_tree_id = blend_tree_id;
        s->is_blend_tree = true;
    }
}

void anim_sm_set_state_sub_machine(AnimStateMachine* sm, u32 state_id, AnimStateMachine* sub_machine) {
    AnimState* s = anim_sm_get_state(sm, state_id);
    if (s) {
        s->sub_state_machine = sub_machine;
        s->has_sub_machine = (sub_machine != NULL);
        if (sub_machine) {
            sub_machine->parent_machine = sm;
        }
    }
}

void anim_sm_set_state_callbacks(AnimStateMachine* sm, u32 state_id,
                                 AnimStateCallback on_enter,
                                 AnimStateCallback on_exit,
                                 AnimStateCallback on_update) {
    AnimState* s = anim_sm_get_state(sm, state_id);
    if (s) {
        s->on_enter = on_enter;
        s->on_exit = on_exit;
        s->on_update = on_update;
    }
}

void anim_sm_add_state_notify(AnimStateMachine* sm, u32 state_id, const char* name, f32 time, AnimNotifyCallback callback) {
    AnimState* s = anim_sm_get_state(sm, state_id);
    if (s && s->notify_count < ANIM_MAX_NOTIFIES) {
        AnimNotify* n = &s->notifies[s->notify_count++];
        if (name) strncpy(n->name, name, ANIM_MAX_NAME_LENGTH - 1);
        n->time = time;
        n->callback = callback;
        n->is_triggered = false;
    }
}

// =================================================================================================
// Transition Management
// =================================================================================================

u32 anim_sm_add_transition(AnimStateMachine* sm, u32 from_state, u32 to_state, f32 duration) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_transition_capacity(sm);

    u32 id = sm->transition_count++;
    AnimTransition* t = &sm->transitions[id];
    t->from_state_id = from_state;
    t->to_state_id = to_state;
    t->duration = duration;
    t->has_exit_time = false;
    t->exit_time = 0.9f;
    t->interruption_source = ANIM_INTERRUPT_NONE;

    return id;
}

void anim_sm_add_condition_float(AnimStateMachine* sm, u32 transition_id, u32 param_id, AnimConditionType type, f32 threshold) {
    if (!sm || transition_id >= sm->transition_count) return;
    AnimTransition* t = &sm->transitions[transition_id];
    if (t->condition_count >= ANIM_MAX_CONDITIONS) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->type = type;
    c->threshold.float_threshold = threshold;
    c->custom_condition = NULL;
}

void anim_sm_add_condition_int(AnimStateMachine* sm, u32 transition_id, u32 param_id, AnimConditionType type, i32 threshold) {
    if (!sm || transition_id >= sm->transition_count) return;
    AnimTransition* t = &sm->transitions[transition_id];
    if (t->condition_count >= ANIM_MAX_CONDITIONS) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->type = type;
    c->threshold.int_threshold = threshold;
    c->custom_condition = NULL;
}

void anim_sm_add_condition_bool(AnimStateMachine* sm, u32 transition_id, u32 param_id, bool expected) {
    if (!sm || transition_id >= sm->transition_count) return;
    AnimTransition* t = &sm->transitions[transition_id];
    if (t->condition_count >= ANIM_MAX_CONDITIONS) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->type = expected ? ANIM_COND_TRUE : ANIM_COND_FALSE;
    c->custom_condition = NULL;
}

void anim_sm_add_condition_trigger(AnimStateMachine* sm, u32 transition_id, u32 param_id) {
    if (!sm || transition_id >= sm->transition_count) return;
    AnimTransition* t = &sm->transitions[transition_id];
    if (t->condition_count >= ANIM_MAX_CONDITIONS) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->param_index = param_id;
    c->type = ANIM_COND_TRIGGERED;
    c->custom_condition = NULL;
}

void anim_sm_add_condition_custom(AnimStateMachine* sm, u32 transition_id, AnimConditionCallback callback) {
    if (!sm || transition_id >= sm->transition_count) return;
    AnimTransition* t = &sm->transitions[transition_id];
    if (t->condition_count >= ANIM_MAX_CONDITIONS) return;

    AnimTransitionCondition* c = &t->conditions[t->condition_count++];
    c->custom_condition = callback;
}

// =================================================================================================
// Layers
// =================================================================================================

u32 anim_sm_add_layer(AnimStateMachine* sm, const char* name, f32 weight) {
    if (!sm) return ANIM_INVALID_ID;
    ensure_layer_capacity(sm);

    u32 id = sm->layer_count++;
    AnimLayer* l = &sm->layers[id];
    l->id = id;
    if (name) strncpy(l->name, name, ANIM_MAX_NAME_LENGTH - 1);
    l->weight = weight;
    l->blend_mode = ANIM_LAYER_BLEND_OVERRIDE;
    l->current_state_id = 0; // Default state is usually 0
    l->previous_state_id = 0;
    l->is_transitioning = false;

    return id;
}

void anim_sm_set_layer_mask(AnimStateMachine* sm, u32 layer_id, u32 mask_id) {
    if (!sm || layer_id >= sm->layer_count) return;
    sm->layers[layer_id].avatar_mask_id = mask_id;
}

// =================================================================================================
// Runtime Logic
// =================================================================================================

static bool check_condition(AnimStateMachine* sm, const AnimTransitionCondition* cond) {
    if (cond->custom_condition) {
        return cond->custom_condition(sm, sm->context_data);
    }

    if (cond->param_index >= sm->param_count) return false;
    AnimParameter* p = &sm->parameters[cond->param_index];

    switch (p->type) {
        case ANIM_PARAM_FLOAT: {
            f32 val = p->value.float_val;
            f32 thresh = cond->threshold.float_threshold;
            switch (cond->type) {
                case ANIM_COND_GREATER: return val > thresh;
                case ANIM_COND_LESS: return val < thresh;
                case ANIM_COND_EQUALS: return fabsf(val - thresh) < 0.0001f;
                case ANIM_COND_NOT_EQUALS: return fabsf(val - thresh) > 0.0001f;
                default: return false;
            }
        }
        case ANIM_PARAM_INT: {
            i32 val = p->value.int_val;
            i32 thresh = cond->threshold.int_threshold;
            switch (cond->type) {
                case ANIM_COND_GREATER: return val > thresh;
                case ANIM_COND_LESS: return val < thresh;
                case ANIM_COND_EQUALS: return val == thresh;
                case ANIM_COND_NOT_EQUALS: return val != thresh;
                default: return false;
            }
        }
        case ANIM_PARAM_BOOL:
            return (cond->type == ANIM_COND_TRUE) ? p->value.bool_val : !p->value.bool_val;

        case ANIM_PARAM_TRIGGER:
            if (cond->type == ANIM_COND_TRIGGERED && p->value.bool_val && !p->trigger_consumed) {
                return true;
            }
            return false;

        default: return false;
    }
}

static bool check_transition(AnimStateMachine* sm, const AnimTransition* trans, const AnimState* current_state) {
    // Check exit time
    if (trans->has_exit_time) {
        if (current_state->normalized_time < trans->exit_time) {
            return false;
        }
    }

    // Check all conditions
    for (u32 i = 0; i < trans->condition_count; i++) {
        if (!check_condition(sm, &trans->conditions[i])) {
            return false;
        }
    }

    return true;
}

static void consume_triggers(AnimStateMachine* sm, const AnimTransition* trans) {
    for (u32 i = 0; i < trans->condition_count; i++) {
        if (trans->conditions[i].custom_condition) continue;

        u32 pid = trans->conditions[i].param_index;
        if (pid < sm->param_count) {
            AnimParameter* p = &sm->parameters[pid];
            if (p->type == ANIM_PARAM_TRIGGER) {
                p->trigger_consumed = true;
                p->value.bool_val = false;
            }
        }
    }
}

static void update_state(AnimStateMachine* sm, AnimState* state, f32 dt, bool is_active_state) {
    if (!state) return;

    // Advance time
    f32 duration = state->length > 0.0f ? state->length : 1.0f;
    f32 increment = (dt * state->speed * sm->global_speed) / duration;

    state->normalized_time += increment;

    // Loop handling
    if (state->normalized_time >= 1.0f) {
        if (state->loop) {
            state->normalized_time = fmodf(state->normalized_time, 1.0f);
            // Reset notifies if we looped
            for (u32 i = 0; i < state->notify_count; i++) {
                state->notifies[i].is_triggered = false;
            }
        } else {
            state->normalized_time = 1.0f;
        }
    }

    // Process Notifies
    if (is_active_state) {
        for (u32 i = 0; i < state->notify_count; i++) {
            AnimNotify* notify = &state->notifies[i];

            // Simple trigger check: if we passed the time in this frame
            // Or if we looped and the time is small
            bool should_trigger = false;

            if (!notify->is_triggered && state->normalized_time >= notify->time) {
                should_trigger = true;
            }

            if (should_trigger) {
                notify->is_triggered = true;
                if (notify->callback) {
                    notify->callback(sm, notify->name, sm->context_data);
                }
            }
        }

        // Update callback
        if (state->on_update) {
            state->on_update(sm, sm->context_data);
        }

        // Sub-machine update
        if (state->has_sub_machine && state->sub_state_machine) {
            anim_sm_update(state->sub_state_machine, dt);
        }
    }
}

void anim_sm_update(AnimStateMachine* sm, f32 dt) {
    if (!sm || sm->paused || sm->state_count == 0) return;

    for (u32 l = 0; l < sm->layer_count; l++) {
        AnimLayer* layer = &sm->layers[l];
        AnimState* current = &sm->states[layer->current_state_id];

        // 1. Update Current State
        update_state(sm, current, dt, !layer->is_transitioning);

        if (layer->is_transitioning) {
             AnimState* prev = &sm->states[layer->previous_state_id];
             // Also update previous state during transition (e.g. to finish exit time)
             update_state(sm, prev, dt, false);

             // Update Blend
             layer->transition_time += dt;
             f32 duration = layer->transition_duration > 0.0f ? layer->transition_duration : 0.001f;
             f32 t = layer->transition_time / duration;

             if (t >= 1.0f) {
                 // Finish Transition
                 layer->is_transitioning = false;
                 layer->previous_state_id = layer->current_state_id; // Just clean up reference

                 // End callback for previous
                 if (prev->on_exit) prev->on_exit(sm, sm->context_data);

                 // Trigger Transition End
                 if (layer->active_transition && layer->active_transition->on_transition_end) {
                     layer->active_transition->on_transition_end(sm, sm->context_data);
                 }
                 layer->active_transition = NULL;
             }
        }

        // 2. Check for Transitions (if not transitioning, or if interruption allowed)
        bool allow_check = !layer->is_transitioning;
        u32 check_from_state = layer->current_state_id;

        // Interruption logic
        if (layer->is_transitioning && layer->active_transition) {
            AnimInterruptionSource is = layer->active_transition->interruption_source;
            if (is == ANIM_INTERRUPT_SOURCE || is == ANIM_INTERRUPT_ANY_SOURCE) {
                allow_check = true;
                check_from_state = layer->previous_state_id;
            } else if (is == ANIM_INTERRUPT_DESTINATION || is == ANIM_INTERRUPT_ANY_DESTINATION) {
                allow_check = true;
                check_from_state = layer->current_state_id;
            }
        }

        if (allow_check) {
            AnimTransition* best_trans = NULL;
            int best_priority = -9999;

            for (u32 t = 0; t < sm->transition_count; t++) {
                AnimTransition* trans = &sm->transitions[t];

                if (trans->from_state_id != check_from_state) continue;

                // Get the state object we are checking from to check exit time against
                AnimState* check_state_obj = &sm->states[check_from_state];

                if (check_transition(sm, trans, check_state_obj)) {
                    if (trans->priority > best_priority) {
                        best_trans = trans;
                        best_priority = trans->priority;
                    }
                }
            }

            if (best_trans) {
                // START TRANSITION

                // If we are interrupting, we might want to handle callbacks differently?
                // For now, treat interruption as a new transition starting from current blend state.

                // If interrupting source, we are basically leaving source (again?) or redirecting.
                // If interrupting destination, we are leaving destination early.

                AnimState* next = &sm->states[best_trans->to_state_id];
                if (next->on_enter) next->on_enter(sm, sm->context_data);

                // Reset new state
                next->normalized_time = best_trans->offset;
                for (u32 i = 0; i < next->notify_count; i++) next->notifies[i].is_triggered = false;

                // Setup Layer
                // If we were transitioning, previous_state_id was the OLD old state.
                // If we interrupt, what becomes the new previous?
                // Visuals will blend from "Current Blend Result" to New State.
                // Since we don't have a "Blend Result" state object, we usually keep the logic simple:
                // If interrupting Source -> We are blending from Source to New Target.
                // If interrupting Destination -> We are blending from Destination to New Target.

                if (layer->is_transitioning) {
                     // We are interrupting.
                     // The previous transition is cancelled.
                     if (layer->active_transition && layer->active_transition->on_transition_interrupt) {
                         layer->active_transition->on_transition_interrupt(sm, sm->context_data);
                     }
                }

                // Update state IDs
                layer->previous_state_id = check_from_state; // Start blend from where we checked
                layer->current_state_id = best_trans->to_state_id;

                layer->is_transitioning = true;
                layer->transition_time = 0.0f;
                layer->transition_duration = best_trans->duration;
                layer->active_transition = best_trans;

                // Consume triggers
                consume_triggers(sm, best_trans);

                // Transition Start Callback
                if (best_trans->on_transition_start) {
                    best_trans->on_transition_start(sm, sm->context_data);
                }
            }
        }
    }
}

void anim_sm_set_context(AnimStateMachine* sm, void* context) {
    if (sm) sm->context_data = context;
}

u32 anim_sm_get_current_state(AnimStateMachine* sm, u32 layer_id) {
    if (!sm || layer_id >= sm->layer_count) return 0;
    return sm->layers[layer_id].current_state_id;
}

f32 anim_sm_get_current_time(AnimStateMachine* sm, u32 layer_id) {
    if (!sm || layer_id >= sm->layer_count) return 0.0f;
    u32 state_id = sm->layers[layer_id].current_state_id;
    if (state_id < sm->state_count) {
        return sm->states[state_id].normalized_time;
    }
    return 0.0f;
}
