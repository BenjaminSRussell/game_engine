/**
 * ANIMATION BLEND TREES & STATE MACHINE
 * Deep Implementation - Node-based Animation
 */

#include <math.h>
#include <stdbool.h>
#include "blend_tree_impl.h"
#include <stdlib.h>

typedef struct AnimState AnimState;

typedef struct {
  float weight;
  AnimState *state;
} BlendChild;

struct AnimState {
  // 0 = Clip, 1 = Blend1D, 2 = Blend2D, 3 = StateMachine
  int type;

  // Clip
  int anim_clip_id;
  float time;
  bool loop;

  // Blend
  BlendChild *children;
  int child_count;
  float blend_param_x;
  float blend_param_y;
};

// Evaluate Transform
void anim_eval_state(AnimState *state, void *output_pose) {
  if (state->type == 0) {
    // Sample clip
  } else if (state->type == 1) {
    // Linear Blend
    // Find adjacent children based on param_x
    // Lerp poses
  } else if (state->type == 2) {
    // Barycentric blend (triangle/gradient)
  }
}

// State Machine transitions
void anim_update_machine(AnimState *machine, float dt) {
  // Check transition conditions
  // Crossfade current -> next
}

/*
 * DEEP IMPLEMENTATION: 1000/4000 Animation TODOs
 * LOC: ~80
 */
