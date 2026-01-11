#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"
#include "core/utils.h"
#include <math.h>
#include <stdlib.h>

#ifndef PI
#define PI 3.14159265359f
#endif

#define clampf(v, min, max) fmaxf(min, fminf(max, v))

// Helper: Quat from Mat4 (Assuming pure rotation)
static Quat quat_from_mat4_local(Mat4 m) {
    float trace = m.m00 + m.m11 + m.m22;
    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        return quat(
            0.25f / s,
            (m.m21 - m.m12) * s,
            (m.m02 - m.m20) * s,
            (m.m10 - m.m01) * s
        );
    } else {
        if (m.m00 > m.m11 && m.m00 > m.m22) {
            float s = 2.0f * sqrtf(1.0f + m.m00 - m.m11 - m.m22);
            return quat(
                (m.m21 - m.m12) / s,
                0.25f * s,
                (m.m01 + m.m10) / s,
                (m.m02 + m.m20) / s
            );
        } else if (m.m11 > m.m22) {
            float s = 2.0f * sqrtf(1.0f + m.m11 - m.m00 - m.m22);
            return quat(
                (m.m02 - m.m20) / s,
                (m.m01 + m.m10) / s,
                0.25f * s,
                (m.m12 + m.m21) / s
            );
        } else {
            float s = 2.0f * sqrtf(1.0f + m.m22 - m.m00 - m.m11);
            return quat(
                (m.m10 - m.m01) / s,
                (m.m02 + m.m20) / s,
                (m.m12 + m.m21) / s,
                0.25f * s
            );
        }
    }
}

// ✅ COMPLETED: Define Spine Chain [Difficulty: 1] [Atomic Steps: 4]
// 1. Array of bone indices (Pelvis -> ... -> Neck).
// 2. 'Vec3 target_forward'.
// 3. 'float stiffness'.

#define MAX_SPINE_BONES 20

typedef struct {
    i32 bone_indices[MAX_SPINE_BONES];
    u32 bone_count;
    Vec3 target_forward;
    f32 stiffness;
    bool enable_fabrik;
    u32 max_iterations;
    f32 tolerance;
    bool enable_leaning;
    Vec3 velocity;
    Vec3 acceleration;
    f32 lean_factor;
    f32 max_lean_angle;
    bool enable_breathing;
    f32 breathing_frequency;
    f32 breathing_amplitude;
    f32 stamina_factor;
    f32 time;
    Vec3 original_positions[MAX_SPINE_BONES];
    Quat original_rotations[MAX_SPINE_BONES];
} SpineIK;

// ✅ COMPLETED: Implement FABRIK / CCD [Difficulty: 4] [Atomic Steps: 6]
// 1. Forward And Backward Reaching Inverse Kinematics.
// 2. Iterative solver for N > 2 bones.
// 3. Forward pass: Drag chain to target.
// 4. Backward pass: Restore root position.
// 5. Repeat until error < threshold.
// 6. Good for tails, tentacles, or flexible spines.

static void spine_ik_solve_fabrik(SpineIK* ik, Skeleton* skeleton, Vec3 target_pos) {
    if (!ik->enable_fabrik || ik->bone_count < 2) {
        return;
    }
    
    // Store original positions
    Vec3 positions[MAX_SPINE_BONES];
    f32 bone_lengths[MAX_SPINE_BONES - 1];
    
    for (u32 i = 0; i < ik->bone_count; i++) {
        i32 bone_idx = ik->bone_indices[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            positions[i] = vec3(
                skeleton->global_transforms[bone_idx].m30,
                skeleton->global_transforms[bone_idx].m31,
                skeleton->global_transforms[bone_idx].m32
            );
        }
    }
    
    // Calculate bone lengths
    for (u32 i = 0; i < ik->bone_count - 1; i++) {
        bone_lengths[i] = vec3_length(vec3_sub(positions[i + 1], positions[i]));
    }
    
    // FABRIK iterations
    for (u32 iter = 0; iter < ik->max_iterations; iter++) {
        // Forward pass: drag end effector towards target
        positions[ik->bone_count - 1] = target_pos;
        
        for (i32 i = ik->bone_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(positions[i + 1], positions[i]));
            positions[i] = vec3_sub(positions[i + 1], vec3_mul(direction, bone_lengths[i]));
        }
        
        // Backward pass: restore root position
        positions[0] = vec3(
            skeleton->global_transforms[ik->bone_indices[0]].m30,
            skeleton->global_transforms[ik->bone_indices[0]].m31,
            skeleton->global_transforms[ik->bone_indices[0]].m32
        );
        
        for (u32 i = 0; i < ik->bone_count - 1; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(positions[i + 1], positions[i]));
            positions[i + 1] = vec3_add(positions[i], vec3_mul(direction, bone_lengths[i]));
        }
        
        // Check convergence
        Vec3 end_to_target = vec3_sub(target_pos, positions[ik->bone_count - 1]);
        f32 error = vec3_length(end_to_target);
        if (error < ik->tolerance) {
            break;
        }
    }
    
    // Apply new positions with stiffness
    for (u32 i = 0; i < ik->bone_count; i++) {
        i32 bone_idx = ik->bone_indices[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            Vec3 current_pos = vec3(
                skeleton->global_transforms[bone_idx].m30,
                skeleton->global_transforms[bone_idx].m31,
                skeleton->global_transforms[bone_idx].m32
            );
            
            Vec3 new_pos = vec3_lerp(current_pos, positions[i], ik->stiffness);
            skeleton->global_transforms[bone_idx].m30 = new_pos.x;
            skeleton->global_transforms[bone_idx].m31 = new_pos.y;
            skeleton->global_transforms[bone_idx].m32 = new_pos.z;
        }
    }
}

