#include "include/animation/ik_system.h"
#include "include/core/logger.h"
#include "include/math/math.h"
#include <math.h>

/**
 * @brief Limits a rotation quaternion to be within angular limits
 * @param rotation The rotation to clamp
 * @param min_angles Minimum Euler angles (radians)
 * @param max_angles Maximum Euler angles (radians)
 * @return Clamped quaternion
 */
static Quat clamp_rotation(Quat rotation, Vec3 min_angles, Vec3 max_angles) {
    // Convert to Euler angles (X=Roll, Y=Pitch, Z=Yaw)
    Vec3 euler = quat_to_euler(rotation);
    
    bool changed = false;
    
    if (euler.x < min_angles.x) { euler.x = min_angles.x; changed = true; }
    if (euler.x > max_angles.x) { euler.x = max_angles.x; changed = true; }
    
    if (euler.y < min_angles.y) { euler.y = min_angles.y; changed = true; }
    if (euler.y > max_angles.y) { euler.y = max_angles.y; changed = true; }
    
    if (euler.z < min_angles.z) { euler.z = min_angles.z; changed = true; }
    if (euler.z > max_angles.z) { euler.z = max_angles.z; changed = true; }
    
    if (changed) {
        // quat_from_euler takes (pitch, yaw, roll) -> (y, z, x)
        return quat_from_euler(euler.y, euler.z, euler.x);
    }
    return rotation;
}

/**
 * @brief Advanced CCD Solver implementation
 * Supports constraints and pole vectors
 */
