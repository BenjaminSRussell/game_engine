#include "include/animation/animation_system.h"
#include <common.h>
#include "include/math/mat4.h"
#include "include/math/math_all.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_BONES
#define MAX_BONES 100
#endif

// Helper: Quat from Matrix
static Quat quat_from_mat4(Mat4 m) {
  f32 trace = m.data[0][0] + m.data[1][1] + m.data[2][2];
  if (trace > 0.0f) {
    f32 s = 0.5f / sqrtf(trace + 1.0f);
    return quat(0.25f / s, (m.data[2][1] - m.data[1][2]) * s,
                (m.data[0][2] - m.data[2][0]) * s,
                (m.data[1][0] - m.data[0][1]) * s);
  } else {
    if (m.data[0][0] > m.data[1][1] && m.data[0][0] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[0][0] - m.data[1][1] - m.data[2][2]);
      return quat((m.data[2][1] - m.data[1][2]) / s, 0.25f * s,
                  (m.data[0][1] + m.data[1][0]) / s,
                  (m.data[0][2] + m.data[2][0]) / s);
    } else if (m.data[1][1] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[1][1] - m.data[0][0] - m.data[2][2]);
      return quat((m.data[0][2] - m.data[2][0]) / s,
                  (m.data[0][1] + m.data[1][0]) / s, 0.25f * s,
                  (m.data[1][2] + m.data[2][1]) / s);
    } else {
      f32 s = 2.0f * sqrtf(1.0f + m.data[2][2] - m.data[0][0] - m.data[1][1]);
      return quat((m.data[1][0] - m.data[0][1]) / s,
                  (m.data[0][2] + m.data[2][0]) / s,
                  (m.data[1][2] + m.data[2][1]) / s, 0.25f * s);
    }
  }
}

// Helper: Get Translation from Mat4
static Vec3 mat4_get_translation(Mat4 m) {
  return vec3(m.data[3][0], m.data[3][1], m.data[3][2]);
}

// Helper: Get Scale from Mat4
static Vec3 mat4_get_scale(Mat4 m) {
  Vec3 scale;
  scale.x = vec3_length(vec3(m.data[0][0], m.data[0][1], m.data[0][2]));
  scale.y = vec3_length(vec3(m.data[1][0], m.data[1][1], m.data[1][2]));
  scale.z = vec3_length(vec3(m.data[2][0], m.data[2][1], m.data[2][2]));
  return scale;
}

// Helper: Get Rotation from Mat4
static Quat mat4_get_rotation(Mat4 m) {
  Vec3 scale = mat4_get_scale(m);
  Mat4 unscaled = m;
  if (scale.x > 0.0001f) {
    unscaled.data[0][0] /= scale.x;
    unscaled.data[0][1] /= scale.x;
    unscaled.data[0][2] /= scale.x;
  }
  if (scale.y > 0.0001f) {
    unscaled.data[1][0] /= scale.y;
    unscaled.data[1][1] /= scale.y;
    unscaled.data[1][2] /= scale.y;
  }
  if (scale.z > 0.0001f) {
    unscaled.data[2][0] /= scale.z;
    unscaled.data[2][1] /= scale.z;
    unscaled.data[2][2] /= scale.z;
  }
  return quat_from_mat4(unscaled);
}

// Helper: Mat4 from Transform (T, R, S)
static Mat4 mat4_from_transform(Vec3 t, Quat r, Vec3 s) {
  Mat4 translation = mat4_translate(t);
  Mat4 rotation = quat_to_mat4(r);
  Mat4 scale = mat4_scale(s);

  // T * R * S
  Mat4 rs = mat4_mul(rotation, scale);
  return mat4_mul(translation, rs);
}

typedef enum {
  CORRECTION_TYPE_SPACE_MAINTENANCE,
  CORRECTION_TYPE_FOOT_PLANT,
  CORRECTION_TYPE_STRETCH,
  CORRECTION_TYPE_COLLISION
} CorrectionType;

typedef struct CorrectionRule {
  CorrectionType type;
  i32 bone_indices[4]; // Up to 4 bones for multi-bone constraints
  f32 parameters[4];   // Type-specific parameters
  bool is_active;
  f32 strength; // 0.0 to 1.0
} CorrectionRule;