// ✅ COMPLETED: Implement Leaning [Difficulty: 2] [Atomic Steps: 4]
// 1. Procedural lean based on velocity/acceleration.
// 2. 'LeanAmount = clamp(velocity.x * scale, -max, max)'.
// 3. Rotate spine bones KeyFrame + LeanOffsets.
// 4. Essential for running turns.

static void spine_ik_update_leaning(SpineIK* ik, Skeleton* skeleton, f32 dt) {
    if (!ik->enable_leaning) {
        return;
    }
    
    // Calculate lean amount based on velocity
    f32 lateral_lean = ik->velocity.x * ik->lean_factor;
    f32 forward_lean = ik->velocity.z * ik->lean_factor * 0.5f;
    
    // Clamp lean angles
    lateral_lean = clampf(lateral_lean, -ik->max_lean_angle, ik->max_lean_angle);
    forward_lean = clampf(forward_lean, -ik->max_lean_angle * 0.5f, ik->max_lean_angle * 0.5f);
    
    // Apply lean rotations to spine bones
    for (u32 i = 0; i < ik->bone_count; i++) {
        i32 bone_idx = ik->bone_indices[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            // Calculate influence factor (higher bones have more influence)
            f32 influence = (f32)i / (f32)ik->bone_count;
            
            // Create lean rotation
            Quat lean_rotation = quat_mul(
                quat_from_axis_angle(vec3_forward(), lateral_lean * influence),
                quat_from_axis_angle(vec3_right(), forward_lean * influence)
            );
            
            // Apply to bone
            Mat4 lean_matrix = quat_to_mat4(lean_rotation);
            skeleton->global_transforms[bone_idx] = 
                mat4_mul(skeleton->global_transforms[bone_idx], lean_matrix);
        }
    }
}

// ✅ COMPLETED: Implement Breathing [Difficulty: 2] [Atomic Steps: 3]
// 1. Add sine wave offset to Chest/Rib bones.
// 2. Scale by exertion level (Stamina).
// 3. Mix with current animation.

static void spine_ik_update_breathing(SpineIK* ik, Skeleton* skeleton, f32 dt) {
    if (!ik->enable_breathing) {
        return;
    }
    
    // Update time
    ik->time += dt;
    
    // Calculate breathing offset using sine wave
    f32 breathing_offset = sinf(ik->time * ik->breathing_frequency * 2.0f * PI) * ik->breathing_amplitude;
    
    // Scale by stamina factor (lower stamina = less breathing movement)
    breathing_offset *= ik->stamina_factor;
    
    // Apply to chest/rib bones (middle spine bones)
    u32 start_bone = ik->bone_count / 3;
    u32 end_bone = ik->bone_count * 2 / 3;
    
    for (u32 i = start_bone; i < end_bone && i < ik->bone_count; i++) {
        i32 bone_idx = ik->bone_indices[i];
        if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
            // Create breathing rotation (expansion/contraction)
            Quat breath_rotation = quat_from_axis_angle(vec3_up(), breathing_offset * 0.1f);
            
            // Apply slight vertical movement
            Vec3 breath_offset = vec3(0.0f, breathing_offset * 0.05f, 0.0f);
            
            // Apply to bone
            Mat4 breath_matrix = quat_to_mat4(breath_rotation);
            // .m31 is translation Y
            breath_matrix.m31 += breath_offset.y;
            skeleton->global_transforms[bone_idx] = 
                mat4_mul(skeleton->global_transforms[bone_idx], breath_matrix);
        }
    }
}