static void ik_ccd_solve_chain(IKChain *chain) {
    if (!chain || chain->bone_count < 2) return;
    
    Vec3 target_pos = chain->target_position;
    
    // Performance: Early exit if already at target
    Vec3 end_effector = chain->bones[chain->bone_count - 1].position;
    if (vec3_distance_sq(end_effector, target_pos) < chain->precision * chain->precision) {
        return;
    }

    // Iteration loop
    for (u32 iter = 0; iter < chain->max_iterations; iter++) {
        bool any_change = false;

        // Iterate from end effector's parent down to the root
        for (i32 i = chain->bone_count - 2; i >= 0; i--) {
            // Get current bone position and end effector position
            Vec3 bone_pos = chain->bones[i].position;
            Vec3 end_pos = chain->bones[chain->bone_count - 1].position;

            // Vector from current bone to end effector
            Vec3 to_end = vec3_sub(end_pos, bone_pos);
            // Vector from current bone to target
            Vec3 to_target = vec3_sub(target_pos, bone_pos);
            
            // Normalize
            f32 len_end = vec3_length(to_end);
            f32 len_target = vec3_length(to_target);
            
            // Skip if too short (singularity)
            if (len_end < 0.001f || len_target < 0.001f) continue;

            Vec3 to_end_norm = vec3_div(to_end, len_end);
            Vec3 to_target_norm = vec3_div(to_target, len_target);

            // Calculate rotation to align to_end with to_target
            f32 dot = vec3_dot(to_end_norm, to_target_norm);
            dot = fmaxf(-1.0f, fminf(1.0f, dot));

            f32 angle = acosf(dot);
            Vec3 axis;

            // Handle singularity (straight chain with target closer) or normal case
            if (angle < 0.001f) {
                // If vectors are aligned but target is closer, we need to bend
                if (len_target < len_end - 0.01f) {
                    // Need to bend. Use pole vector or default axis.
                    if (chain->pole_vector_enabled) {
                        Vec3 to_pole = vec3_sub(chain->pole_vector, bone_pos);
                        axis = vec3_cross(to_end_norm, to_pole);
                        if (vec3_length_sq(axis) < 0.001f) axis = vec3_create(1,0,0);
                    } else {
                        // Arbitrary axis (Y)
                        axis = vec3_create(0,1,0);
                        // Check if parallel to to_end
                        if (fabsf(vec3_dot(to_end_norm, axis)) > 0.9f) axis = vec3_create(1,0,0);
                    }
                    axis = vec3_normalize(axis);
                    angle = 0.05f; // Small nudge to break symmetry
                } else {
                    continue; // Aligned and distance fine (or target further away)
                }
            } else {
                // Normal case
                axis = vec3_cross(to_end_norm, to_target_norm);
                if (vec3_length_sq(axis) < 0.000001f) {
                    continue; // Should be handled by angle check, but safe guard
                }
                axis = vec3_normalize(axis);
            }
            
            // Dampen angle by stiffness if set
            if (chain->bones[i].stiffness > 0.0f) {
                angle *= (1.0f - chain->bones[i].stiffness);
            }
            
            // Create rotation quaternion (in World Space)
            Quat rot_delta = quat_from_axis_angle(axis, angle);

            // Save old rotation to compute actual change after constraints
            Quat old_rot = chain->bones[i].rotation;

            // Apply rotation to current bone
            chain->bones[i].rotation = quat_mul(rot_delta, chain->bones[i].rotation);
            chain->bones[i].rotation = quat_normalize(chain->bones[i].rotation);

            // Constraints (Local Space)
            if (chain->bones[i].constraints_enabled) {
                // Get Parent Rotation
                Quat parent_rot = quat_identity();
                if (i > 0) {
                    parent_rot = chain->bones[i-1].rotation;
                }

                // Convert to Local Space: local = inv(parent) * world
                Quat inv_parent = quat_conjugate(parent_rot);
                Quat local_rot = quat_mul(inv_parent, chain->bones[i].rotation);

                // Clamp
                local_rot = clamp_rotation(
                    local_rot,
                    chain->bones[i].min_angles,
                    chain->bones[i].max_angles
                );

                // Convert back to World Space: world = parent * local
                chain->bones[i].rotation = quat_mul(parent_rot, local_rot);
                chain->bones[i].rotation = quat_normalize(chain->bones[i].rotation);
            }

            // Calculate ACTUAL delta applied (after constraints)
            // new = delta * old -> delta = new * inv(old)
            Quat inv_old = quat_conjugate(old_rot);
            Quat actual_delta = quat_mul(chain->bones[i].rotation, inv_old);
            actual_delta = quat_normalize(actual_delta);

            // Update children
            for (u32 j = i + 1; j < chain->bone_count; j++) {
                // Update position
                Vec3 parent_pos = chain->bones[j-1].position;
                f32 length = chain->bones[j-1].length;

                // Assuming bone direction is X axis in local space
                Vec3 offset = quat_rotate_vec3(chain->bones[j-1].rotation, vec3_create(length, 0, 0));
                chain->bones[j].position = vec3_add(parent_pos, offset);

                // Update global rotation by applying the parent's change
                chain->bones[j].rotation = quat_mul(actual_delta, chain->bones[j].rotation);
                chain->bones[j].rotation = quat_normalize(chain->bones[j].rotation);
            }

            any_change = true;
        }
        
        // Pole Vector
        // We rotate the entire chain around the (Root->End) axis so that the mid-joint lies on the pole plane
        if (chain->pole_vector_enabled && chain->bone_count >= 3) {
            Vec3 root_pos = chain->bones[0].position;
            Vec3 end_pos = chain->bones[chain->bone_count - 1].position;
            Vec3 mid_pos = chain->bones[chain->bone_count / 2].position;

            Vec3 pole_target = chain->pole_vector; // Position

            Vec3 root_to_end = vec3_sub(end_pos, root_pos);
            if (vec3_length_sq(root_to_end) > 0.0001f) {
                Vec3 root_to_mid = vec3_sub(mid_pos, root_pos);
                Vec3 root_to_pole = vec3_sub(pole_target, root_pos);

                // Normal of the plane defined by Root, End, and Current Mid
                Vec3 current_normal = vec3_cross(root_to_end, root_to_mid);
                // Normal of the plane defined by Root, End, and Pole Target
                Vec3 target_normal = vec3_cross(root_to_end, root_to_pole);

                // Check for singularity in pole vector logic
                // If chain is straight, current_normal is zero.
                // But if we nudged it earlier, it shouldn't be zero!
                // However, we apply pole vector correction AFTER the loop.
                // If the loop straightened it out (unlikely if target is close), or if the nudge was small.

                if (vec3_length_sq(current_normal) > 0.0001f && vec3_length_sq(target_normal) > 0.0001f) {
                    current_normal = vec3_normalize(current_normal);
                    target_normal = vec3_normalize(target_normal);

                    // Rotation needed to align normal
                    // Axis of rotation is root_to_end (normalized)
                    Vec3 axis = vec3_normalize(root_to_end);

                    // Angle
                    float cos_theta = vec3_dot(current_normal, target_normal);
                    cos_theta = fmaxf(-1.0f, fminf(1.0f, cos_theta));
                    float theta = acosf(cos_theta);

                    // Determine direction
                    Vec3 cross_normals = vec3_cross(current_normal, target_normal);
                    if (vec3_dot(cross_normals, axis) < 0) {
                        theta = -theta;
                    }

                    if (fabsf(theta) > 0.001f) {
                        Quat twist_rot = quat_from_axis_angle(axis, theta);

                        // Apply twist to root
                        chain->bones[0].rotation = quat_mul(twist_rot, chain->bones[0].rotation);
                        chain->bones[0].rotation = quat_normalize(chain->bones[0].rotation);

                        // Update all bone positions and rotations
                        for (u32 j = 1; j < chain->bone_count; j++) {
                            // Recompute position based on parent
                            Vec3 parent_pos = chain->bones[j-1].position;
                            f32 length = chain->bones[j-1].length;
                            Vec3 offset = quat_rotate_vec3(chain->bones[j-1].rotation, vec3_create(length, 0, 0));
                            chain->bones[j].position = vec3_add(parent_pos, offset);

                            // Rotate children too
                            chain->bones[j].rotation = quat_mul(twist_rot, chain->bones[j].rotation);
                            chain->bones[j].rotation = quat_normalize(chain->bones[j].rotation);
                        }
                        any_change = true;
                    }
                }
            }
        }
        
        // Check for convergence
        end_effector = chain->bones[chain->bone_count - 1].position;
        if (vec3_distance_sq(end_effector, target_pos) < chain->precision * chain->precision) {
            break;
        }

        if (!any_change) break;
    }
}
