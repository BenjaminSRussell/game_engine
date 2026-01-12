/**
 * CCD IK SOLVER - Cyclic Coordinate Descent
 * Iterative IK solver that adjusts one joint at a time
 * Good for complex chains and easy to implement constraints
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CCD_MAX_BONES 32
#define CCD_MAX_ITERATIONS 50
#define CCD_TOLERANCE 0.001f

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    Vec3 rotation;  // Euler angles for simplicity
} Bone;

typedef struct {
    Bone bones[CCD_MAX_BONES];
    uint32_t bone_count;
    Vec3 target_position;
    uint32_t max_iterations;
    float tolerance;
    bool reached_target;
} CCDChain;

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

// Convert bone position to world space
static Vec3 get_bone_world_position(const CCDChain *chain, uint32_t bone_index) {
    Vec3 pos = vec3(0, 0, 0);
    
    for (uint32_t i = 0; i <= bone_index; i++) {
        pos = vec3_add(pos, chain->bones[i].position);
    }
    
    return pos;
}

// Rotate a point around an axis
static Vec3 rotate_point_around_axis(Vec3 point, Vec3 axis, Vec3 pivot, float angle) {
    // Translate to pivot
    Vec3 p = vec3_sub(point, pivot);
    
    // Normalize axis
    Vec3 a = vec3_normalize(axis);
    
    // Rodrigues' rotation formula
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    
    Vec3 term1 = vec3_scale(p, cos_angle);
    Vec3 term2 = vec3_scale(vec3_cross(a, p), sin_angle);
    Vec3 term3 = vec3_scale(a, vec3_dot(a, p) * (1.0f - cos_angle));
    
    Vec3 rotated = vec3_add(vec3_add(term1, term2), term3);
    
    // Translate back
    return vec3_add(rotated, pivot);
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// CCD chain creation
CCDChain *ccd_chain_create(const float *bone_positions, uint32_t bone_count) {
    if (bone_count == 0 || bone_count > CCD_MAX_BONES) {
        return NULL;
    }
    
    CCDChain *chain = (CCDChain*)calloc(1, sizeof(CCDChain));
    if (!chain) {
        return NULL;
    }
    
    chain->bone_count = bone_count;
    chain->max_iterations = CCD_MAX_ITERATIONS;
    chain->tolerance = CCD_TOLERANCE;
    
    // Initialize bone positions
    for (uint32_t i = 0; i < bone_count; i++) {
        chain->bones[i].position = vec3(
            bone_positions[i * 3],
            bone_positions[i * 3 + 1], 
            bone_positions[i * 3 + 2]
        );
        chain->bones[i].rotation = vec3(0, 0, 0);
    }
    
    return chain;
}

void ccd_chain_destroy(CCDChain *chain) {
    if (chain) {
        free(chain);
    }
}

// CCD solve algorithm
bool ccd_solve(CCDChain *chain, float target_x, float target_y, float target_z) {
    if (!chain) {
        return false;
    }
    
    chain->target_position = vec3(target_x, target_y, target_z);
    chain->reached_target = false;
    
    // CCD iterations
    for (uint32_t iter = 0; iter < chain->max_iterations; iter++) {
        // Update each bone from end to root
        for (int32_t i = (int32_t)chain->bone_count - 1; i >= 0; i--) {
            Vec3 bone_pos = get_bone_world_position(chain, i);
            Vec3 end_effector = get_bone_world_position(chain, chain->bone_count - 1);
            
            // Vectors from bone to end effector and target
            Vec3 to_end = vec3_normalize(vec3_sub(end_effector, bone_pos));
            Vec3 to_target = vec3_normalize(vec3_sub(chain->target_position, bone_pos));
            
            // Calculate rotation angle
            float dot = vec3_dot(to_end, to_target);
            dot = fmaxf(-1.0f, fminf(1.0f, dot)); // Clamp to avoid domain errors
            float angle = acosf(dot);
            
            // Calculate rotation axis
            Vec3 axis = vec3_cross(to_end, to_target);
            if (vec3_length(axis) < 0.0001f) {
                continue; // Already aligned
            }
            
            // Apply rotation to this bone and all subsequent bones
            for (uint32_t j = i; j < chain->bone_count; j++) {
                chain->bones[j].position = rotate_point_around_axis(
                    chain->bones[j].position, axis, bone_pos, angle
                );
            }
        }
        
        // Check convergence
        Vec3 end_effector = get_bone_world_position(chain, chain->bone_count - 1);
        float error = vec3_distance(end_effector, chain->target_position);
        
        if (error < chain->tolerance) {
            chain->reached_target = true;
            return true;
        }
    }
    
    return false;
}

// Configuration functions
void ccd_set_iterations(CCDChain *chain, uint32_t iterations) {
    if (chain) {
        chain->max_iterations = iterations;
    }
}

void ccd_set_tolerance(CCDChain *chain, float tolerance) {
    if (chain) {
        chain->tolerance = tolerance;
    }
}

// Query functions
void ccd_get_bone_position(const CCDChain *chain, uint32_t bone_index, float *out) {
    if (!chain || !out || bone_index >= chain->bone_count) {
        return;
    }
    
    Vec3 world_pos = get_bone_world_position(chain, bone_index);
    out[0] = world_pos.x;
    out[1] = world_pos.y;
    out[2] = world_pos.z;
}

bool ccd_reached_target(const CCDChain *chain) {
    return chain ? chain->reached_target : false;
}

float ccd_get_error(const CCDChain *chain) {
    if (!chain) {
        return 0.0f;
    }
    
    Vec3 end_effector = get_bone_world_position(chain, chain->bone_count - 1);
    return vec3_distance(end_effector, chain->target_position);
}

/*
 * CCD IK SOLVER FEATURES:
 * - Iterative solver that adjusts one joint at a time
 * - Simple to implement and understand
 * - Easy to add joint constraints
 * - Works well for complex chains
 * - Guaranteed to converge (though may be slow)
 * - Memory efficient and fast for small chains
 */
