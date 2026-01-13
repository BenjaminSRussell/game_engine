#include "../../character/animation/ik/fabrik_solver.h"
#include "include/math/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FABRIK_CHAINS 64
#define MAX_CHAIN_LENGTH 16
#define FABRIK_TOLERANCE 0.001f
#define FABRIK_MAX_ITERATIONS 10

typedef struct FabrikJoint {
    Vec3 position;
    float min_angle;
    float max_angle;
    bool constrained;
} FabrikJoint;

typedef struct FabrikChain {
    FabrikJoint joints[MAX_CHAIN_LENGTH];
    uint32_t joint_count;
    float bone_lengths[MAX_CHAIN_LENGTH - 1];
    float total_length;
    bool active;
} FabrikChain;

static FabrikChain g_chains[MAX_FABRIK_CHAINS];
static uint32_t g_chain_count = 0;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static Vec3 rotate_point_around_axis(const Vec3* point, const Vec3* axis_origin, 
                                     const Vec3* axis, float angle) {
    // Rodrigues' rotation formula
    Vec3 p = vec3_sub(*point, *axis_origin);
    Vec3 k = vec3_normalize(*axis);
    
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    
    Vec3 term1 = vec3_mul(p, cos_angle);
    Vec3 term2 = vec3_mul(vec3_cross(k, p), sin_angle);
    Vec3 term3 = vec3_mul(k, vec3_dot(k, p) * (1.0f - cos_angle));
    
    return vec3_add(*axis_origin, vec3_add(vec3_add(term1, term2), term3));
}

static void apply_joint_constraints(FabrikChain* chain, int joint_index) {
    if (!chain->joints[joint_index].constrained || 
        joint_index <= 0 || joint_index >= (int)chain->joint_count - 1) {
        return;
    }
    
    // Calculate angle between adjacent bones
    Vec3 bone1 = vec3_normalize(vec3_sub(chain->joints[joint_index].position, 
                                         chain->joints[joint_index-1].position));
    Vec3 bone2 = vec3_normalize(vec3_sub(chain->joints[joint_index+1].position, 
                                         chain->joints[joint_index].position));
    float angle = acosf(fmaxf(-1.0f, fminf(1.0f, vec3_dot(bone1, bone2))));
    float angle_deg = angle * 180.0f / M_PI;
    
    // Enforce angle constraints
    if (angle_deg < chain->joints[joint_index].min_angle) {
        // Rotate to minimum angle
        float rotation_needed = (chain->joints[joint_index].min_angle - angle_deg) * M_PI / 180.0f;
        Vec3 axis = vec3_normalize(vec3_cross(bone1, bone2));
        chain->joints[joint_index+1].position = rotate_point_around_axis(
            &chain->joints[joint_index+1].position,
            &chain->joints[joint_index].position,
            &axis, rotation_needed);
    } else if (angle_deg > chain->joints[joint_index].max_angle) {
        // Rotate to maximum angle
        float rotation_needed = (angle_deg - chain->joints[joint_index].max_angle) * M_PI / 180.0f;
        Vec3 axis = vec3_normalize(vec3_cross(bone1, bone2));
        chain->joints[joint_index+1].position = rotate_point_around_axis(
            &chain->joints[joint_index+1].position,
            &chain->joints[joint_index].position,
            &axis, -rotation_needed);
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void fabrik_init() {
    memset(g_chains, 0, sizeof(g_chains));
    g_chain_count = 0;
}

uint32_t fabrik_create_chain(const Vec3* positions, uint32_t joint_count) {
    if (g_chain_count >= MAX_FABRIK_CHAINS || joint_count < 2 || joint_count > MAX_CHAIN_LENGTH) {
        return UINT32_MAX;
    }
    
    FabrikChain* chain = &g_chains[g_chain_count];
    chain->joint_count = joint_count;
    chain->active = true;
    chain->total_length = 0.0f;
    
    // Copy joint positions
    for (uint32_t i = 0; i < joint_count; i++) {
        chain->joints[i].position = positions[i];
        chain->joints[i].constrained = false;
        chain->joints[i].min_angle = 0.0f;
        chain->joints[i].max_angle = 180.0f;
        
        if (i > 0) {
            Vec3 bone = vec3_sub(positions[i], positions[i-1]);
            chain->bone_lengths[i-1] = vec3_length(bone);
            chain->total_length += chain->bone_lengths[i-1];
        }
    }
    
    return g_chain_count++;
}

void fabrik_solve(uint32_t chain_id, const Vec3* target, int iterations) {
    if (chain_id >= g_chain_count || !g_chains[chain_id].active) return;
    
    FabrikChain* chain = &g_chains[chain_id];
    if (chain->joint_count < 2) return;
    
    // Check if target is reachable
    float target_distance = vec3_distance(chain->joints[0].position, *target);
    if (target_distance > chain->total_length) {
        // Target unreachable - stretch towards it
        Vec3 direction = vec3_normalize(vec3_sub(*target, chain->joints[0].position));
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            chain->joints[i].position = vec3_add(chain->joints[i-1].position, 
                                               vec3_mul(direction, chain->bone_lengths[i-1]));
        }
        return;
    }
    
    // FABRIK algorithm iterations
    for (int iter = 0; iter < iterations; iter++) {
        // Forward reaching - move end effector to target
        chain->joints[chain->joint_count - 1].position = *target;
        
        for (int32_t i = chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->joints[i].position, 
                                                   chain->joints[i + 1].position));
            chain->joints[i].position = vec3_add(chain->joints[i + 1].position, 
                                               vec3_mul(direction, chain->bone_lengths[i]));
            
            // Apply constraints using the helper function
            apply_joint_constraints(chain, i);
        }
        
        // Backward reaching - move root back to original position
        Vec3 original_root = chain->joints[0].position;
        chain->joints[0].position = original_root;
        
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->joints[i].position, 
                                                   chain->joints[i - 1].position));
            chain->joints[i].position = vec3_add(chain->joints[i - 1].position, 
                                               vec3_mul(direction, chain->bone_lengths[i - 1]));
        }
        
        // Check convergence
        float end_error = vec3_distance(chain->joints[chain->joint_count - 1].position, *target);
        if (end_error < FABRIK_TOLERANCE) {
            break;
        }
    }
}

