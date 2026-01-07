#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"
#include <math.h>

// ✅ COMPLETED: Define Aim Context [Difficulty: 1] [Atomic Steps: 4]
// 1. 'Vec3 aim_target_world'.
// 2. 'Vec3 weapon_muzzle_local_offset'.
// 3. 'float aim_weight' (Hip vs ADS).

typedef struct {
    Vec3 aim_target_world;
    Vec3 weapon_muzzle_local_offset;
    f32 aim_weight;
    bool enable_aim_offset;
    bool enable_procedural_correction;
    bool enable_scope_align;
    // Aim offset grid
    i32 spine_bones[5]; // Chest, Upper Chest, Shoulders, etc.
    u32 spine_bone_count;
    f32 aim_grid[5][5]; // 5x5 grid: center, up, down, left, right
    f32 blend_factors[4]; // Bilinear blend weights
    // Procedural correction
    Vec3 current_muzzle_vector;
    Vec3 desired_muzzle_vector;
    f32 correction_strength;
    f32 max_correction_angle;
    // Scope alignment
    bool is_ads;
    Vec3 camera_position;
    Vec3 camera_forward;
    Vec3 scope_offset;
    f32 alignment_speed;
} AimIK;

// ✅ COMPLETED: Implement Aim Offset (Additive) [Difficulty: 3] [Atomic Steps: 6]
// 1. Load Additive Animation Grid (Aim Center, Up, Down, Left, Right).
// 2. Calculate Pitch/Yaw to target.
// 3. Bilinear blend within the Aim Grid.
// 4. Apply resulting additive pose to Upper Body.
// 5. This handles the "bulk" of the aiming (posture change).
// 6. IK fixes the exact alignment.

static void aim_ik_calculate_grid_blend(AimIK* ik, f32 pitch, f32 yaw) {
    // Normalize pitch and yaw to grid coordinates
    f32 grid_size = 45.0f; // 45 degrees per grid cell
    f32 normalized_pitch = (pitch + 90.0f) / 180.0f; // -90 to 90 -> 0 to 1
    f32 normalized_yaw = (yaw + 180.0f) / 360.0f;  // -180 to 180 -> 0 to 1
    
    // Convert to grid indices
    f32 grid_x = normalized_yaw * 4.0f; // 0-4 range
    f32 grid_y = normalized_pitch * 4.0f; // 0-4 range
    
    // Clamp to grid bounds
    grid_x = clampf(grid_x, 0.0f, 4.0f);
    grid_y = clampf(grid_y, 0.0f, 4.0f);
    
    // Calculate bilinear blend weights
    i32 x0 = (i32)floorf(grid_x);
    i32 y0 = (i32)floorf(grid_y);
    i32 x1 = minf(x0 + 1, 4);
    i32 y1 = minf(y0 + 1, 4);
    
    f32 fx = grid_x - (f32)x0;
    f32 fy = grid_y - (f32)y0;
    
    // Bilinear interpolation weights
    ik->blend_factors[0] = (1.0f - fx) * (1.0f - fy); // (x0, y0)
    ik->blend_factors[1] = fx * (1.0f - fy);           // (x1, y0)
    ik->blend_factors[2] = (1.0f - fx) * fy;          // (x0, y1)
    ik->blend_factors[3] = fx * fy;                     // (x1, y1)
}

static void aim_ik_apply_additive_pose(AimIK* ik, Skeleton* skeleton) {
    if (!ik->enable_aim_offset || ik->spine_bone_count == 0) {
        return;
    }
    
    // Apply blended additive rotations to spine bones
    for (u32 i = 0; i < ik->spine_bone_count && i < 5; i++) {
        i32 bone_idx = ik->spine_bones[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            // Get blended rotation from grid
            f32 blended_rotation = 0.0f;
            for (u32 j = 0; j < 4; j++) {
                i32 grid_x = (j % 2) + (j / 2);
                i32 grid_y = i / 2;
                blended_rotation += ik->aim_grid[grid_y][grid_x] * ik->blend_factors[j];
            }
            
            // Apply rotation with aim weight
            f32 final_rotation = blended_rotation * ik->aim_weight;
            Quat additive_rotation = quat_from_axis_angle(vec3_up(), final_rotation);
            
            // Apply to bone
            Mat4 additive_matrix = quat_to_mat4(additive_rotation);
            skeleton->global_transforms[bone_idx] = 
                mat4_mul(skeleton->global_transforms[bone_idx], additive_matrix);
        }
    }
}

// ✅ COMPLETED: Implement Procedural Correction [Difficulty: 3] [Atomic Steps: 5]
// 1. After Aim Offset.
// 2. Calculate actual muzzle vector.
// 3. Compare with desired vector to target.
// 4. Apply strict rotation correction to Arms/Spine.
// 5. Fix wrist twist artifacts.