typedef struct PoseCorrector {
  CorrectionRule rules[MAX_BONES];
  i32 rule_count;
  Skeleton *target_skeleton;
  bool is_enabled;
} PoseCorrector;

// COMPLETED: Define Correction Rules [Difficulty: 1] [Atomic Steps: 4]
PoseCorrector *pose_corrector_create(Skeleton *target_skeleton) {
  if (!target_skeleton)
    return NULL;

  PoseCorrector *corrector = (PoseCorrector *)malloc(sizeof(PoseCorrector));
  if (!corrector)
    return NULL;

  memset(corrector, 0, sizeof(PoseCorrector));
  corrector->target_skeleton = target_skeleton;
  corrector->is_enabled = true;

  return corrector;
}

void pose_corrector_destroy(PoseCorrector *corrector) {
  if (corrector)
    free(corrector);
}

void pose_corrector_add_rule(PoseCorrector *corrector, CorrectionType type,
                             i32 *bone_indices, i32 bone_count, f32 *parameters,
                             f32 strength) {
  if (!corrector || !bone_indices || bone_count <= 0 || bone_count > 4)
    return;

  if (corrector->rule_count >= MAX_BONES)
    return;

  CorrectionRule *rule = &corrector->rules[corrector->rule_count++];
  rule->type = type;
  rule->strength = fmaxf(0.0f, fminf(1.0f, strength));
  rule->is_active = true;

  for (i32 i = 0; i < bone_count; i++) {
    rule->bone_indices[i] = bone_indices[i];
    rule->parameters[i] = parameters[i];
  }
}

// COMPLETED: Implement Space Maintenance [Difficulty: 3] [Atomic Steps: 5]
static void apply_space_maintenance(PoseCorrector *corrector,
                                    CorrectionRule *rule) {
  Skeleton *skeleton = corrector->target_skeleton;

  i32 bone1 = rule->bone_indices[0];
  i32 bone2 = rule->bone_indices[1];

  if (bone1 < 0 || bone2 < 0 || bone1 >= (i32)skeleton->bone_count ||
      bone2 >= (i32)skeleton->bone_count)
    return;

  f32 target_distance = rule->parameters[0];
  f32 stiffness = rule->strength;

  Vec3 pos1 = mat4_get_translation(skeleton->global_transforms[bone1]);
  Vec3 pos2 = mat4_get_translation(skeleton->global_transforms[bone2]);

  Vec3 current_vec = vec3_sub(pos2, pos1);
  f32 current_distance = vec3_length(current_vec);

  if (current_distance < 0.001f)
    return; // Avoid division by zero

  f32 distance_error = target_distance - current_distance;
  f32 correction_factor =
      distance_error * stiffness * 0.5f; // Split correction between both bones

  Vec3 correction_dir = vec3_normalize(current_vec);
  Vec3 correction = vec3_mul(correction_dir, correction_factor);

  // Apply correction to both bones
  Mat4 transform1 = skeleton->global_transforms[bone1];
  Mat4 transform2 = skeleton->global_transforms[bone2];

  Vec3 new_pos1 = vec3_sub(pos1, correction);
  Vec3 new_pos2 = vec3_add(pos2, correction);

  // Update transforms
  Quat rot1 = mat4_get_rotation(transform1);
  Quat rot2 = mat4_get_rotation(transform2);
  Vec3 scale1 = mat4_get_scale(transform1);
  Vec3 scale2 = mat4_get_scale(transform2);

  skeleton->global_transforms[bone1] =
      mat4_from_transform(new_pos1, rot1, scale1);
  skeleton->global_transforms[bone2] =
      mat4_from_transform(new_pos2, rot2, scale2);
}

void pose_corrector_add_space_constraint(PoseCorrector *corrector, i32 bone1,
                                         i32 bone2, f32 target_distance,
                                         f32 strength) {
  i32 bones[2] = {bone1, bone2};
  f32 params[1] = {target_distance};
  pose_corrector_add_rule(corrector, CORRECTION_TYPE_SPACE_MAINTENANCE, bones,
                          2, params, strength);
}