void fabrik_add_constraint(uint32_t chain_id, int joint_index, float min_angle, float max_angle) {
    if (chain_id >= g_chain_count || joint_index < 0 || 
        joint_index >= (int)g_chains[chain_id].joint_count) {
        return;
    }
    
    FabrikChain* chain = &g_chains[chain_id];
    chain->joints[joint_index].constrained = true;
    chain->joints[joint_index].min_angle = min_angle;
    chain->joints[joint_index].max_angle = max_angle;
}

Vec3 fabrik_get_joint_position(uint32_t chain_id, uint32_t joint_index) {
    if (chain_id >= g_chain_count || joint_index >= g_chains[chain_id].joint_count) {
        return (Vec3){0, 0, 0};
    }
    
    return g_chains[chain_id].joints[joint_index].position;
}

void fabrik_set_joint_position(uint32_t chain_id, uint32_t joint_index, const Vec3* position) {
    if (chain_id >= g_chain_count || joint_index >= g_chains[chain_id].joint_count) {
        return;
    }
    
    g_chains[chain_id].joints[joint_index].position = *position;
    
    // Recalculate bone lengths if this isn't the root joint
    if (joint_index > 0) {
        Vec3 bone = vec3_sub(*position, g_chains[chain_id].joints[joint_index - 1].position);
        g_chains[chain_id].bone_lengths[joint_index - 1] = vec3_length(bone);
        
        // Recalculate total length
        g_chains[chain_id].total_length = 0.0f;
        for (uint32_t i = 0; i < g_chains[chain_id].joint_count - 1; i++) {
            g_chains[chain_id].total_length += g_chains[chain_id].bone_lengths[i];
        }
    }
}

bool fabrik_is_chain_active(uint32_t chain_id) {
    return chain_id < g_chain_count && g_chains[chain_id].active;
}

void fabrik_destroy_chain(uint32_t chain_id) {
    if (chain_id >= g_chain_count) return;
    
    g_chains[chain_id].active = false;
    g_chains[chain_id].joint_count = 0;
}

uint32_t fabrik_get_chain_count(void) {
    return g_chain_count;
}