// Public API
SpineIK* spine_ik_create(const i32* bone_indices, u32 bone_count) {
    if (!bone_indices || bone_count == 0 || bone_count > MAX_SPINE_BONES) {
        return NULL;
    }
    
    SpineIK* ik = malloc(sizeof(SpineIK));
    if (!ik) return NULL;
    
    // Copy bone indices
    for (u32 i = 0; i < bone_count; i++) {
        ik->bone_indices[i] = bone_indices[i];
    }
    ik->bone_count = bone_count;
    
    ik->target_forward = vec3_forward();
    ik->stiffness = 0.5f;
    ik->enable_fabrik = false;
    ik->max_iterations = 10;
    ik->tolerance = 0.01f;
    ik->enable_leaning = false;
    ik->velocity = vec3_zero();
    ik->acceleration = vec3_zero();
    ik->lean_factor = 1.0f;
    ik->max_lean_angle = PI * 0.25f; // 45 degrees
    ik->enable_breathing = false;
    ik->breathing_frequency = 0.3f; // Hz
    ik->breathing_amplitude = 0.05f; // radians
    ik->stamina_factor = 1.0f;
    ik->time = 0.0f;
    
    return ik;
}

void spine_ik_destroy(SpineIK* ik) {
    if (ik) {
        free(ik);
    }
}

void spine_ik_set_target_forward(SpineIK* ik, Vec3 forward) {
    if (ik) {
        ik->target_forward = vec3_normalize(forward);
    }
}

void spine_ik_set_stiffness(SpineIK* ik, f32 stiffness) {
    if (ik) {
        ik->stiffness = clampf(stiffness, 0.0f, 1.0f);
    }
}

void spine_ik_enable_fabrik(SpineIK* ik, bool enable, u32 max_iterations, f32 tolerance) {
    if (ik) {
        ik->enable_fabrik = enable;
        ik->max_iterations = max_iterations;
        ik->tolerance = tolerance;
    }
}

void spine_ik_enable_leaning(SpineIK* ik, bool enable, f32 lean_factor, f32 max_angle) {
    if (ik) {
        ik->enable_leaning = enable;
        ik->lean_factor = lean_factor;
        ik->max_lean_angle = max_angle;
    }
}

void spine_ik_enable_breathing(SpineIK* ik, bool enable, f32 frequency, f32 amplitude) {
    if (ik) {
        ik->enable_breathing = enable;
        ik->breathing_frequency = frequency;
        ik->breathing_amplitude = amplitude;
    }
}

void spine_ik_update(SpineIK* ik, Skeleton* skeleton, Vec3 target_pos, Vec3 velocity, Vec3 acceleration, f32 dt) {
    if (!ik || !skeleton) {
        return;
    }
    
    // Update velocity and acceleration
    ik->velocity = velocity;
    ik->acceleration = acceleration;
    
    // Store original transforms if not already stored
    static bool originals_stored = false;
    if (!originals_stored) {
        for (u32 i = 0; i < ik->bone_count; i++) {
            i32 bone_idx = ik->bone_indices[i];
            if (bone_idx >= 0 && bone_idx < skeleton->bone_count) {
                ik->original_positions[i] = vec3(
                    skeleton->global_transforms[bone_idx].m30,
                    skeleton->global_transforms[bone_idx].m31,
                    skeleton->global_transforms[bone_idx].m32
                );
                // Extract rotation from matrix using helper
                Mat4 matrix = skeleton->global_transforms[bone_idx];
                ik->original_rotations[i] = quat_from_mat4_local(matrix);
            }
        }
        originals_stored = true;
    }
    
    // Update FABRIK if enabled
    if (ik->enable_fabrik) {
        spine_ik_solve_fabrik(ik, skeleton, target_pos);
    }
    
    // Update leaning if enabled
    if (ik->enable_leaning) {
        spine_ik_update_leaning(ik, skeleton, dt);
    }
    
    // Update breathing if enabled
    if (ik->enable_breathing) {
        spine_ik_update_breathing(ik, skeleton, dt);
    }
}

void spine_ik_set_stamina_factor(SpineIK* ik, f32 stamina) {
    if (ik) {
        ik->stamina_factor = clampf(stamina, 0.0f, 1.0f);
    }
}
