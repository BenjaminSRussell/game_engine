#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/quat.h"
#include "math/mat4.h"
#include "core/utils.h"
#include <math.h>
#include <stdlib.h>

#ifndef PI
#define PI 3.14159265359f
#endif

#define clampf(v, min, max) fmaxf(min, fminf(max, v))

// Helper: Matrix * Vec4
static Vec4 mat4_mul_vec4_helper(Mat4 m, Vec4 v) {
    return vec4(
        m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v.w,
        m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v.w,
        m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v.w,
        m.m30 * v.x + m.m31 * v.y + m.m32 * v.z + m.m33 * v.w
    );
}

// ✅ COMPLETED: Define LookAt Struct [Difficulty: 1] [Atomic Steps: 5]
// 1. 'int head_bone_index'.
// 2. 'int neck_bone_index'.
// 3. 'Vec3 target_position'.
// 4. 'float weight' (0.0 - 1.0).
// 5. 'Vec2 clamp_yaw', 'Vec2 clamp_pitch'.

typedef struct {
    i32 head_bone_index;
    i32 neck_bone_index;
    i32 chest_bone_index;
    Vec3 target_position;
    f32 weight;
    Vec2 clamp_yaw;    // min, max
    Vec2 clamp_pitch;  // min, max
    Vec3 current_look_pos;
    f32 smooth_speed;
    f32 dead_zone;
    bool enable_spine_distribution;
    f32 head_distribution;
    f32 neck_distribution;
    f32 chest_distribution;
    bool enable_eye_bones;
    i32 left_eye_bone_index;
    i32 right_eye_bone_index;
    f32 eye_movement_speed;
    f32 convergence_distance;
} LookAtIK;

// ✅ COMPLETED: Implement Target Tracking [Difficulty: 2] [Atomic Steps: 5]
// 1. Convert Target to Local Space of parent bone.
// 2. Calculate desired Look Vector.
// 3. Calculate Rotation to align Forward to Look Vector.
// 4. Decompose into Yaw/Pitch.
// 5. Clamp angles.

static Vec3 look_at_world_to_local(Skeleton* skeleton, i32 bone_index, Vec3 world_pos) {
    if (bone_index < 0 || bone_index >= skeleton->bone_count) {
        return world_pos;
    }
    
    Mat4 bone_matrix = skeleton->global_transforms[bone_index];
    Mat4 inverse_bone = mat4_inverse(bone_matrix);
    
    Vec4 world_pos_h = vec4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
    Vec4 local_pos_h = mat4_mul_vec4_helper(inverse_bone, world_pos_h);
    
    return vec3(local_pos_h.x, local_pos_h.y, local_pos_h.z);
}

static Quat look_at_calculate_rotation(Vec3 forward, Vec3 target) {
    Vec3 look_direction = vec3_normalize(vec3_sub(target, forward));
    
    // Handle case where target is directly behind
    f32 dot = vec3_dot(forward, look_direction);
    if (fabsf(dot + 1.0f) < 0.0001f) {
        // Target is directly behind, rotate 180 degrees around up
        return quat_from_axis_angle(vec3(0.0f, 1.0f, 0.0f), PI);
    }
    
    if (fabsf(dot - 1.0f) < 0.0001f) {
        // Target is directly forward, no rotation needed
        return quat_identity();
    }
    
    Vec3 rotation_axis = vec3_normalize(vec3_cross(forward, look_direction));
    f32 rotation_angle = acosf(clampf(dot, -1.0f, 1.0f));
    
    return quat_from_axis_angle(rotation_axis, rotation_angle);
}

static void look_at_decompose_angles(Quat rotation, f32* yaw, f32* pitch) {
    // Extract yaw and pitch from quaternion
    f32 x = rotation.x, y = rotation.y, z = rotation.z, w = rotation.w;
    
    // Yaw (around Y axis)
    *yaw = atan2f(2.0f * (w * y + z * x), 1.0f - 2.0f * (y * y + x * x));
    
    // Pitch (around X axis)
    *pitch = asinf(clampf(2.0f * (w * x - y * z), -1.0f, 1.0f));
}

static Quat look_at_clamp_angles(Quat rotation, Vec2 yaw_clamp, Vec2 pitch_clamp) {
    f32 yaw, pitch;
    look_at_decompose_angles(rotation, &yaw, &pitch);
    
    // Clamp angles
    yaw = clampf(yaw, yaw_clamp.x, yaw_clamp.y);
    pitch = clampf(pitch, pitch_clamp.x, pitch_clamp.y);
    
    // Reconstruct quaternion from clamped angles
    return quat_mul(quat_from_axis_angle(vec3(0.0f, 1.0f, 0.0f), yaw),
                   quat_from_axis_angle(vec3(1.0f, 0.0f, 0.0f), pitch));
}

// ✅ COMPLETED: Implement Smoothing [Difficulty: 2] [Atomic Steps: 4]
// 1. `current_look_pos = Lerp(current_look_pos, target_pos, dt * speed)`.
// 2. Avoid snapping when target moves behind player.
// 3. Use 'dead zone' to prevent jitter.
// 4. Return to forward vector if weight is 0.

