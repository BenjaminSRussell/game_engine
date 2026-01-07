#include "../core/physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Note: Using standard types where possible, but Ragdoll has specific structs
// We define them here or move to types header if shared globally.
// For now, these seem specific to this module.

typedef struct {
  float position[3], rotation[4];
  float velocity[3], angular_velocity[3];
  float mass, inv_mass;
  int bone_index;
} RagdollBodyInternal;

typedef struct {
  int body_a, body_b;
  float anchor_a[3], anchor_b[3];
  float min_angle, max_angle;
  float stiffness;
} RagdollJointInternal;

typedef struct RagdollSystem {
  RagdollBodyInternal *bodies;
  int body_count;
  RagdollJointInternal *joints;
  int joint_count;
  bool active;
  float blend_weight; // For blending back to animation
} RagdollSystem;

RagdollSystem *ragdoll_create(int body_count, int joint_count) {
  RagdollSystem *rd = (RagdollSystem*)calloc(1, sizeof(RagdollSystem));
  if (!rd) return NULL;
  rd->bodies = (RagdollBodyInternal*)calloc(body_count, sizeof(RagdollBodyInternal));
  rd->body_count = body_count;
  rd->joints = (RagdollJointInternal*)calloc(joint_count, sizeof(RagdollJointInternal));
  rd->joint_count = joint_count;
  rd->blend_weight = 0;
  return rd;
}

void ragdoll_activate(RagdollSystem *rd, float skeleton_poses[][7]) {
  if (!rd) return;
  rd->active = true;
  rd->blend_weight = 1.0f;

  for (int i = 0; i < rd->body_count; i++) {
    memcpy(rd->bodies[i].position, skeleton_poses[i], sizeof(float) * 3);
    memcpy(rd->bodies[i].rotation, skeleton_poses[i] + 3, sizeof(float) * 4);
    // Reset velocities?
    memset(rd->bodies[i].velocity, 0, sizeof(float)*3);
    memset(rd->bodies[i].angular_velocity, 0, sizeof(float)*3);
  }
}

void ragdoll_update(RagdollSystem *rd, float dt) {
  if (!rd || !rd->active)
    return;

  // Apply gravity
  for (int i = 0; i < rd->body_count; i++) {
    rd->bodies[i].velocity[1] -= 9.8f * dt;
  }

  // Solve joint constraints
  for (int iter = 0; iter < 4; iter++) {
    for (int i = 0; i < rd->joint_count; i++) {
      RagdollJointInternal *joint = &rd->joints[i];
      RagdollBodyInternal *a = &rd->bodies[joint->body_a];
      RagdollBodyInternal *b = &rd->bodies[joint->body_b];

      // Simplified joint constraint
      float diff[3];
      for (int j = 0; j < 3; j++) {
        diff[j] = (b->position[j] - a->position[j]);
      }

      float dist =
          sqrtf(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
      if (dist > 0.001f) {
        for (int j = 0; j < 3; j++) {
          float correction = diff[j] / dist * joint->stiffness;
          a->position[j] += correction * 0.5f;
          b->position[j] -= correction * 0.5f;
        }
      }
    }
  }

  // Integrate positions
  for (int i = 0; i < rd->body_count; i++) {
    rd->bodies[i].position[0] += rd->bodies[i].velocity[0] * dt;
    rd->bodies[i].position[1] += rd->bodies[i].velocity[1] * dt;
    rd->bodies[i].position[2] += rd->bodies[i].velocity[2] * dt;
  }

  // Blend back to animation if deactivating
  if (rd->blend_weight < 1.0f) {
    rd->blend_weight -= dt * 0.5f;
    if (rd->blend_weight <= 0) {
      rd->active = false;
      rd->blend_weight = 0;
    }
  }
}
