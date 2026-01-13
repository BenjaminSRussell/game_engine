#include "include/common.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Advanced FABRIK implementation with constraints and multiple chains
#define MAX_CHAINS 32
#define MAX_JOINTS_PER_CHAIN 32
#define MAX_ITERATIONS 20

typedef struct {
    Vec3 original_positions[MAX_JOINTS_PER_CHAIN];
    Vec3 positions[MAX_JOINTS_PER_CHAIN];
    float lengths[MAX_JOINTS_PER_CHAIN - 1];
    uint32_t joint_count;
    float total_length;
    bool active;
} FABRIKChain;

typedef struct {
    Vec3 axis;
    float min_angle;
    float max_angle;
    bool enabled;
} JointConstraint;

typedef struct {
    FABRIKChain chains[MAX_CHAINS];
    JointConstraint constraints[MAX_CHAINS][MAX_JOINTS_PER_CHAIN];
    uint32_t chain_count;
    bool initialized;
    float tolerance;
} FABRIKSolver;

static FABRIKSolver g_fabrik_solver = {0};

void fabrik_init(void) {
    memset(&g_fabrik_solver, 0, sizeof(FABRIKSolver));
    g_fabrik_solver.tolerance = 0.001f;
    g_fabrik_solver.initialized = true;
}

void fabrik_solve_stub(void *chain, float target[3], int iterations) {
    if (!chain || !target || iterations <= 0) return;
    
    // Cast to chain structure
    FABRIKChain* fabrik_chain = (FABRIKChain*)chain;
    if (!fabrik_chain->active) return;
    
    Vec3 target_pos = {target[0], target[1], target[2]};
    Vec3 root_pos = fabrik_chain->positions[0];
    
    // Check if target is reachable
    float target_distance = vec3_distance(root_pos, target_pos);
    if (target_distance > fabrik_chain->total_length) {
        // Target unreachable - stretch towards it
        Vec3 direction = vec3_normalize(vec3_sub(target_pos, root_pos));
        for (uint32_t i = 1; i < fabrik_chain->joint_count; i++) {
            fabrik_chain->positions[i] = vec3_add(root_pos, vec3_scale(direction, 
                fabrik_chain->lengths[i-1] + (i > 1 ? fabrik_chain->lengths[i-2] : 0)));
        }
        return;
    }
    
    // FABRIK algorithm iterations
    for (int iter = 0; iter < iterations && iter < MAX_ITERATIONS; iter++) {
        // Forward reaching
        fabrik_chain->positions[fabrik_chain->joint_count - 1] = target_pos;
        for (int i = fabrik_chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(fabrik_chain->positions[i], 
                                                      fabrik_chain->positions[i + 1]));
            fabrik_chain->positions[i] = vec3_add(fabrik_chain->positions[i + 1], 
                                                 vec3_scale(direction, fabrik_chain->lengths[i]));
        }
        
        // Backward reaching
        fabrik_chain->positions[0] = root_pos;
        for (uint32_t i = 1; i < fabrik_chain->joint_count; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(fabrik_chain->positions[i], 
                                                      fabrik_chain->positions[i - 1]));
            fabrik_chain->positions[i] = vec3_add(fabrik_chain->positions[i - 1], 
                                                 vec3_scale(direction, fabrik_chain->lengths[i - 1]));
        }
        
        // Check convergence
        float error = vec3_distance(fabrik_chain->positions[fabrik_chain->joint_count - 1], target_pos);
        if (error < g_fabrik_solver.tolerance) {
            break;
        }
    }
}

void fabrik_add_constraint(void *chain, int joint_index, float min_angle,
                           float max_angle) {
    if (!chain || joint_index < 0) return;
    
    FABRIKChain* fabrik_chain = (FABRIKChain*)chain;
    if (joint_index >= (int)fabrik_chain->joint_count) return;
    
    // Find which chain this belongs to and add constraint
    for (uint32_t i = 0; i < g_fabrik_solver.chain_count; i++) {
        if (&g_fabrik_solver.chains[i] == fabrik_chain) {
            g_fabrik_solver.constraints[i][joint_index].axis = (Vec3){0, 1, 0}; // Y-axis constraint
            g_fabrik_solver.constraints[i][joint_index].min_angle = min_angle;
            g_fabrik_solver.constraints[i][joint_index].max_angle = max_angle;
            g_fabrik_solver.constraints[i][joint_index].enabled = true;
            break;
        }
    }
}

uint32_t fabrik_create_chain(const Vec3* positions, uint32_t joint_count) {
    if (!g_fabrik_solver.initialized || g_fabrik_solver.chain_count >= MAX_CHAINS ||
        !positions || joint_count < 2 || joint_count > MAX_JOINTS_PER_CHAIN) {
        return UINT32_MAX;
    }
    
    uint32_t chain_id = g_fabrik_solver.chain_count++;
    FABRIKChain* chain = &g_fabrik_solver.chains[chain_id];
    
    memcpy(chain->original_positions, positions, joint_count * sizeof(Vec3));
    memcpy(chain->positions, positions, joint_count * sizeof(Vec3));
    chain->joint_count = joint_count;
    chain->active = true;
    
    // Calculate segment lengths and total length
    chain->total_length = 0.0f;
    for (uint32_t i = 0; i < joint_count - 1; i++) {
        Vec3 diff = vec3_sub(positions[i + 1], positions[i]);
        chain->lengths[i] = vec3_length(diff);
        chain->total_length += chain->lengths[i];
    }
    
    // Initialize constraints as disabled
    for (uint32_t i = 0; i < joint_count; i++) {
        g_fabrik_solver.constraints[chain_id][i].enabled = false;
    }
    
    return chain_id;
}

bool fabrik_solve_chain_id(uint32_t chain_id, const Vec3* target, int max_iterations) {
    if (!g_fabrik_solver.initialized || chain_id >= g_fabrik_solver.chain_count || !target) {
        return false;
    }
    
    FABRIKChain* chain = &g_fabrik_solver.chains[chain_id];
    if (!chain->active) return false;
    
    float target_array[3] = {target->x, target->y, target->z};
    fabrik_solve_stub(chain, target_array, max_iterations);
    
    return true;
}

Vec3 fabrik_get_joint_position(uint32_t chain_id, uint32_t joint_index) {
    if (!g_fabrik_solver.initialized || chain_id >= g_fabrik_solver.chain_count ||
        joint_index >= g_fabrik_solver.chains[chain_id].joint_count) {
        return (Vec3){0, 0, 0};
    }
    
    return g_fabrik_solver.chains[chain_id].positions[joint_index];
}

void fabrik_reset_chain(uint32_t chain_id) {
    if (!g_fabrik_solver.initialized || chain_id >= g_fabrik_solver.chain_count) {
        return;
    }
    
    FABRIKChain* chain = &g_fabrik_solver.chains[chain_id];
    memcpy(chain->positions, chain->original_positions, chain->joint_count * sizeof(Vec3));
}
