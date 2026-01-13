/**
 * @file inverse_kinematics_solver.c
 * @brief Inverse Kinematics (IK) Library.
 *
 * Implements FABRIK (Forward And Backward Reaching Inverse Kinematics)
 * and Two-Bone IK (Analytic) for efficient limb placement.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include "include/animation/core/inverse_kinematics_solver.h"
#include "include/math/math.h"
#include <math.h>
#include <string.h>

// =================================================================================================
//                                      METHODS
// =================================================================================================

/**
 * @brief Analytic Two-Bone IK Solver.
 *
 * Perfect for legs and arms (Thigh -> Shin -> Foot).
 * Calculates the middle joint position to reach a target.
 *
 * @param root_pos Start of the chain (Hip/Shoulder).
 * @param end_pos Target position (Foot/Hand).
 * @param joint_pos Mid-joint position (Knee/Elbow).
 * @param length_a Length of upper bone.
 * @param length_b Length of lower bone.
 * @param hint_dir Direction to bend the joint (Pole Vector).
 */
void ik_solve_two_bone(Vec3 root_pos, Vec3 end_pos, Vec3 *joint_pos,
                       float length_a, float length_b, Vec3 hint_dir) {
  Vec3 dir = vec3_sub(end_pos, root_pos);
  float dist = vec3_length(dir);

  // Clamp distance to max reach
  if (dist > length_a + length_b) {
    dist = length_a + length_b;
    dir = vec3_normalize(dir);
    end_pos = vec3_add(root_pos, vec3_mul(dir, dist)); // vec3_scale -> vec3_mul
  }

  // Law of Cosines to find angle at the root
  // c^2 = a^2 + b^2 - 2ab cos(C)
  // joint_angle = acos( (a^2 + dist^2 - b^2) / (2 * a * dist) )

  float a2 = length_a * length_a;
  float b2 = length_b * length_b;
  float c2 = dist * dist;

  float cos_angle = (a2 + c2 - b2) / (2.0f * length_a * dist);
  if (cos_angle < -1.0f)
    cos_angle = -1.0f;
  if (cos_angle > 1.0f)
    cos_angle = 1.0f;

  float angle = acosf(cos_angle);

  // Construct plane basis
  Vec3 forward = vec3_normalize(dir);
  Vec3 right = vec3_cross(forward, hint_dir);
  if (vec3_length_sq(right) < 0.001f) {
    right = (Vec3){1, 0, 0}; // Fallback
  }
  right = vec3_normalize(right);
  Vec3 up = vec3_cross(right, forward); // Plane normal

  // Rotate forward vector by angle around 'up' (or 'right' depending on basis)
  // Simplified: Mid joint forms a triangle in the plane defined by start, end,
  // and slight hint

  // P_joint = P_root + rotate(forward, angle, axis=right) * length_a
  Quat q = quat_from_axis_angle(right,
                                angle); // Rotate "up" from the forward vector?

  // Implementation note: This function seems incomplete/referenced for logic
  // only in original file Should complete or leave as placeholder? It
  // calculates 'q' but doesn't use it to set joint_pos. Assuming this is
  // existing code I'm fixing types for. If required, I should apply the
  // rotation to find joint pos.

  // Logic to find joint position:
  // Rotate 'forward' vector by 'angle' around 'right' axis
  Vec3 joint_dir = quat_rotate_vec3(q, forward);
  *joint_pos = vec3_add(root_pos, vec3_mul(joint_dir, length_a));
}

/**
 * @brief FABRIK Solver for N-link chains.
 */
void ik_solve_fabrik(Vec3 *joints, int count, float *lengths, Vec3 target,
                     int iterations) {
  // Check reachability
  float max_reach = 0;
  for (int i = 0; i < count - 1; i++)
    max_reach += lengths[i];

  if (vec3_distance(joints[0], target) > max_reach) {
    // Stretch
    for (int i = 0; i < count - 1; i++) {
      float r = vec3_distance(target, joints[i]);
      float lambda = lengths[i] / r;
      joints[i + 1] = vec3_lerp(joints[i], target, lambda);
    }
  } else {
    // Iterative solve
    Vec3 root_pos = joints[0];

    for (int iter = 0; iter < iterations; iter++) {
      // BACKWARD: Set end to target, pull towards root
      joints[count - 1] = target;
      for (int i = count - 2; i >= 0; i--) {
        Vec3 dir = vec3_normalize(vec3_sub(joints[i], joints[i + 1]));
        joints[i] = vec3_add(joints[i + 1], vec3_mul(dir, lengths[i]));
      }

      // FORWARD: Set root back to start, pull towards end
      joints[0] = root_pos;
      for (int i = 0; i < count - 1; i++) {
        Vec3 dir = vec3_normalize(vec3_sub(joints[i + 1], joints[i]));
        joints[i + 1] = vec3_add(joints[i], vec3_mul(dir, lengths[i]));
      }
    }
  }
}