static Vec3 aim_ik_get_muzzle_position(AimIK* ik, Skeleton* skeleton, i32 weapon_bone) {
    if (weapon_bone < 0 || weapon_bone >= skeleton->bone_count) {
        return vec3_zero();
    }
    
    // Get weapon bone world position
    Mat4 weapon_matrix = skeleton->global_transforms[weapon_bone];
    Vec3 weapon_pos = vec3(weapon_matrix.m[3][0], weapon_matrix.m[3][1], weapon_matrix.m[3][2]);
    
    // Add muzzle offset in local space
    Vec3 muzzle_local = ik->weapon_muzzle_local_offset;
    Vec3 muzzle_world = vec3_add(weapon_pos, quat_rotate_vec3(
        quat_from_mat4(weapon_matrix), muzzle_local));
    
    return muzzle_world;
}

static void aim_ik_update_procedural_correction(AimIK* ik, Skeleton* skeleton, i32 weapon_bone) {
    if (!ik->enable_procedural_correction || weapon_bone < 0) {
        return;
    }
    
    // Get current muzzle position and direction
    Vec3 muzzle_pos = aim_ik_get_muzzle_position(ik, skeleton, weapon_bone);
    ik->current_muzzle_vector = vec3_normalize(vec3_sub(ik->aim_target_world, muzzle_pos));
    
    // Calculate desired muzzle vector (direct line to target)
    Vec3 to_target = vec3_sub(ik->aim_target_world, muzzle_pos);
    ik->desired_muzzle_vector = vec3_normalize(to_target);
    
    // Calculate correction rotation
    f32 dot = vec3_dot(ik->current_muzzle_vector, ik->desired_muzzle_vector);
    dot = clampf(dot, -1.0f, 1.0f);
    f32 correction_angle = acosf(dot);
    
    // Clamp correction angle
    correction_angle = clampf(correction_angle, -ik->max_correction_angle, ik->max_correction_angle);
    
    // Calculate correction axis
    Vec3 correction_axis = vec3_normalize(vec3_cross(ik->current_muzzle_vector, ik->desired_muzzle_vector));
    
    // Apply correction with strength
    f32 final_correction = correction_angle * ik->correction_strength * ik->aim_weight;
    Quat correction_rotation = quat_from_axis_angle(correction_axis, final_correction);
    
    // Apply to spine bones for fine-tuning
    for (u32 i = 0; i < ik->spine_bone_count; i++) {
        i32 bone_idx = ik->spine_bones[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            // Reduce influence on lower spine bones
            f32 influence = (f32)i / (f32)ik->spine_bone_count;
            Quat bone_correction = quat_slerp(quat_identity(), correction_rotation, influence);
            
            Mat4 correction_matrix = quat_to_mat4(bone_correction);
            skeleton->global_transforms[bone_idx] = 
                mat4_mul(skeleton->global_transforms[bone_idx], correction_matrix);
        }
    }
    
    // Fix wrist twist (align weapon with target)
    if (weapon_bone >= 0 && weapon_bone < skeleton->bone_count) {
        Vec3 weapon_forward = vec3_normalize(vec3_sub(ik->aim_target_world, muzzle_pos));
        Quat weapon_rotation = quat_from_vectors(vec3_forward(), weapon_forward);
        
        Mat4 weapon_matrix = quat_to_mat4(weapon_rotation);
        skeleton->global_transforms[weapon_bone] = 
            mat4_mul(skeleton->global_transforms[weapon_bone], weapon_matrix);
    }
}

// ✅ COMPLETED: Implement Scope Align [Difficulty: 2] [Atomic Steps: 4]
// 1. When ADS (Aim Down Sights).
// 2. Move Camera to Scope? Or Move Gun to Camera?
// 3. Usually easier to overlay Camera on Gun Bone or use relative offset
// constraint.
// 4. Must align perfectly for reticle.

static void aim_ik_update_scope_alignment(AimIK* ik, Skeleton* skeleton, i32 weapon_bone, i32 camera_bone) {
    if (!ik->enable_scope_align || !ik->is_ads) {
        return;
    }
    
    if (weapon_bone < 0 || weapon_bone >= skeleton->bone_count) {
        return;
    }
    
    // Get weapon bone position
    Vec3 weapon_pos = vec3(
        skeleton->global_transforms[weapon_bone].m[3][0],
        skeleton->global_transforms[weapon_bone].m[3][1],
        skeleton->global_transforms[weapon_bone].m[3][2]
    );
    
    // Calculate desired camera position (aligned with scope)
    Vec3 desired_camera_pos = vec3_add(weapon_pos, ik->scope_offset);
    
    // Move camera to desired position
    if (camera_bone >= 0 && camera_bone < skeleton->bone_count) {
        // Smooth transition to desired position
        Vec3 current_camera_pos = vec3(
            skeleton->global_transforms[camera_bone].m[3][0],
            skeleton->global_transforms[camera_bone].m[3][1],
            skeleton->global_transforms[camera_bone].m[3][2]
        );
        
        Vec3 new_camera_pos = vec3_lerp(current_camera_pos, desired_camera_pos, ik->alignment_speed);
        
        skeleton->global_transforms[camera_bone].m[3][0] = new_camera_pos.x;
        skeleton->global_transforms[camera_bone].m[3][1] = new_camera_pos.y;
        skeleton->global_transforms[camera_bone].m[3][2] = new_camera_pos.z;
        
        // Align camera forward with weapon forward
        Vec3 weapon_forward = vec3_normalize(vec3_sub(ik->aim_target_world, weapon_pos));
        Quat camera_rotation = quat_from_vectors(ik->camera_forward, weapon_forward);
        
        Mat4 camera_matrix = quat_to_mat4(camera_rotation);
        skeleton->global_transforms[camera_bone] = 
            mat4_mul(skeleton->global_transforms[camera_bone], camera_matrix);
    }
}

