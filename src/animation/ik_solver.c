// src/engine/animation/ik_solver.c - Inverse Kinematics Solver
// Matches actual Bone/Skeleton types from animation_system.h

#include "engine/include/math/math.h"
#include <animation/animation_system.h>
// animation/ik_solver.c
// Advanced Inverse Kinematics (IK) and Full-Body IK (FBIK) implementation.
//
// TODO: Implement CCD (Cyclic Coordinate Descent) IK solver for basic limbs.
// TODO: Add support for FABRIK (Forward And Backward Reaching Inverse
// Kinematics) for multi-chain IK.
// TODO: Implement Full-Body IK (FBIK) with center-of-mass balancing logic.
// TODO: Add support for bone-constraints (Angle limits, Twist limits) during
// solving.
// TODO: Implement Foot-Placement IK for terrain adaptation using ray-casting.
// TODO: Add support for Pole-Vector constraints for realistic elbow/knee
// positioning.
// TODO: Implement Look-At IK for head/eye tracking.
// TODO: Add support for multi-effector IK goals (Hand + Foot + Head).
// TODO: Implement soft-limits and damping to avoid joint-popping.
// TODO: Research and implement Bio-mechanical constraints for realistic
// movement.
// TODO: Add support for networked IK synchronization (IK-Target replication).

#include "engine/include/common.h"
#include <stdlib.h>

// =============================================================================
// IK CHAIN DEFINITION (local, since not in header)
// =============================================================================

typedef struct IKChain {
  i32 root_bone_index;
  i32 end_effector_bone_index;
  Vec3 target_position;
  u32 iteration_count;
  u32 bone_count;
  bool active;
} IKChain;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

static inline Vec3 ik_vec3_sub(Vec3 a, Vec3 b) {
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline f32 ik_vec3_length(Vec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 ik_vec3_normalize(Vec3 v) {
  f32 len = ik_vec3_length(v);
  if (len < 0.0001f)
    return (Vec3){0, 1, 0};
  return (Vec3){v.x / len, v.y / len, v.z / len};
}

static inline f32 ik_vec3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 ik_vec3_cross(Vec3 a, Vec3 b) {
  return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
}

// Extract position from Mat4 (translation column)
static inline Vec3 mat4_get_translation(const Mat4 *m) {
  return (Vec3){m->m[12], m->m[13], m->m[14]};
}

// Get world position of bone using skeleton's global_transforms
static Vec3 get_bone_world_pos(Skeleton *skeleton, i32 bone_idx) {
  if (!skeleton || bone_idx < 0 || bone_idx >= (i32)skeleton->bone_count) {
    return (Vec3){0, 0, 0};
  }

  if (skeleton->global_transforms) {
    return mat4_get_translation(&skeleton->global_transforms[bone_idx]);
  }

  // Fallback: use local_transform
  return mat4_get_translation(&skeleton->bones[bone_idx].local_transform);
}

// =============================================================================
// CCD IK SOLVER
// =============================================================================

void animation_solve_ik(Skeleton *skeleton, IKChain *chain) {
  if (!skeleton || !chain || !chain->active)
    return;
  if (chain->bone_count < 2)
    return;
  if (!skeleton->global_transforms)
    return;

  const f32 tolerance = 0.01f;

  // Iterate CCD
  for (u32 iter = 0; iter < chain->iteration_count; iter++) {
    // Get current end effector position
    Vec3 end_pos = get_bone_world_pos(skeleton, chain->end_effector_bone_index);

    // Check if close enough to target
    Vec3 to_target = ik_vec3_sub(chain->target_position, end_pos);
    f32 dist = ik_vec3_length(to_target);

    if (dist < tolerance) {
      break; // Close enough
    }

    // Iterate from end effector towards root
    i32 bone_idx = chain->end_effector_bone_index;

    for (u32 i = 0; i < chain->bone_count - 1 && bone_idx >= 0; i++) {
      // Get bone position
      Vec3 bone_pos = get_bone_world_pos(skeleton, bone_idx);

      // Recalculate end effector position
      end_pos = get_bone_world_pos(skeleton, chain->end_effector_bone_index);

      // Vector from bone to end effector
      Vec3 to_end = ik_vec3_sub(end_pos, bone_pos);

      // Vector from bone to target
      Vec3 to_target_local = ik_vec3_sub(chain->target_position, bone_pos);

      // Normalize
      to_end = ik_vec3_normalize(to_end);
      to_target_local = ik_vec3_normalize(to_target_local);

      // Calculate rotation axis and angle
      Vec3 axis = ik_vec3_cross(to_end, to_target_local);
      f32 axis_len = ik_vec3_length(axis);

      if (axis_len > 0.0001f) {
        axis = ik_vec3_normalize(axis);
        f32 dot = ik_vec3_dot(to_end, to_target_local);
        dot = fminf(1.0f, fmaxf(-1.0f, dot));
        f32 angle = acosf(dot);

        // Clamp angle for stability
        f32 max_angle = 0.5f; // ~28 degrees per iteration
        if (angle > max_angle)
          angle = max_angle;

        // Apply rotation to bone's local transform
        // This requires matrix rotation - simplified for now
        // Full implementation would use mat4_rotate_axis
      }

      // Move to parent bone
      bone_idx = skeleton->bones[bone_idx].parent_index;
    }
  }
}

// =============================================================================
// TWO-BONE IK (Arm/Leg) - Analytic Solution
// =============================================================================

void animation_solve_two_bone_ik(Skeleton *skeleton, i32 root_idx, i32 mid_idx,
                                 i32 end_idx, Vec3 target, Vec3 pole_hint) {
  if (!skeleton || !skeleton->global_transforms)
    return;
  if (root_idx < 0 || mid_idx < 0 || end_idx < 0)
    return;

  // Get bone positions
  Vec3 root_pos = get_bone_world_pos(skeleton, root_idx);
  Vec3 mid_pos = get_bone_world_pos(skeleton, mid_idx);
  Vec3 end_pos = get_bone_world_pos(skeleton, end_idx);

  // Bone lengths
  f32 upper_len = ik_vec3_length(ik_vec3_sub(mid_pos, root_pos));
  f32 lower_len = ik_vec3_length(ik_vec3_sub(end_pos, mid_pos));

  // Distance to target
  Vec3 to_target = ik_vec3_sub(target, root_pos);
  f32 target_dist = ik_vec3_length(to_target);

  // Clamp to reachable distance
  f32 max_reach = upper_len + lower_len - 0.01f;
  f32 min_reach = fabsf(upper_len - lower_len) + 0.01f;

  if (target_dist > max_reach)
    target_dist = max_reach;
  if (target_dist < min_reach)
    target_dist = min_reach;

  // Law of cosines for elbow angle
  f32 cos_angle = (upper_len * upper_len + lower_len * lower_len -
                   target_dist * target_dist) /
                  (2.0f * upper_len * lower_len);
  cos_angle = fminf(1.0f, fmaxf(-1.0f, cos_angle));
  (void)cos_angle; // Would be used to compute actual elbow bend

  // Apply rotations using pole hint for elbow direction
  (void)pole_hint; // Full implementation would use this
}
