/**
 * =================================================================================================
 *                          FABRIK IK SOLVER IMPLEMENTATION
 *                          Phase 3: Animation System
 * =================================================================================================
 *
 * PURPOSE: Forward And Backward Reaching Inverse Kinematics
 * Fast and stable for arbitrary chain lengths
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "animation/ik_advanced/fabrik.h"

#define FABRIK_MAX_BONES 32
#define FABRIK_DEFAULT_ITERATIONS 10
#define FABRIK_DEFAULT_TOLERANCE 0.01f

typedef struct {
  float x, y, z;
} FabrikVec3;

// FABRIK chain structure definition
struct FABRIKChain {
  uint32_t bone_indices[FABRIK_MAX_BONES];
  uint32_t bone_count;

  FabrikVec3 joint_positions[FABRIK_MAX_BONES + 1];
  float bone_lengths[FABRIK_MAX_BONES];
  float total_length;

  FabrikVec3 target_position;
  FabrikVec3 pole_vector;
  FabrikVec3 root_position;

  uint32_t max_iterations;
  float tolerance;
  bool use_pole_vector;
  bool reached_target;
};

// -----------------------------------------------------------------------------
// Vector Math
// -----------------------------------------------------------------------------

static inline FabrikVec3 fabrik_vec3(float x, float y, float z) {
  FabrikVec3 v = {x, y, z};
  return v;
}

static inline FabrikVec3 fabrik_vec3_sub(FabrikVec3 a, FabrikVec3 b) {
  return fabrik_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline FabrikVec3 fabrik_vec3_add(FabrikVec3 a, FabrikVec3 b) {
  return fabrik_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline FabrikVec3 fabrik_vec3_scale(FabrikVec3 v, float s) {
  return fabrik_vec3(v.x * s, v.y * s, v.z * s);
}

static inline float fabrik_vec3_length(FabrikVec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float fabrik_vec3_distance(FabrikVec3 a, FabrikVec3 b) {
  return fabrik_vec3_length(fabrik_vec3_sub(a, b));
}

static inline FabrikVec3 fabrik_vec3_normalize(FabrikVec3 v) {
  float len = fabrik_vec3_length(v);
  if (len > 0.0001f) {
    return fabrik_vec3_scale(v, 1.0f / len);
  }
  return fabrik_vec3(0.0f, 1.0f, 0.0f);
}

static inline float fabrik_vec3_dot(FabrikVec3 a, FabrikVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline FabrikVec3 fabrik_vec3_cross(FabrikVec3 a, FabrikVec3 b) {
  return fabrik_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                     a.x * b.y - a.y * b.x);
}

// -----------------------------------------------------------------------------
// Chain Creation
// -----------------------------------------------------------------------------

FABRIKChain *fabrik_chain_create(const float *joint_positions,
                                 uint32_t bone_count) {
  if (bone_count == 0 || bone_count > FABRIK_MAX_BONES)
    return NULL;

  FABRIKChain *chain = (FABRIKChain *)calloc(1, sizeof(FABRIKChain));
  if (!chain)
    return NULL;

  chain->bone_count = bone_count;
  chain->max_iterations = FABRIK_DEFAULT_ITERATIONS;
  chain->tolerance = FABRIK_DEFAULT_TOLERANCE;
  chain->total_length = 0.0f;

  // Copy joint positions (bone_count + 1 joints for bone_count bones)
  for (uint32_t i = 0; i <= bone_count; i++) {
    chain->joint_positions[i] =
        fabrik_vec3(joint_positions[i * 3], joint_positions[i * 3 + 1],
                    joint_positions[i * 3 + 2]);
  }

  // Compute bone lengths
  for (uint32_t i = 0; i < bone_count; i++) {
    chain->bone_lengths[i] = fabrik_vec3_distance(
        chain->joint_positions[i], chain->joint_positions[i + 1]);
    chain->total_length += chain->bone_lengths[i];
  }

  // Store root position
  chain->root_position = chain->joint_positions[0];

  // Default pole vector (pointing forward)
  chain->pole_vector = fabrik_vec3(0.0f, 0.0f, 1.0f);

  return chain;
}

void fabrik_chain_destroy(FABRIKChain *chain) {
  if (chain)
    free(chain);
}

// -----------------------------------------------------------------------------
// FABRIK Core Algorithm
// -----------------------------------------------------------------------------

static void fabrik_forward_reach(FABRIKChain *chain) {
  // Set end effector to target
  chain->joint_positions[chain->bone_count] = chain->target_position;

  // Iterate backward from end effector to root
  for (int i = (int)chain->bone_count - 1; i >= 0; i--) {
    FabrikVec3 direction = fabrik_vec3_sub(chain->joint_positions[i],
                                           chain->joint_positions[i + 1]);
    direction = fabrik_vec3_normalize(direction);

    chain->joint_positions[i] =
        fabrik_vec3_add(chain->joint_positions[i + 1],
                        fabrik_vec3_scale(direction, chain->bone_lengths[i]));
  }
}

static void fabrik_backward_reach(FABRIKChain *chain) {
  // Restore root to original position
  chain->joint_positions[0] = chain->root_position;

  // Iterate forward from root to end effector
  for (uint32_t i = 0; i < chain->bone_count; i++) {
    FabrikVec3 direction = fabrik_vec3_sub(chain->joint_positions[i + 1],
                                           chain->joint_positions[i]);
    direction = fabrik_vec3_normalize(direction);

    chain->joint_positions[i + 1] =
        fabrik_vec3_add(chain->joint_positions[i],
                        fabrik_vec3_scale(direction, chain->bone_lengths[i]));
  }
}

static void fabrik_apply_pole_vector(FABRIKChain *chain) {
  if (!chain->use_pole_vector || chain->bone_count < 2)
    return;

  // Only apply to middle joint for now (2-bone IK like arm/leg)
  uint32_t mid = chain->bone_count / 2;

  FabrikVec3 root = chain->joint_positions[0];
  FabrikVec3 end = chain->joint_positions[chain->bone_count];
  FabrikVec3 mid_joint = chain->joint_positions[mid];

  // Create plane from root -> end
  FabrikVec3 chain_dir = fabrik_vec3_normalize(fabrik_vec3_sub(end, root));
  FabrikVec3 pole_dir =
      fabrik_vec3_normalize(fabrik_vec3_sub(chain->pole_vector, root));

  // Project pole onto perpendicular plane
  float dot = fabrik_vec3_dot(pole_dir, chain_dir);
  FabrikVec3 pole_proj =
      fabrik_vec3_sub(pole_dir, fabrik_vec3_scale(chain_dir, dot));
  pole_proj = fabrik_vec3_normalize(pole_proj);

  // Project mid joint onto plane
  FabrikVec3 mid_offset = fabrik_vec3_sub(mid_joint, root);
  float mid_along = fabrik_vec3_dot(mid_offset, chain_dir);
  FabrikVec3 mid_on_axis =
      fabrik_vec3_add(root, fabrik_vec3_scale(chain_dir, mid_along));

  // Distance from axis
  float dist_from_axis = fabrik_vec3_distance(mid_joint, mid_on_axis);

  // Move mid joint toward pole direction
  chain->joint_positions[mid] = fabrik_vec3_add(
      mid_on_axis, fabrik_vec3_scale(pole_proj, dist_from_axis));
}

bool fabrik_solve(FABRIKChain *chain, float target_x, float target_y,
                  float target_z) {
  if (!chain)
    return false;

  chain->target_position = fabrik_vec3(target_x, target_y, target_z);
  chain->reached_target = false;

  // Check if target is reachable
  float dist_to_target =
      fabrik_vec3_distance(chain->root_position, chain->target_position);

  if (dist_to_target > chain->total_length) {
    // Target unreachable - stretch toward it
    FabrikVec3 direction = fabrik_vec3_normalize(
        fabrik_vec3_sub(chain->target_position, chain->root_position));

    chain->joint_positions[0] = chain->root_position;
    for (uint32_t i = 0; i < chain->bone_count; i++) {
      chain->joint_positions[i + 1] =
          fabrik_vec3_add(chain->joint_positions[i],
                          fabrik_vec3_scale(direction, chain->bone_lengths[i]));
    }
    return false;
  }

  // FABRIK iteration
  for (uint32_t iter = 0; iter < chain->max_iterations; iter++) {
    // Forward reach
    fabrik_forward_reach(chain);

    // Apply pole vector constraint
    if (chain->use_pole_vector) {
      fabrik_apply_pole_vector(chain);
    }

    // Backward reach
    fabrik_backward_reach(chain);

    // Check convergence
    float error = fabrik_vec3_distance(
        chain->joint_positions[chain->bone_count], chain->target_position);

    if (error < chain->tolerance) {
      chain->reached_target = true;
      return true;
    }
  }

  // Didn't fully converge but got close
  return false;
}

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

void fabrik_set_iterations(FABRIKChain *chain, uint32_t iterations) {
  if (chain)
    chain->max_iterations = iterations;
}

void fabrik_set_tolerance(FABRIKChain *chain, float tolerance) {
  if (chain)
    chain->tolerance = tolerance;
}

void fabrik_set_pole_vector(FABRIKChain *chain, float x, float y, float z) {
  if (!chain)
    return;
  chain->pole_vector = fabrik_vec3(x, y, z);
  chain->use_pole_vector = true;
}

void fabrik_disable_pole_vector(FABRIKChain *chain) {
  if (chain)
    chain->use_pole_vector = false;
}

// -----------------------------------------------------------------------------
// Query Results
// -----------------------------------------------------------------------------

void fabrik_get_joint_position(FABRIKChain *chain, uint32_t joint_index,
                               float *out) {
  if (!chain || !out || joint_index > chain->bone_count)
    return;

  out[0] = chain->joint_positions[joint_index].x;
  out[1] = chain->joint_positions[joint_index].y;
  out[2] = chain->joint_positions[joint_index].z;
}

bool fabrik_reached_target(FABRIKChain *chain) {
  return chain ? chain->reached_target : false;
}

float fabrik_get_error(FABRIKChain *chain) {
  if (!chain)
    return 0.0f;
  return fabrik_vec3_distance(chain->joint_positions[chain->bone_count],
                              chain->target_position);
}

// -----------------------------------------------------------------------------
// Two-Bone IK (Analytical Solution)
// -----------------------------------------------------------------------------

bool fabrik_solve_two_bone(float *root, float *mid, float *end, float *target,
                           float *pole, float length_upper,
                           float length_lower) {
  FabrikVec3 v_root = fabrik_vec3(root[0], root[1], root[2]);
  FabrikVec3 v_target = fabrik_vec3(target[0], target[1], target[2]);
  FabrikVec3 v_pole = fabrik_vec3(pole[0], pole[1], pole[2]);

  float target_dist = fabrik_vec3_distance(v_root, v_target);
  float total_length = length_upper + length_lower;

  // Clamp target distance
  if (target_dist > total_length * 0.9999f) {
    target_dist = total_length * 0.9999f;
  }
  if (target_dist < fabsf(length_upper - length_lower) * 1.0001f) {
    target_dist = fabsf(length_upper - length_lower) * 1.0001f;
  }

  // Law of cosines to find elbow angle
  float cos_angle = (length_upper * length_upper + target_dist * target_dist -
                     length_lower * length_lower) /
                    (2.0f * length_upper * target_dist);
  cos_angle = fmaxf(-1.0f, fminf(1.0f, cos_angle));
  float angle = acosf(cos_angle);

  // Direction to target
  FabrikVec3 dir_to_target =
      fabrik_vec3_normalize(fabrik_vec3_sub(v_target, v_root));

  // Create bend direction from pole vector
  FabrikVec3 pole_dir = fabrik_vec3_sub(v_pole, v_root);
  float pole_along = fabrik_vec3_dot(pole_dir, dir_to_target);
  FabrikVec3 pole_perp =
      fabrik_vec3_sub(pole_dir, fabrik_vec3_scale(dir_to_target, pole_along));
  pole_perp = fabrik_vec3_normalize(pole_perp);

  // Calculate mid position
  float mid_along = length_upper * cosf(angle);
  float mid_perp = length_upper * sinf(angle);

  FabrikVec3 v_mid =
      fabrik_vec3_add(v_root, fabrik_vec3_scale(dir_to_target, mid_along));
  v_mid = fabrik_vec3_add(v_mid, fabrik_vec3_scale(pole_perp, mid_perp));

  // Output
  mid[0] = v_mid.x;
  mid[1] = v_mid.y;
  mid[2] = v_mid.z;

  end[0] = target[0];
  end[1] = target[1];
  end[2] = target[2];

  return true;
}
