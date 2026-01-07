#ifndef ANIMATION_STATE_MACHINE_H
#define ANIMATION_STATE_MACHINE_H

#include "character/animation/animation_clip.h"
#include <simd/simd.h>

#define MAX_STATES 32

typedef struct animation_state {
    char name[64];
    animation_clip_t* clip;
    bool looping;
} animation_state_t;

typedef struct animation_state_machine {
    animation_state_t states[MAX_STATES];
    uint32_t state_count;
    
    int32_t current_state_index;
    float current_time;
    
    int32_t target_state_index; // For blending
    float transition_time;
    float transition_duration;
    
    simd_float4x4* bone_transforms; // Output buffer
    uint32_t bone_count;
} animation_state_machine_t;

animation_state_machine_t* asm_create(uint32_t bone_count);
void asm_destroy(animation_state_machine_t* asm_inst);
int32_t asm_add_state(animation_state_machine_t* asm_inst, const char* name, animation_clip_t* clip, bool looping);
void asm_transition_to(animation_state_machine_t* asm_inst, const char* state_name, float duration);
void asm_update(animation_state_machine_t* asm_inst, float delta_time);

#endif // ANIMATION_STATE_MACHINE_H
