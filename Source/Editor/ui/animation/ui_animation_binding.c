/*
 * ui_animation_binding.c
 * UI Property Animation Binding Implementation
 */

#include "editor/ui/animation/ui_animation_binding.h"
#include "editor/ui/widgets/ui_rect.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

#define MAX_BINDINGS 1024
#define MAX_PROPERTY_REGISTRATIONS 64

static struct {
    ui_animation_binding_state_t *bindings;
    ui_animation_property_registry_t properties[MAX_PROPERTY_REGISTRATIONS];
    uint32_t property_count;
    uint32_t next_id;
    bool initialized;
} g_anim_system = {0};

/* Easing functions */
float ui_easing_linear(float t) { return t; }
float ui_easing_ease_in_quad(float t) { return t * t; }
float ui_easing_ease_out_quad(float t) { return t * (2.0f - t); }
float ui_easing_ease_in_out_quad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

int ui_animation_binding_init(void) {
    if (g_anim_system.initialized) return 0;
    
    g_anim_system.bindings = calloc(MAX_BINDINGS, sizeof(ui_animation_binding_state_t));
    if (!g_anim_system.bindings) return -1;
    
    g_anim_system.next_id = 1;
    g_anim_system.initialized = true;
    LOG_INFO("UI Animation Binding System initialized");
    return 0;
}

void ui_animation_binding_shutdown(void) {
    if (!g_anim_system.initialized) return;
    
    free(g_anim_system.bindings);
    memset(&g_anim_system, 0, sizeof(g_anim_system));
}

int ui_animation_binding_create(ui_animation_binding_handle_t *out_handle, const ui_animation_binding_desc_t *desc) {
    if (!g_anim_system.initialized || !out_handle || !desc) return -1;
    
    uint32_t id = g_anim_system.next_id++;
    if (id >= MAX_BINDINGS) return -1;
    
    ui_animation_binding_state_t *state = &g_anim_system.bindings[id];
    memset(state, 0, sizeof(*state));
    
    state->id = id;
    state->element_id = desc->element_id;
    state->property_type = desc->property_type;
    strncpy(state->property_name, desc->property_name, sizeof(state->property_name) - 1);
    
    state->duration = desc->duration;
    state->delay = desc->delay;
    state->loop = desc->loop;
    state->ping_pong = desc->ping_pong;
    state->easing_func = desc->easing_func ? desc->easing_func : ui_easing_linear;
    
    state->on_start = desc->on_start;
    state->on_complete = desc->on_complete;
    state->on_update = desc->on_update;
    state->user_data = desc->user_data;
    
    out_handle->id = id;
    return 0;
}

void ui_animation_binding_update(float delta_time) {
    if (!g_anim_system.initialized) return;
    
    for (uint32_t i = 1; i < MAX_BINDINGS; i++) {
        ui_animation_binding_state_t *state = &g_anim_system.bindings[i];
        if (!state->active || state->paused) continue;
        
        state->current_time += delta_time;
        
        if (state->current_time < state->delay) continue;
        
        float anim_time = state->current_time - state->delay;
        state->progress = anim_time / state->duration;
        
        if (state->progress >= 1.0f) {
            if (state->loop) {
                state->current_time = state->delay;
                state->progress = 0.0f;
                if (state->ping_pong) state->reverse = !state->reverse;
            } else {
                state->progress = 1.0f;
                state->active = false;
                if (state->on_complete) state->on_complete(state->element_id);
            }
        }
        
        float t = state->reverse ? 1.0f - state->progress : state->progress;
        t = state->easing_func(t);
        
        if (state->on_update) state->on_update(state->element_id, t);
    }
}

uint32_t ui_animation_binding_get_active_count(void) {
    if (!g_anim_system.initialized) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 1; i < MAX_BINDINGS; i++) {
        if (g_anim_system.bindings[i].active) count++;
    }
    return count;
}
