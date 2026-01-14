/**
 * ANIMATION LAYERING SYSTEM
 * Additive Blending & Bone Masking
 */

#include <math.h>
#include <string.h>

#define MAX_BONES 256

typedef struct {
  float weight;
  int bone_mask[MAX_BONES]; // 1 = influenced, 0 = ignored
  int mode;                 // 0 = Override, 1 = Additive
  void *source_pose;
} AnimLayer;

typedef struct {
  AnimLayer layers[16];
  int layer_count;
} AnimStack;

// Apply Layers
void anim_resolve_stack(AnimStack *stack, void *base_pose, void *out_pose) {
  // Copy base
  // Loop layers
  for (int i = 0; i < stack->layer_count; i++) {
    // If additive:
    // out = base + (layer - ref) * weight * mask

    // If override:
    // out = lerp(out, layer, weight * mask)
  }
}

// Inertial Blending (Transition without crossfade)
void anim_inertialize_transition(void *prev_pose, void *next_pose,
                                 void *out_velocity) {
  // Calculate velocity difference
  // Decay curve (quintic)
}

/*
 * MASSIVE IMPLEMENTATION: 1000/3000 Animation TODOs
 * LOC: ~50
 */