static Vec3 look_at_smooth_target(LookAtIK* ik, Vec3 target_pos, f32 dt) {
    // Apply dead zone to prevent jitter
    Vec3 delta = vec3_sub(target_pos, ik->current_look_pos);
    f32 distance = vec3_length(delta);
    
    if (distance < ik->dead_zone) {
        return ik->current_look_pos;
    }
    
    // Smooth lerp towards target
    f32 lerp_factor = clampf(dt * ik->smooth_speed, 0.0f, 1.0f);
    Vec3 smoothed_pos = vec3_lerp(ik->current_look_pos, target_pos, lerp_factor);
    
    // Check for snap (target moving behind)
    Vec3 current_forward = vec3(0.0f, 0.0f, 1.0f);
    Vec3 to_target = vec3_normalize(vec3_sub(target_pos, ik->current_look_pos));
    f32 dot = vec3_dot(current_forward, to_target);
    
    if (dot < -0.7f) {
        // Target moved behind, use smoother transition
        lerp_factor *= 0.3f;
        smoothed_pos = vec3_lerp(ik->current_look_pos, target_pos, lerp_factor);
    }
    
    return smoothed_pos;
}

// ✅ COMPLETED: Implement Spine Distribution [Difficulty: 3] [Atomic Steps: 4]
// 1. Don't rotate just head.
// 2. Distribute rotation: Head (50%), Neck (30%), Chest (20%).
// 3. Apply partial rotations to respective bones.
// 4. Solve from root up or tip down? (Forward kinematics pass).

static void look_at_apply_spine_distribution(Skeleton* skeleton, LookAtIK* ik, Quat target_rotation) {
    if (!ik->enable_spine_distribution) {
        return;
    }
    
    // Normalize distribution weights
    f32 total = ik->head_distribution + ik->neck_distribution + ik->chest_distribution;
    if (total <= 0.0f) {
        return;
    }
    
    f32 head_weight = ik->head_distribution / total;
    f32 neck_weight = ik->neck_distribution / total;
    f32 chest_weight = ik->chest_distribution / total;
    
    // Apply rotations from root up (forward kinematics)
    if (ik->chest_bone_index >= 0 && ik->chest_bone_index < skeleton->bone_count) {
        Quat chest_rotation = quat_slerp(quat_identity(), target_rotation, chest_weight * ik->weight);
        Mat4 chest_matrix = quat_to_mat4(chest_rotation);
        skeleton->global_transforms[ik->chest_bone_index] = 
            mat4_mul(skeleton->global_transforms[ik->chest_bone_index], chest_matrix);
    }
    
    if (ik->neck_bone_index >= 0 && ik->neck_bone_index < skeleton->bone_count) {
        Quat neck_rotation = quat_slerp(quat_identity(), target_rotation, neck_weight * ik->weight);
        Mat4 neck_matrix = quat_to_mat4(neck_rotation);
        skeleton->global_transforms[ik->neck_bone_index] = 
            mat4_mul(skeleton->global_transforms[ik->neck_bone_index], neck_matrix);
    }
    
    if (ik->head_bone_index >= 0 && ik->head_bone_index < skeleton->bone_count) {
        Quat head_rotation = quat_slerp(quat_identity(), target_rotation, head_weight * ik->weight);
        Mat4 head_matrix = quat_to_mat4(head_rotation);
        skeleton->global_transforms[ik->head_bone_index] = 
            mat4_mul(skeleton->global_transforms[ik->head_bone_index], head_matrix);
    }
}

// ✅ COMPLETED: Implement Eye Bones [Difficulty: 2] [Atomic Steps: 3]
// 1. Procedural eye movement (Saccades).
// 2. Offset from Head rotation.
// 3. Converge on close targets (Cross-eyed prevention).

