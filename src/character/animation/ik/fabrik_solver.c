#include "include/common.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_CHAINS 64
#define MAX_JOINTS_PER_CHAIN 16

typedef struct {
    Vec3 positions[MAX_JOINTS_PER_CHAIN];
    float lengths[MAX_JOINTS_PER_CHAIN - 1];
    uint32_t joint_count;
    float min_angles[MAX_JOINTS_PER_CHAIN];
    float max_angles[MAX_JOINTS_PER_CHAIN];
    bool has_constraints[MAX_JOINTS_PER_CHAIN];
} FABRIKChain;

typedef struct {
    FABRIKChain chains[MAX_CHAINS];
    uint32_t chain_count;
    bool initialized;
} FABRIKSolver;

static FABRIKSolver g_solver = {0};

void fabrik_init(void) {
    memset(&g_solver, 0, sizeof(FABRIKSolver));
    g_solver.initialized = true;
}

uint32_t fabrik_add_chain(const Vec3* positions, uint32_t joint_count) {
    if (!g_solver.initialized || g_solver.chain_count >= MAX_CHAINS || 
        !positions || joint_count < 2 || joint_count > MAX_JOINTS_PER_CHAIN) {
        return UINT32_MAX;
    }
    
    uint32_t chain_id = g_solver.chain_count++;
    FABRIKChain* chain = &g_solver.chains[chain_id];
    
    memcpy(chain->positions, positions, joint_count * sizeof(Vec3));
    chain->joint_count = joint_count;
    
    for (uint32_t i = 0; i < joint_count - 1; i++) {
        Vec3 diff = vec3_sub(positions[i + 1], positions[i]);
        chain->lengths[i] = vec3_length(diff);
    }
    
    memset(chain->has_constraints, 0, sizeof(chain->has_constraints));
    return chain_id;
}

bool fabrik_solve_chain(uint32_t chain_id, const Vec3* target, int max_iterations) {
    if (!g_solver.initialized || chain_id >= g_solver.chain_count || !target) {
        return false;
    }
    
    FABRIKChain* chain = &g_solver.chains[chain_id];
    Vec3 target_pos = *target;
    Vec3 root = chain->positions[0];
    float tolerance = 0.001f;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // Forward reaching
        chain->positions[chain->joint_count - 1] = target_pos;
        for (int i = chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], chain->positions[i + 1]));
            chain->positions[i] = vec3_add(chain->positions[i + 1], vec3_scale(direction, chain->lengths[i]));
        }
        
        // Backward reaching
        chain->positions[0] = root;
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], chain->positions[i - 1]));
            chain->positions[i] = vec3_add(chain->positions[i - 1], vec3_scale(direction, chain->lengths[i - 1]));
        }
        
        // Check convergence
        float distance = vec3_distance(chain->positions[chain->joint_count - 1], target_pos);
        if (distance < tolerance) {
            return true;
        }
    }
    
    return false;
}

Vec3 fabrik_get_joint_position(uint32_t chain_id, uint32_t joint_index) {
    if (!g_solver.initialized || chain_id >= g_solver.chain_count || 
        joint_index >= g_solver.chains[chain_id].joint_count) {
        return (Vec3){0, 0, 0};
    }
    return g_solver.chains[chain_id].positions[joint_index];
}

void fabrik_add_constraint(uint32_t chain_id, uint32_t joint_index, float min_angle, float max_angle) {
    if (!g_solver.initialized || chain_id >= g_solver.chain_count || 
        joint_index >= g_solver.chains[chain_id].joint_count) {
        return;
    }
    
    FABRIKChain* chain = &g_solver.chains[chain_id];
    chain->min_angles[joint_index] = min_angle;
    chain->max_angles[joint_index] = max_angle;
    chain->has_constraints[joint_index] = true;
}