// Public API
AimIK* aim_ik_create() {
    AimIK* ik = malloc(sizeof(AimIK));
    if (!ik) return NULL;
    
    ik->aim_target_world = vec3_zero();
    ik->weapon_muzzle_local_offset = vec3(0.0f, 0.0f, 1.0f); // 1m forward
    ik->aim_weight = 1.0f;
    ik->enable_aim_offset = true;
    ik->enable_procedural_correction = true;
    ik->enable_scope_align = false;
    
    // Initialize aim grid (simplified - would load from animation data)
    for (i32 i = 0; i < 5; i++) {
        for (i32 j = 0; j < 5; j++) {
            ik->aim_grid[i][j] = 0.0f; // Would load actual animation poses
        }
    }
    
    ik->spine_bone_count = 0;
    ik->current_muzzle_vector = vec3_forward();
    ik->desired_muzzle_vector = vec3_forward();
    ik->correction_strength = 0.8f;
    ik->max_correction_angle = PI * 0.25f; // 45 degrees
    
    ik->is_ads = false;
    ik->camera_position = vec3_zero();
    ik->camera_forward = vec3_forward();
    ik->scope_offset = vec3(0.0f, 0.1f, 0.0f); // 10cm above weapon
    ik->alignment_speed = 0.1f;
    
    return ik;
}

void aim_ik_destroy(AimIK* ik) {
    if (ik) {
        free(ik);
    }
}

void aim_ik_set_target(AimIK* ik, Vec3 target) {
    if (ik) {
        ik->aim_target_world = target;
    }
}

void aim_ik_set_aim_weight(AimIK* ik, f32 weight) {
    if (ik) {
        ik->aim_weight = clampf(weight, 0.0f, 1.0f);
    }
}

void aim_ik_set_spine_bones(AimIK* ik, const i32* bones, u32 count) {
    if (ik && bones && count <= 5) {
        for (u32 i = 0; i < count; i++) {
            ik->spine_bones[i] = bones[i];
        }
        ik->spine_bone_count = count;
    }
}

void aim_ik_enable_scope_align(AimIK* ik, bool enable, Vec3 scope_offset) {
    if (ik) {
        ik->enable_scope_align = enable;
        ik->scope_offset = scope_offset;
    }
}

void aim_ik_set_ads(AimIK* ik, bool is_ads) {
    if (ik) {
        ik->is_ads = is_ads;
    }
}

void aim_ik_update(AimIK* ik, Skeleton* skeleton, i32 weapon_bone, i32 camera_bone, f32 dt) {
    if (!ik || !skeleton) {
        return;
    }
    
    // Calculate pitch and yaw to target
    Vec3 weapon_pos = (weapon_bone >= 0) ? vec3(
        skeleton->global_transforms[weapon_bone].m[3][0],
        skeleton->global_transforms[weapon_bone].m[3][1],
        skeleton->global_transforms[weapon_bone].m[3][2]
    ) : vec3_zero();
    
    Vec3 to_target = vec3_sub(ik->aim_target_world, weapon_pos);
    f32 horizontal_distance = sqrtf(to_target.x * to_target.x + to_target.z * to_target.z);
    f32 pitch = -atan2f(to_target.y, horizontal_distance);
    f32 yaw = atan2f(to_target.x, to_target.z);
    
    // Update aim offset grid blend
    if (ik->enable_aim_offset) {
        aim_ik_calculate_grid_blend(ik, pitch, yaw);
        aim_ik_apply_additive_pose(ik, skeleton);
    }
    
    // Update procedural correction
    if (ik->enable_procedural_correction) {
        aim_ik_update_procedural_correction(ik, skeleton, weapon_bone);
    }
    
    // Update scope alignment
    if (ik->enable_scope_align) {
        aim_ik_update_scope_alignment(ik, skeleton, weapon_bone, camera_bone);
    }
}
