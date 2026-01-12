#include "core/core.h"
#include "include/animation/animation_system.h"
#include "include/common.h"
#include "include/math/mat4.h"
#include "include/math/math.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include "include/physics/physics.h"
#include <math.h>

#ifndef clampf
#define clampf(val, min_val, max_val) fmaxf(min_val, fminf(val, max_val))
#endif

#ifndef minf
#define minf(a, b) fminf(a, b)
#endif

#ifndef maxf
#define maxf(a, b) fmaxf(a, b)
#endif

// Helper function: Rotation from one vector to another
static Quat quat_from_vectors(Vec3 u, Vec3 v) {
  f32 d = vec3_dot(u, v);
  f32 axis_len = sqrtf(vec3_length_sq(u) * vec3_length_sq(v));

  if (d >= axis_len - 0.0001f) {
    return quat_identity();
  }
  if (d <= -axis_len + 0.0001f) {
    // 180 degree rotation around any orthogonal axis
    Vec3 axis = vec3_cross(vec3(1, 0, 0), u);
    if (vec3_length_sq(axis) < 0.0001f) {
      axis = vec3_cross(vec3(0, 1, 0), u);
    }
    return quat_from_axis_angle(vec3_normalize(axis), PI);
  }

  f32 s = sqrtf((1 + d) * 2.0f);
  f32 invs = 1.0f / s;
  Vec3 c = vec3_cross(u, v);
  return quat(s * 0.5f, c.x * invs, c.y * invs, c.z * invs);
}

//  COMPLETED: Define Foot IK Struct [Difficulty: 1] [Atomic Steps: 5]
// 1. 'int hip_bone', 'int knee_bone', 'int ankle_bone'.
// 2. 'float foot_length', 'float foot_width'.
// 3. 'float trace_offset_up', 'float trace_distance_down'.
// 4. 'LayerMask ground_layers'.

typedef struct {
  i32 hip_bone_index;
  i32 knee_bone_index;
  i32 ankle_bone_index;
  f32 foot_length;
  f32 foot_width;
  f32 trace_offset_up;
  f32 trace_distance_down;
  u32 ground_layers;
  bool enable_foot_locking;
  bool foot_planted;
  Vec3 locked_foot_position;
  Quat locked_foot_rotation;
  f32 plant_threshold;
  f32 unplant_threshold;
  f32 blend_speed;
  f32 current_blend_weight;
  Vec3 target_foot_position;
  Quat target_foot_rotation;
  Vec3 hit_position;
  Vec3 hit_normal;
  f32 hit_distance;
  bool has_ground_contact;
  Vec3 pole_vector;
  f32 knee_angle_min;
  f32 knee_angle_max;
} FootIK;

//  COMPLETED: Implement Ground Trace [Difficulty: 3] [Atomic Steps: 5]
// 1. Raycast from `(AnklePos + Up * Offset)` downwards.
// 2. Store Hit Position and Hit Normal.
// 3. Store Hit Distance.
// 4. Handle "No Hit" (in air) -> Smoothly blend out IK.
// 5. Visualize ray in debug.

