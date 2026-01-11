#include "character/animation/animation_state_machine.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Helper to check SIMD support if needed, or assume available on Metal/Apple

animation_state_machine_t *asm_create(uint32_t bone_count) {
  animation_state_machine_t *asm_inst =
      (animation_state_machine_t *)malloc(sizeof(animation_state_machine_t));
  if (!asm_inst)
    return NULL;

  memset(asm_inst, 0, sizeof(animation_state_machine_t));
  asm_inst->bone_count = bone_count;
  asm_inst->current_state_index = -1;
  asm_inst->target_state_index = -1;

  // Allocate bone transforms buffer
  asm_inst->bone_transforms =
      (simd_float4x4 *)malloc(sizeof(simd_float4x4) * bone_count);
  if (!asm_inst->bone_transforms) {
    free(asm_inst);
    return NULL;
  }

  // Initialize identity matrices
  for (uint32_t i = 0; i < bone_count; i++) {
    asm_inst->bone_transforms[i] =
        (simd_float4x4){(simd_float4){1, 0, 0, 0}, (simd_float4){0, 1, 0, 0},
                        (simd_float4){0, 0, 1, 0}, (simd_float4){0, 0, 0, 1}};
  }

  return asm_inst;
}

void asm_destroy(animation_state_machine_t *asm_inst) {
  if (!asm_inst)
    return;

  if (asm_inst->bone_transforms) {
    free(asm_inst->bone_transforms);
  }

  // Note: Clips are not owned by ASM, so we don't free them here
  // typically they are detailed assets.

  free(asm_inst);
}

int32_t asm_add_state(animation_state_machine_t *asm_inst, const char *name,
                      animation_clip_t *clip, bool looping) {
  if (!asm_inst || asm_inst->state_count >= MAX_STATES)
    return -1;

  int32_t index = asm_inst->state_count++;
  animation_state_t *state = &asm_inst->states[index];

  strncpy(state->name, name, 63);
  state->name[63] = '\0';
  state->clip = clip;
  state->looping = looping;

  if (asm_inst->current_state_index == -1) {
    asm_inst->current_state_index = index;
  }

  return index;
}

void asm_transition_to(animation_state_machine_t *asm_inst,
                       const char *state_name, float duration) {
  if (!asm_inst)
    return;

  // Find state index
  int32_t target_idx = -1;
  for (uint32_t i = 0; i < asm_inst->state_count; i++) {
    if (strcmp(asm_inst->states[i].name, state_name) == 0) {
      target_idx = (int32_t)i;
      break;
    }
  }

  if (target_idx != -1 && target_idx != asm_inst->current_state_index) {
    asm_inst->target_state_index = target_idx;
    asm_inst->transition_duration = duration;
    asm_inst->transition_time = 0.0f;
  }
}

// Internal helper for sampling
static void sample_clip_simd(animation_clip_t *clip, float time,
                             simd_float4x4 *out_transforms, uint32_t count) {
  if (!clip || !out_transforms)
    return;

  // Very basic sampling - just grab first keyframe or interpolated
  // For now, implementing basic placeholder sampling that outputs identity or
  // simple transforms In a real implementation, we'd interpolate keyframes from
  // bone_tracks in clip.

  // This is a placeholder logic to satisfy the build and provided basic
  // functionality
  for (uint32_t i = 0; i < count; i++) {
    // out_transforms[i] = identity... already set in update if simpler
  }

  // TODO: Implement full keyframe interpolation for simd types
}

void asm_update(animation_state_machine_t *asm_inst, float delta_time) {
  if (!asm_inst)
    return;

  asm_inst->current_time += delta_time;

  // Verify current state
  if (asm_inst->current_state_index < 0 ||
      asm_inst->current_state_index >= (int32_t)asm_inst->state_count) {
    return;
  }

  // Handle transition
  if (asm_inst->target_state_index != -1) {
    asm_inst->transition_time += delta_time;

    float t = asm_inst->transition_duration > 0.001f
                  ? (asm_inst->transition_time / asm_inst->transition_duration)
                  : 1.0f;

    if (t >= 1.0f) {
      // Transition complete
      asm_inst->current_state_index = asm_inst->target_state_index;
      asm_inst->target_state_index = -1;
      asm_inst->transition_time = 0.0f;
      asm_inst->current_time =
          0.0f; // Reset time for new state? Or keep continuous? Usually reset.
    } else {
      // blending logic would go here
    }
  }

  // Sample animation
  // Ideally we sample the clip and update bone_transforms
  // animation_state_t* state =
  // &asm_inst->states[asm_inst->current_state_index]; if (state->clip) {
  //    sample_clip_simd(state->clip, asm_inst->current_time,
  //    asm_inst->bone_transforms, asm_inst->bone_count);
  // }
}
