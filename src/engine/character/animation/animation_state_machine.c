#include "character/animation/animation_state_machine.h"
#include "character/animation/animation_sampler.h"
#include "character/animation/animation_blender.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

animation_state_machine_t* asm_create(uint32_t bone_count) {
    animation_state_machine_t* sm = (animation_state_machine_t*)calloc(1, sizeof(animation_state_machine_t));
    if (!sm) return NULL;
    
    sm->bone_count = bone_count;
    sm->bone_transforms = (simd_float4x4*)malloc(sizeof(simd_float4x4) * bone_count);
    if (!sm->bone_transforms) {
        free(sm);
        return NULL;
    }
    
    sm->current_state_index = -1;
    sm->target_state_index = -1;
    
    return sm;
}

void asm_destroy(animation_state_machine_t* sm) {
    if (sm) {
        if (sm->bone_transforms) free(sm->bone_transforms);
        free(sm);
    }
}

int32_t asm_add_state(animation_state_machine_t* sm, const char* name, animation_clip_t* clip, bool looping) {
    if (!sm || sm->state_count >= MAX_STATES) return -1;
    
    uint32_t idx = sm->state_count++;
    strncpy(sm->states[idx].name, name, 63);
    sm->states[idx].clip = clip;
    sm->states[idx].looping = looping;
    
    // If first state, set as current
    if (sm->current_state_index == -1) {
        sm->current_state_index = idx;
        sm->current_time = 0.0f;
    }
    
    return idx;
}

void asm_transition_to(animation_state_machine_t* sm, const char* state_name, float duration) {
    if (!sm) return;
    
    for (uint32_t i = 0; i < sm->state_count; ++i) {
        if (strcmp(sm->states[i].name, state_name) == 0) {
            if (sm->current_state_index != i) {
                sm->target_state_index = i;
                sm->transition_duration = duration;
                sm->transition_time = 0.0f;
            }
            return;
        }
    }
}

void asm_update(animation_state_machine_t* sm, float delta_time) {
    if (!sm || sm->current_state_index == -1) return;
    
    sm->current_time += delta_time;
    
    // Handle current state sampling
    animation_state_t* current = &sm->states[sm->current_state_index];
    if (current->clip) {
        animation_sample(current->clip, sm->current_time, sm->bone_transforms, sm->bone_count);
    }
    
    // Handle blending if transitioning
    if (sm->target_state_index != -1) {
        sm->transition_time += delta_time;
        float t = sm->transition_duration > 0 ? sm->transition_time / sm->transition_duration : 1.0f;
        
        if (t >= 1.0f) {
            // Transition complete
            sm->current_state_index = sm->target_state_index;
            sm->current_time = sm->transition_time; // Carry over time? Or reset? Using 0 for new state usually safer
            sm->current_time = 0.0f; // Reset time for new state
            sm->target_state_index = -1;
            
            // Sample new state directly
            animation_state_t* new_state = &sm->states[sm->current_state_index];
             if (new_state->clip) {
                animation_sample(new_state->clip, sm->current_time, sm->bone_transforms, sm->bone_count);
            }
        } else {
            // Blend
            animation_state_t* target = &sm->states[sm->target_state_index];
            if (target->clip) {
                 simd_float4x4* target_transforms = (simd_float4x4*)malloc(sizeof(simd_float4x4) * sm->bone_count);
                 if (target_transforms) {
                     // Sample target state (starting from 0)
                     animation_sample(target->clip, sm->transition_time, target_transforms, sm->bone_count);
                     
                     // Blend in place into sm->bone_transforms
                     animation_blend(sm->bone_transforms, target_transforms, t, sm->bone_transforms, sm->bone_count);
                     
                     free(target_transforms);
                 }
            }
        }
    }
}
