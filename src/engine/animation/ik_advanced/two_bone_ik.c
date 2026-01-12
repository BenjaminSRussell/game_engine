/**
 * TWO-BONE IK SOLVER - Analytical Solution
 * Fast, exact solution for 2-bone chains (arms, legs)
 * Supports pole vector for orientation control
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float x, y, z;
} Vec3;

// Vector operations
static inline Vec3 vec3(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 vec3_scale(Vec3 v, float s) {
    return vec3(v.x * s, v.y * s, v.z * s);
}

static inline float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float vec3_distance(Vec3 a, Vec3 b) {
    return vec3_length(vec3_sub(a, b));
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        return vec3_scale(v, 1.0f / len);
    }
    return vec3(0, 1, 0);
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

void two_bone_ik_init() {
    // Initialize any global state if needed
}

// Main two-bone IK solver
bool two_bone_ik_solve(float root[3], float mid[3], float end[3],
                       float target[3], float pole[3], 
                       float length_upper, float length_lower,
                       float *out_mid_angle, float *out_end_angle) {
    
    // Convert to Vec3 for easier math
    Vec3 v_root = vec3(root[0], root[1], root[2]);
    Vec3 v_target = vec3(target[0], target[1], target[2]);
    Vec3 v_pole = vec3(pole[0], pole[1], pole[2]);
    
    // Calculate distances
    float target_dist = vec3_distance(v_root, v_target);
    float total_length = length_upper + length_lower;
    
    // Check if target is reachable
    if (target_dist > total_length) {
        // Target too far - stretch toward it
        Vec3 direction = vec3_normalize(vec3_sub(v_target, v_root));
        
        // Place joints in a straight line toward target
        mid[0] = root[0] + direction.x * length_upper;
        mid[1] = root[1] + direction.y * length_upper;
        mid[2] = root[2] + direction.z * length_upper;
        
        end[0] = root[0] + direction.x * total_length;
        end[1] = root[1] + direction.y * total_length;
        end[2] = root[2] + direction.z * total_length;
        
        if (out_mid_angle) *out_mid_angle = 0.0f;
        if (out_end_angle) *out_end_angle = 0.0f;
        
        return false;
    }
    
    // Check if target is too close (fully bent)
    float min_reach = fabsf(length_upper - length_lower);
    if (target_dist < min_reach) {
        target_dist = min_reach;
    }
    
    // Law of cosines to find elbow bend angle
    float cos_elbow = (length_upper * length_upper + target_dist * target_dist - 
                      length_lower * length_lower) / (2.0f * length_upper * target_dist);
    cos_elbow = fmaxf(-1.0f, fminf(1.0f, cos_elbow)); // Clamp to valid range
    float elbow_angle = acosf(cos_elbow);
    
    // Direction from root to target
    Vec3 root_to_target = vec3_normalize(vec3_sub(v_target, v_root));
    
    // Calculate bend plane using pole vector
    Vec3 root_to_pole = vec3_sub(v_pole, v_root);
    
    // Project pole onto plane perpendicular to root_to_target
    float pole_along = vec3_dot(root_to_pole, root_to_target);
    Vec3 pole_perp = vec3_sub(root_to_pole, vec3_scale(root_to_target, pole_along));
    Vec3 bend_direction = vec3_normalize(pole_perp);
    
    // If pole vector is parallel to target direction, use arbitrary perpendicular
    if (vec3_length(pole_perp) < 0.001f) {
        // Create arbitrary perpendicular vector
        Vec3 up = vec3(0, 1, 0);
        if (fabsf(root_to_target.y) > 0.9f) {
            up = vec3(1, 0, 0);
        }
        bend_direction = vec3_normalize(vec3_cross(root_to_target, up));
    }
    
    // Calculate middle joint position
    float mid_along = length_upper * cosf(elbow_angle);
    float mid_perp = length_upper * sinf(elbow_angle);
    
    Vec3 v_mid = vec3_add(v_root, vec3_scale(root_to_target, mid_along));
    v_mid = vec3_add(v_mid, vec3_scale(bend_direction, mid_perp));
    
    // Calculate end joint position (should be at target)
    Vec3 v_end = v_target;
    
    // Output results
    mid[0] = v_mid.x;
    mid[1] = v_mid.y;
    mid[2] = v_mid.z;
    
    end[0] = v_end.x;
    end[1] = v_end.y;
    end[2] = v_end.z;
    
    // Calculate joint angles for output (optional)
    if (out_mid_angle) {
        *out_mid_angle = elbow_angle;
    }
    
    if (out_end_angle) {
        // Calculate angle at end joint (for debugging/validation)
        Vec3 mid_to_end = vec3_normalize(vec3_sub(v_end, v_mid));
        Vec3 mid_to_root = vec3_normalize(vec3_sub(v_root, v_mid));
        float cos_end = vec3_dot(mid_to_end, mid_to_root);
        cos_end = fmaxf(-1.0f, fminf(1.0f, cos_end));
        *out_end_angle = acosf(cos_end);
    }
    
    return true;
}

// Simplified version without pole vector
bool two_bone_ik_solve_simple(float root[3], float mid[3], float end[3],
                              float target[3], float length_upper, float length_lower) {
    // Use default pole vector (above the root)
    float pole[3] = {root[0], root[1] + 1.0f, root[2]};
    return two_bone_ik_solve(root, mid, end, target, pole, length_upper, length_lower, NULL, NULL);
}

// Calculate joint angles for a given pose
void two_bone_ik_calculate_angles(const float root[3], const float mid[3], const float end[3],
                                  float *out_shoulder_angle, float *out_elbow_angle) {
    Vec3 v_root = vec3(root[0], root[1], root[2]);
    Vec3 v_mid = vec3(mid[0], mid[1], mid[2]);
    Vec3 v_end = vec3(end[0], end[1], end[2]);
    
    // Upper arm vector
    Vec3 upper = vec3_normalize(vec3_sub(v_mid, v_root));
    
    // Lower arm vector  
    Vec3 lower = vec3_normalize(vec3_sub(v_end, v_mid));
    
    // Full arm vector
    Vec3 full = vec3_normalize(vec3_sub(v_end, v_root));
    
    // Shoulder angle (between upper arm and full arm direction)
    if (out_shoulder_angle) {
        float cos_shoulder = vec3_dot(upper, full);
        cos_shoulder = fmaxf(-1.0f, fminf(1.0f, cos_shoulder));
        *out_shoulder_angle = acosf(cos_shoulder);
    }
    
    // Elbow angle (between upper and lower arm)
    if (out_elbow_angle) {
        float cos_elbow = -vec3_dot(upper, lower); // Negative for interior angle
        cos_elbow = fmaxf(-1.0f, fminf(1.0f, cos_elbow));
        *out_elbow_angle = acosf(cos_elbow);
    }
}

/*
 * TWO-BONE IK SOLVER FEATURES:
 * - Analytical solution (exact, no iteration needed)
 * - Extremely fast performance
 * - Supports pole vector for orientation control
 * - Handles unreachable targets gracefully
 * - Provides joint angle calculations
 * - Perfect for arms, legs, and simple mechanisms
 * - Numerically stable and predictable
 */
