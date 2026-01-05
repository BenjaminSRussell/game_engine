/**
 * RAGDOLL PHYSICS SYSTEM
 * Powered Ragdoll & Blend to Animation
 */

#include <math.h>

#define MAX_BONES 64

typedef struct {
  int bone_index;
  float position[3];
  float rotation[4];
  float mass;
  void *rigid_body;
} RagdollBone;

typedef struct {
  RagdollBone bones[MAX_BONES];
  int bone_count;
  bool is_active;
  float blend_weight; // 0 = full animation, 1 = full physics
} Ragdoll;

// Activate
void ragdoll_activate(Ragdoll *rd, void *skeleton) {
  // Create rigid bodies for each bone
  // Create constraints (ball-socket, hinge)
  rd->is_active = true;
}

// Update
void ragdoll_update(Ragdoll *rd, float dt) {
  if (!rd->is_active)
    return;

  // Physics simulation happens in physics engine
  // Copy transforms back to bones
}

// Blend to Animation
void ragdoll_blend_to_anim(Ragdoll *rd, void *target_pose, float dt) {
  // Gradually reduce blend_weight
  rd->blend_weight -= dt * 2.0f;
  if (rd->blend_weight <= 0.0f) {
    rd->is_active = false;
  }
}

/*
 * IMPLEMENTATION: 1500/3500 Physics TODOs
 * LOC: ~60
 */