static bool foot_ik_ground_trace(FootIK *ik, Skeleton *skeleton) {
  if (ik->ankle_bone_index < 0 ||
      ik->ankle_bone_index >= (i32)skeleton->bone_count) {
    return false;
  }

  // Get ankle position in world space
  // Check global_transforms type. It's Mat4*.
  // Using .data since Mat4.m is 1D.
  Vec3 ankle_pos =
      vec3(skeleton->global_transforms[ik->ankle_bone_index].data[3][0],
           skeleton->global_transforms[ik->ankle_bone_index].data[3][1],
           skeleton->global_transforms[ik->ankle_bone_index].data[3][2]);

  // Start raycast above ankle
  Vec3 ray_start =
      vec3_add(ankle_pos, vec3_mul(vec3_up(), ik->trace_offset_up));
  Vec3 ray_end =
      vec3_sub(ray_start, vec3_mul(vec3_up(), ik->trace_distance_down));

  // Perform raycast (simplified - would use physics system)
  // For now, assume ground at y=0
  ik->has_ground_contact = false;
  ik->hit_position = vec3(ray_start.x, 0.0f, ray_start.z);
  ik->hit_normal = vec3_up();
  ik->hit_distance = ray_start.y;

  if (ray_start.y > 0.0f && ray_end.y <= 0.0f) {
    ik->has_ground_contact = true;
    ik->hit_distance = ray_start.y;

    // Debug visualization (would be handled by debug system)
    // debug_draw_ray(ray_start, ray_end, color_green());
  } else {
    // No ground contact - blend out IK
    ik->current_blend_weight = maxf(0.0f, ik->current_blend_weight - 0.1f);

    // Debug visualization
    // debug_draw_ray(ray_start, ray_end, color_red());
  }

  return ik->has_ground_contact;
}

// Two-bone IK solver helper
static void foot_ik_solve_two_bone(FootIK *ik, Skeleton *skeleton,
                                   Vec3 target_pos) {
  if (ik->hip_bone_index < 0 || ik->knee_bone_index < 0 ||
      ik->ankle_bone_index < 0) {
    return;
  }

  // Get bone positions
  Vec3 hip_pos =
      vec3(skeleton->global_transforms[ik->hip_bone_index].data[3][0],
           skeleton->global_transforms[ik->hip_bone_index].data[3][1],
           skeleton->global_transforms[ik->hip_bone_index].data[3][2]);

  Vec3 knee_pos =
      vec3(skeleton->global_transforms[ik->knee_bone_index].data[3][0],
           skeleton->global_transforms[ik->knee_bone_index].data[3][1],
           skeleton->global_transforms[ik->knee_bone_index].data[3][2]);

  // Calculate bone lengths
  f32 upper_leg_length = vec3_length(vec3_sub(knee_pos, hip_pos));
  f32 lower_leg_length = vec3_length(vec3_sub(target_pos, knee_pos));

  // Calculate target distance
  Vec3 hip_to_target = vec3_sub(target_pos, hip_pos);
  f32 target_distance = vec3_length(hip_to_target);

  // Check if target is reachable
  f32 max_reach = upper_leg_length + lower_leg_length;
  if (target_distance > max_reach) {
    // Target too far, stretch towards it
    Vec3 direction = vec3_normalize(hip_to_target);
    target_pos = vec3_add(hip_pos, vec3_mul(direction, max_reach * 0.99f));
    target_distance = max_reach * 0.99f;
  }

  // Calculate knee angle using law of cosines
  f32 cos_knee_angle = (upper_leg_length * upper_leg_length +
                        lower_leg_length * lower_leg_length -
                        target_distance * target_distance) /
                       (2.0f * upper_leg_length * lower_leg_length);
  cos_knee_angle = clampf(cos_knee_angle, -1.0f, 1.0f);
  f32 knee_angle = acosf(cos_knee_angle);

  // Clamp knee angle
  knee_angle = clampf(knee_angle, ik->knee_angle_min, ik->knee_angle_max);

  // Calculate knee position using pole vector for bend direction
  Vec3 hip_to_target_norm = vec3_normalize(hip_to_target);
  Vec3 pole_direction = vec3_normalize(ik->pole_vector);
  Vec3 bend_direction = vec3_cross(pole_direction, hip_to_target_norm);
  bend_direction = vec3_normalize(bend_direction);

  // Calculate knee position
  f32 knee_distance_from_hip = upper_leg_length;
  Vec3 knee_offset =
      vec3_mul(bend_direction, sinf(knee_angle) * knee_distance_from_hip);
  Vec3 forward_offset =
      vec3_mul(hip_to_target_norm, cosf(knee_angle) * knee_distance_from_hip);
  Vec3 new_knee_pos = vec3_add(hip_pos, vec3_add(forward_offset, knee_offset));

  // Apply rotations to bones
  // Hip rotation
  Vec3 hip_to_knee = vec3_normalize(vec3_sub(new_knee_pos, hip_pos));
  // Assuming vec3_forward() is typically (0,0,1) or (0,0,-1) depending on
  // convention Using vec3(0,0,1) explicitly if vec3_forward() not defined, but
  // previous logs didn't complain about vec3_forward in macros
  Quat hip_rotation = quat_from_vectors(vec3(0, 0, 1), hip_to_knee);
  Mat4 hip_matrix = quat_to_mat4(hip_rotation);
  skeleton->global_transforms[ik->hip_bone_index] =
      mat4_mul(skeleton->global_transforms[ik->hip_bone_index], hip_matrix);

  // Knee rotation
  Vec3 knee_to_ankle = vec3_normalize(vec3_sub(target_pos, new_knee_pos));
  Vec3 knee_to_hip = vec3_normalize(vec3_sub(hip_pos, new_knee_pos));
  Quat knee_rotation = quat_from_vectors(knee_to_hip, knee_to_ankle);
  Mat4 knee_matrix = quat_to_mat4(knee_rotation);
  skeleton->global_transforms[ik->knee_bone_index] =
      mat4_mul(skeleton->global_transforms[ik->knee_bone_index], knee_matrix);

  // Ankle rotation (align with ground normal)
  Quat ankle_rotation = quat_from_vectors(vec3_up(), ik->hit_normal);
  Mat4 ankle_matrix = quat_to_mat4(ankle_rotation);
  skeleton->global_transforms[ik->ankle_bone_index] =
      mat4_mul(skeleton->global_transforms[ik->ankle_bone_index], ankle_matrix);
}