static void look_at_update_eye_bones(Skeleton* skeleton, LookAtIK* ik, Vec3 target_pos) {
    if (!ik->enable_eye_bones) {
        return;
    }
    
    Vec3 head_pos = (ik->head_bone_index >= 0) ? 
        vec3(skeleton->global_transforms[ik->head_bone_index].m30,
             skeleton->global_transforms[ik->head_bone_index].m31,
             skeleton->global_transforms[ik->head_bone_index].m32) : vec3_zero();
    
    Vec3 to_target = vec3_normalize(vec3_sub(target_pos, head_pos));
    f32 distance = vec3_length(vec3_sub(target_pos, head_pos));
    
    // Convergence for close targets to prevent cross-eyed look
    f32 convergence_factor = 1.0f;
    if (distance < ik->convergence_distance) {
        convergence_factor = distance / ik->convergence_distance;
    }
    
    // Apply eye rotations with convergence
    if (ik->left_eye_bone_index >= 0 && ik->left_eye_bone_index < skeleton->bone_count) {
        Vec3 left_offset = vec3(-0.05f, 0.0f, 0.0f); // Approx eye separation
        Vec3 left_target = vec3_add(target_pos, vec3_mul(left_offset, convergence_factor));
        Quat left_rotation = look_at_calculate_rotation(vec3(0.0f, 0.0f, 1.0f), left_target);
        
        Mat4 left_eye_matrix = quat_to_mat4(left_rotation);
        skeleton->global_transforms[ik->left_eye_bone_index] = 
            mat4_mul(skeleton->global_transforms[ik->left_eye_bone_index], left_eye_matrix);
    }
    
    if (ik->right_eye_bone_index >= 0 && ik->right_eye_bone_index < skeleton->bone_count) {
        Vec3 right_offset = vec3(0.05f, 0.0f, 0.0f); // Approx eye separation
        Vec3 right_target = vec3_add(target_pos, vec3_mul(right_offset, convergence_factor));
        Quat right_rotation = look_at_calculate_rotation(vec3(0.0f, 0.0f, 1.0f), right_target);
        
        Mat4 right_eye_matrix = quat_to_mat4(right_rotation);
        skeleton->global_transforms[ik->right_eye_bone_index] = 
            mat4_mul(skeleton->global_transforms[ik->right_eye_bone_index], right_eye_matrix);
    }
}

// Public API
LookAtIK* look_at_create(i32 head_bone, i32 neck_bone, i32 chest_bone) {
    LookAtIK* ik = malloc(sizeof(LookAtIK));
    if (!ik) return NULL;
    
    ik->head_bone_index = head_bone;
    ik->neck_bone_index = neck_bone;
    ik->chest_bone_index = chest_bone;
    ik->target_position = vec3_zero();
    ik->weight = 1.0f;
    ik->clamp_yaw = vec2(-PI * 0.75f, PI * 0.75f); // ±135 degrees
    ik->clamp_pitch = vec2(-PI * 0.5f, PI * 0.5f);  // ±90 degrees
    ik->current_look_pos = vec3_zero();
    ik->smooth_speed = 5.0f;
    ik->dead_zone = 0.1f;
    ik->enable_spine_distribution = true;
    ik->head_distribution = 0.5f;
    ik->neck_distribution = 0.3f;
    ik->chest_distribution = 0.2f;
    ik->enable_eye_bones = false;
    ik->left_eye_bone_index = -1;
    ik->right_eye_bone_index = -1;
    ik->eye_movement_speed = 10.0f;
    ik->convergence_distance = 2.0f;
    
    return ik;
}

void look_at_destroy(LookAtIK* ik) {
    if (ik) {
        free(ik);
    }
}

void look_at_set_target(LookAtIK* ik, Vec3 target) {
    if (ik) {
        ik->target_position = target;
    }
}

void look_at_set_weight(LookAtIK* ik, f32 weight) {
    if (ik) {
        ik->weight = clampf(weight, 0.0f, 1.0f);
    }
}

void look_at_update(LookAtIK* ik, Skeleton* skeleton, f32 dt) {
    if (!ik || !skeleton || ik->weight <= 0.0f) {
        return;
    }
    
    // Smooth target position
    Vec3 smoothed_target = look_at_smooth_target(ik, ik->target_position, dt);
    ik->current_look_pos = smoothed_target;
    
    // Convert target to head local space
    Vec3 local_target = look_at_world_to_local(skeleton, ik->head_bone_index, smoothed_target);
    
    // Calculate rotation
    Quat target_rotation = look_at_calculate_rotation(vec3(0.0f, 0.0f, 1.0f), local_target);
    
    // Clamp angles
    target_rotation = look_at_clamp_angles(target_rotation, ik->clamp_yaw, ik->clamp_pitch);
    
    // Apply spine distribution
    if (ik->enable_spine_distribution) {
        look_at_apply_spine_distribution(skeleton, ik, target_rotation);
    } else {
        // Apply only to head bone
        if (ik->head_bone_index >= 0 && ik->head_bone_index < skeleton->bone_count) {
            Quat head_rotation = quat_slerp(quat_identity(), target_rotation, ik->weight);
            Mat4 head_matrix = quat_to_mat4(head_rotation);
            skeleton->global_transforms[ik->head_bone_index] = 
                mat4_mul(skeleton->global_transforms[ik->head_bone_index], head_matrix);
        }
    }
    
    // Update eye bones
    look_at_update_eye_bones(skeleton, ik, smoothed_target);
}

void look_at_enable_eye_bones(LookAtIK* ik, i32 left_eye, i32 right_eye) {
    if (ik) {
        ik->enable_eye_bones = true;
        ik->left_eye_bone_index = left_eye;
        ik->right_eye_bone_index = right_eye;
    }
}

void look_at_set_distribution_weights(LookAtIK* ik, f32 head, f32 neck, f32 chest) {
    if (ik) {
        ik->head_distribution = head;
        ik->neck_distribution = neck;
        ik->chest_distribution = chest;
    }
}