// COMPLETED: Implement Foot Plant Fix [Difficulty: 2] [Atomic Steps: 4]
static void apply_foot_plant_fix(PoseCorrector *corrector,
                                 CorrectionRule *rule) {
  Skeleton *skeleton = corrector->target_skeleton;

  i32 foot_bone = rule->bone_indices[0];
  f32 ground_height = rule->parameters[0];
  f32 foot_radius = rule->parameters[1];

  if (foot_bone < 0 || foot_bone >= (i32)skeleton->bone_count)
    return;

  Mat4 foot_transform = skeleton->global_transforms[foot_bone];
  Vec3 foot_pos = mat4_get_translation(foot_transform);

  // Check if foot is below ground plane
  if (foot_pos.y < ground_height + foot_radius) {
    // Project foot onto ground plane
    Vec3 corrected_pos = foot_pos;
    corrected_pos.y = ground_height + foot_radius;

    // Apply correction with strength factor
    f32 correction_amount = rule->strength;
    corrected_pos = vec3_lerp(foot_pos, corrected_pos, correction_amount);

    // Update transform
    Quat foot_rot = mat4_get_rotation(foot_transform);
    Vec3 foot_scale = mat4_get_scale(foot_transform);

    skeleton->global_transforms[foot_bone] =
        mat4_from_transform(corrected_pos, foot_rot, foot_scale);
  }
}

void pose_corrector_add_foot_plant(PoseCorrector *corrector, i32 foot_bone,
                                   f32 ground_height, f32 foot_radius,
                                   f32 strength) {
  i32 bones[1] = {foot_bone};
  f32 params[2] = {ground_height, foot_radius};
  pose_corrector_add_rule(corrector, CORRECTION_TYPE_FOOT_PLANT, bones, 1,
                          params, strength);
}

void pose_corrector_update(PoseCorrector *corrector) {
  if (!corrector || !corrector->is_enabled || !corrector->target_skeleton)
    return;

  // Apply all correction rules in order
  for (i32 i = 0; i < corrector->rule_count; i++) {
    CorrectionRule *rule = &corrector->rules[i];
    if (!rule->is_active)
      continue;

    switch (rule->type) {
    case CORRECTION_TYPE_SPACE_MAINTENANCE:
      apply_space_maintenance(corrector, rule);
      break;

    case CORRECTION_TYPE_FOOT_PLANT:
      apply_foot_plant_fix(corrector, rule);
      break;

    case CORRECTION_TYPE_STRETCH:
      // TODO: Implement stretch correction
      break;

    case CORRECTION_TYPE_COLLISION:
      // TODO: Implement collision correction
      break;
    }
  }
}

void pose_corrector_enable(PoseCorrector *corrector, bool enabled) {
  if (corrector)
    corrector->is_enabled = enabled;
}

void pose_corrector_set_rule_strength(PoseCorrector *corrector, i32 rule_index,
                                      f32 strength) {
  if (!corrector || rule_index < 0 || rule_index >= corrector->rule_count)
    return;

  corrector->rules[rule_index].strength = fmaxf(0.0f, fminf(1.0f, strength));
}

void pose_corrector_remove_rule(PoseCorrector *corrector, i32 rule_index) {
  if (!corrector || rule_index < 0 || rule_index >= corrector->rule_count)
    return;

  // Shift remaining rules
  for (i32 i = rule_index; i < corrector->rule_count - 1; i++) {
    corrector->rules[i] = corrector->rules[i + 1];
  }

  corrector->rule_count--;
}

void pose_corrector_clear_rules(PoseCorrector *corrector) {
  if (corrector) {
    corrector->rule_count = 0;
  }
}

i32 pose_corrector_get_rule_count(PoseCorrector *corrector) {
  return corrector ? corrector->rule_count : 0;
}

CorrectionRule *pose_corrector_get_rule(PoseCorrector *corrector, i32 index) {
  if (!corrector || index < 0 || index >= corrector->rule_count)
    return NULL;
  return &corrector->rules[index];
}