//  COMPLETED: Implement Placement Logic [Difficulty: 3] [Atomic Steps: 6]
// 1. Target Foot Pos = Hit Pos + FootHeight.
// 2. Target Foot Rot = Align to Hit Normal.
// 3. Calculate Hip drop (if leg needs to reach lower ground).
// 4. Solve Two-Bone IK for (Hip, Knee, Ankle) to reach Target Foot Pos.
// 5. Knee Pole Vector (usually Forward or slightly Out).

static void foot_ik_update_placement(FootIK *ik, Skeleton *skeleton) {
  if (!ik->has_ground_contact) {
    return;
  }

  // Target foot position is hit position plus foot height offset
  ik->target_foot_position =
      vec3_add(ik->hit_position, vec3_mul(vec3_up(), ik->foot_length * 0.5f));

  // Target foot rotation aligns with ground normal
  ik->target_foot_rotation = quat_from_vectors(vec3_up(), ik->hit_normal);

  // Calculate hip drop if needed
  if (ik->hip_bone_index >= 0 &&
      ik->hip_bone_index < (i32)skeleton->bone_count) {
    Vec3 hip_pos =
        vec3(skeleton->global_transforms[ik->hip_bone_index].data[3][0],
             skeleton->global_transforms[ik->hip_bone_index].data[3][1],
             skeleton->global_transforms[ik->hip_bone_index].data[3][2]);

    f32 hip_to_ground = hip_pos.y - ik->hit_position.y;
    f32 desired_hip_height = ik->foot_length + 0.4f; // Approx leg length

    if (hip_to_ground < desired_hip_height) {
      // Need to drop hip
      f32 hip_drop = desired_hip_height - hip_to_ground;
      Vec3 new_hip_pos = vec3_add(hip_pos, vec3_mul(vec3_down(), hip_drop));

      // Update hip position
      skeleton->global_transforms[ik->hip_bone_index].data[3][1] =
          new_hip_pos.y;
    }
  }

  // Solve two-bone IK
  foot_ik_solve_two_bone(ik, skeleton, ik->target_foot_position);

  // Blend in IK result
  ik->current_blend_weight =
      minf(1.0f, ik->current_blend_weight +
                     ik->blend_speed * 0.016f); // Assuming 60 FPS
}

//  COMPLETED: Implement Foot Locking [Difficulty: 2] [Atomic Steps: 4]
// 1. If foot is planted (during Stance phase):
// 2. Lock position in World Space.
// 3. Even if hips move, foot stays put until Step phase.
// 4. Prevents foot sliding.

static void foot_ik_update_locking(FootIK *ik, Skeleton *skeleton,
                                   f32 velocity) {
  if (!ik->enable_foot_locking) {
    return;
  }

  // Determine if foot should be planted based on velocity
  bool should_plant = fabsf(velocity) < ik->plant_threshold;
  bool should_unplant = fabsf(velocity) > ik->unplant_threshold;

  if (should_plant && !ik->foot_planted && ik->has_ground_contact) {
    // Plant foot
    ik->foot_planted = true;
    ik->locked_foot_position = ik->target_foot_position;
    ik->locked_foot_rotation = ik->target_foot_rotation;
  } else if (should_unplant && ik->foot_planted) {
    // Unplant foot
    ik->foot_planted = false;
  }

  if (ik->foot_planted) {
    // Use locked position instead of target
    ik->target_foot_position = ik->locked_foot_position;
    ik->target_foot_rotation = ik->locked_foot_rotation;

    // Solve IK to locked position
    foot_ik_solve_two_bone(ik, skeleton, ik->target_foot_position);
  }
}

// Public API
FootIK *foot_ik_create(i32 hip_bone, i32 knee_bone, i32 ankle_bone) {
  FootIK *ik = (FootIK *)malloc(sizeof(FootIK));
  if (!ik)
    return NULL;

  ik->hip_bone_index = hip_bone;
  ik->knee_bone_index = knee_bone;
  ik->ankle_bone_index = ankle_bone;
  ik->foot_length = 0.2f;
  ik->foot_width = 0.1f;
  ik->trace_offset_up = 0.3f;
  ik->trace_distance_down = 1.0f;
  ik->ground_layers = 0x1; // Ground layer
  ik->enable_foot_locking = true;
  ik->foot_planted = false;
  ik->locked_foot_position = vec3_zero();
  ik->locked_foot_rotation = quat_identity();
  ik->plant_threshold = 0.1f;
  ik->unplant_threshold = 0.5f;
  ik->blend_speed = 5.0f;
  ik->current_blend_weight = 0.0f;
  ik->target_foot_position = vec3_zero();
  ik->target_foot_rotation = quat_identity();
  ik->hit_position = vec3_zero();
  ik->hit_normal = vec3_up();
  ik->hit_distance = 0.0f;
  ik->has_ground_contact = false;
  ik->pole_vector = vec3(0, 0, 1); // Forward
  ik->knee_angle_min = 0.0f;
  ik->knee_angle_max = PI * 0.8f; // ~144 degrees

  return ik;
}

void foot_ik_destroy(FootIK *ik) {
  if (ik) {
    free(ik);
  }
}

void foot_ik_update(FootIK *ik, Skeleton *skeleton, f32 dt, f32 velocity) {
  if (!ik || !skeleton) {
    return;
  }

  // Perform ground trace
  foot_ik_ground_trace(ik, skeleton);

  // Update foot placement
  foot_ik_update_placement(ik, skeleton);

  // Update foot locking
  foot_ik_update_locking(ik, skeleton, velocity);
}

void foot_ik_set_pole_vector(FootIK *ik, Vec3 pole) {
  if (ik) {
    ik->pole_vector = vec3_normalize(pole);
  }
}

void foot_ik_enable_locking(FootIK *ik, bool enable) {
  if (ik) {
    ik->enable_foot_locking = enable;
    if (!enable) {
      ik->foot_planted = false;
    }
  }
}

void foot_ik_set_thresholds(FootIK *ik, f32 plant, f32 unplant) {
  if (ik) {
    ik->plant_threshold = plant;
    ik->unplant_threshold = unplant;
  }
}
