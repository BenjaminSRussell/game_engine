/*
 * ik_consolidated.c
 * Consolidated IK Solver Implementation
 * 
 * Unified implementation for all IK solvers with performance optimization
 */

#include "ik_consolidated.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static IKSolver g_ik_solver = {0};

/* Statistics tracking */
static struct {
    uint32_t total_solves[IK_SOLVER_COUNT];
    uint32_t successful_solves[IK_SOLVER_COUNT];
    float total_error[IK_SOLVER_COUNT];
    uint32_t solve_count[IK_SOLVER_COUNT];
} g_ik_stats = {0};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static float vec3_distance_squared(const Vec3* a, const Vec3* b) {
    Vec3 diff = {a->x - b->x, a->y - b->y, a->z - b->z};
    return diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
}

static Vec3 vec3_normalize_safe(const Vec3* v) {
    float length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (length < 1e-6f) {
        return (Vec3){0, 1, 0}; // Default up vector
    }
    return (Vec3){v->x / length, v->y / length, v->z / length};
}

/* ============================================================================
 * FABRIK IMPLEMENTATION
 * ============================================================================ */

static bool ik_fabrik_solve_internal(IKChain* chain, const Vec3* target, int max_iterations, float tolerance) {
    if (!chain->active || !target) return false;
    
    Vec3 target_pos = *target;
    Vec3 root_pos = chain->positions[0];
    
    // Check if target is reachable
    float target_distance = vec3_distance(&root_pos, &target_pos);
    if (target_distance > chain->total_length) {
        // Target unreachable - stretch towards it
        Vec3 direction = vec3_normalize_safe(vec3_sub(&target_pos, &root_pos));
        float accumulated_length = 0.0f;
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            accumulated_length += chain->lengths[i-1];
            chain->positions[i] = vec3_add(&root_pos, vec3_scale(&direction, accumulated_length));
        }
        return false;
    }
    
    // FABRIK algorithm iterations
    for (int iter = 0; iter < max_iterations; iter++) {
        // Forward reaching
        chain->positions[chain->joint_count - 1] = target_pos;
        for (int i = chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize_safe(vec3_sub(chain->positions[i], chain->positions[i + 1]));
            chain->positions[i] = vec3_add(chain->positions[i + 1], vec3_scale(direction, chain->lengths[i]));
        }
        
        // Backward reaching
        chain->positions[0] = root_pos;
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            Vec3 direction = vec3_normalize_safe(vec3_sub(chain->positions[i], chain->positions[i - 1]));
            chain->positions[i] = vec3_add(chain->positions[i - 1], vec3_scale(direction, chain->lengths[i - 1]));
        }
        
        // Check convergence
        float error = vec3_distance(chain->positions[chain->joint_count - 1], target_pos);
        if (error < tolerance) {
            return true;
        }
    }
    
    return false;
}

/* ============================================================================
 * CCD IMPLEMENTATION
 * ============================================================================ */

static bool ik_ccd_solve_internal(IKChain* chain, const Vec3* target, int max_iterations, float tolerance) {
    if (!chain->active || !target) return false;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        for (uint32_t i = 0; i < chain->joint_count - 1; i++) {
            Vec3 end_effector = chain->positions[chain->joint_count - 1];
            Vec3 joint_pos = chain->positions[i];
            
            Vec3 to_end = vec3_sub(&end_effector, &joint_pos);
            Vec3 to_target = vec3_sub(target, &joint_pos);
            
            float to_end_len = vec3_length(&to_end);
            float to_target_len = vec3_length(&to_target);
            
            if (to_end_len < 1e-6f || to_target_len < 1e-6f) continue;
            
            // Calculate rotation angle
            float cos_angle = vec3_dot(&to_end, &to_target) / (to_end_len * to_target_len);
            cos_angle = fmaxf(-1.0f, fminf(1.0f, cos_angle));
            float angle = acosf(cos_angle);
            
            if (angle < tolerance) continue;
            
            // Apply rotation to all joints after i
            Vec3 rotation_axis = vec3_normalize_safe(vec3_cross(&to_end, &to_target));
            for (uint32_t j = i + 1; j < chain->joint_count; j++) {
                Vec3 to_joint = vec3_sub(&chain->positions[j], &joint_pos);
                chain->positions[j] = vec3_add(&joint_pos, vec3_rotate_around_axis(&to_joint, &rotation_axis, angle));
            }
        }
        
        // Check convergence
        float error = vec3_distance(&chain->positions[chain->joint_count - 1], target);
        if (error < tolerance) {
            return true;
        }
    }
    
    return false;
}

/* ============================================================================
 * TWO-BONE IK IMPLEMENTATION
 * ============================================================================ */

static bool ik_two_bone_solve_internal(IKChain* chain, const Vec3* target, const Vec3* pole_vector) {
    if (!chain->active || !target || chain->joint_count != 3) return false;
    
    Vec3 root = chain->positions[0];
    Vec3 joint = chain->positions[1];
    Vec3 end = chain->positions[2];
    
    float l1 = chain->lengths[0]; // Root to joint
    float l2 = chain->lengths[1]; // Joint to end
    
    Vec3 target_vec = vec3_sub(target, &root);
    float target_dist = vec3_length(&target_vec);
    
    // Check if target is reachable
    if (target_dist > l1 + l2) {
        // Stretch towards target
        Vec3 direction = vec3_normalize_safe(&target_vec);
        chain->positions[1] = vec3_add(&root, vec3_scale(&direction, l1));
        chain->positions[2] = vec3_add(&root, vec3_scale(&direction, l1 + l2));
        return false;
    }
    
    if (target_dist < fabsf(l1 - l2)) {
        // Target too close, fold chain
        Vec3 direction = vec3_normalize_safe(&target_vec);
        chain->positions[1] = vec3_add(&root, vec3_scale(&direction, l1));
        chain->positions[2] = vec3_add(&chain->positions[1], vec3_scale(&direction, l2));
        return false;
    }
    
    // Law of cosines to find joint angles
    float cos_angle2 = (l1*l1 + l2*l2 - target_dist*target_dist) / (2.0f * l1 * l2);
    cos_angle2 = fmaxf(-1.0f, fminf(1.0f, cos_angle2));
    float angle2 = acosf(cos_angle2);
    
    float cos_angle1 = (l1*l1 + target_dist*target_dist - l2*l2) / (2.0f * l1 * target_dist);
    cos_angle1 = fmaxf(-1.0f, fminf(1.0f, cos_angle1));
    float angle1 = acosf(cos_angle1);
    
    // Apply pole vector for elbow orientation
    Vec3 chain_dir = vec3_normalize_safe(&target_vec);
    Vec3 pole_dir = pole_vector ? *pole_vector : (Vec3){0, 1, 0};
    Vec3 cross = vec3_cross(&chain_dir, &pole_dir);
    Vec3 pole_proj = vec3_normalize_safe(vec3_cross(&cross, &chain_dir));
    
    // Position middle joint
    chain->positions[1] = vec3_add(&root, vec3_scale(&chain_dir, l1 * cosf(angle1)));
    chain->positions[1] = vec3_add(&chain->positions[1], vec3_scale(&pole_proj, l1 * sinf(angle1)));
    
    // Position end effector
    chain->positions[2] = *target;
    
    return true;
}

/* ============================================================================
 * UNIFIED API IMPLEMENTATION
 * ============================================================================ */

int ik_system_init(void) {
    memset(&g_ik_solver, 0, sizeof(IKSolver));
    memset(&g_ik_stats, 0, sizeof(g_ik_stats));
    g_ik_solver.tolerance = 0.001f;
    g_ik_solver.max_iterations = 20;
    g_ik_solver.initialized = true;
    return 0;
}

void ik_system_shutdown(void) {
    memset(&g_ik_solver, 0, sizeof(IKSolver));
    memset(&g_ik_stats, 0, sizeof(g_ik_stats));
}

bool ik_system_is_initialized(void) {
    return g_ik_solver.initialized;
}

uint32_t ik_create_chain(IKSolverType solver_type, const Vec3* positions, uint32_t joint_count) {
    if (!g_ik_solver.initialized || g_ik_solver.chain_count >= 64 ||
        !positions || joint_count < 2 || joint_count > 32) {
        return UINT32_MAX;
    }
    
    uint32_t chain_id = g_ik_solver.chain_count++;
    IKChain* chain = &g_ik_solver.chains[chain_id];
    
    memcpy(chain->positions, positions, joint_count * sizeof(Vec3));
    memcpy(chain->original_positions, positions, joint_count * sizeof(Vec3));
    chain->joint_count = joint_count;
    chain->active = true;
    
    // Calculate segment lengths and total length
    chain->total_length = 0.0f;
    for (uint32_t i = 0; i < joint_count - 1; i++) {
        Vec3 diff = vec3_sub(positions[i + 1], positions[i]);
        chain->lengths[i] = vec3_length(&diff);
        chain->total_length += chain->lengths[i];
    }
    
    // Initialize constraints as disabled
    for (uint32_t i = 0; i < joint_count; i++) {
        g_ik_solver.constraints[chain_id][i].enabled = false;
    }
    
    return chain_id;
}

IKSolveResult ik_solve_to_target(uint32_t chain_id, const Vec3* target) {
    return ik_solve_with_iterations(chain_id, target, g_ik_solver.max_iterations);
}

IKSolveResult ik_solve_with_iterations(uint32_t chain_id, const Vec3* target, int max_iterations) {
    IKSolveResult result = {0};
    
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count || !target) {
        result.success = false;
        return result;
    }
    
    IKChain* chain = &g_ik_solver.chains[chain_id];
    if (!chain->active) {
        result.success = false;
        return result;
    }
    
    result.chain_id = chain_id;
    result.iterations_used = max_iterations;
    
    // For now, default to FABRIK solver
    result.success = ik_fabrik_solve_internal(chain, target, max_iterations, g_ik_solver.tolerance);
    result.error_distance = vec3_distance(&chain->positions[chain->joint_count - 1], target);
    
    // Update statistics
    g_ik_stats.total_solves[IK_SOLVER_FABRIK]++;
    g_ik_stats.solve_count[IK_SOLVER_FABRIK]++;
    g_ik_stats.total_error[IK_SOLVER_FABRIK] += result.error_distance;
    if (result.success) {
        g_ik_stats.successful_solves[IK_SOLVER_FABRIK]++;
    }
    
    return result;
}

Vec3 ik_get_joint_position(uint32_t chain_id, uint32_t joint_index) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count ||
        joint_index >= g_ik_solver.chains[chain_id].joint_count) {
        return (Vec3){0, 0, 0};
    }
    return g_ik_solver.chains[chain_id].positions[joint_index];
}

bool ik_set_joint_position(uint32_t chain_id, uint32_t joint_index, const Vec3* position) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count ||
        joint_index >= g_ik_solver.chains[chain_id].joint_count || !position) {
        return false;
    }
    
    g_ik_solver.chains[chain_id].positions[joint_index] = *position;
    return true;
}

uint32_t ik_get_joint_count(uint32_t chain_id) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count) {
        return 0;
    }
    return g_ik_solver.chains[chain_id].joint_count;
}

float ik_get_chain_length(uint32_t chain_id) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count) {
        return 0.0f;
    }
    return g_ik_solver.chains[chain_id].total_length;
}

void ik_reset_chain(uint32_t chain_id) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count) {
        return;
    }
    
    IKChain* chain = &g_ik_solver.chains[chain_id];
    memcpy(chain->positions, chain->original_positions, chain->joint_count * sizeof(Vec3));
}

void ik_reset_all_chains(void) {
    if (!g_ik_solver.initialized) return;
    
    for (uint32_t i = 0; i < g_ik_solver.chain_count; i++) {
        ik_reset_chain(i);
    }
}

float ik_get_distance_to_target(uint32_t chain_id, const Vec3* target) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count || !target) {
        return -1.0f;
    }
    
    IKChain* chain = &g_ik_solver.chains[chain_id];
    return vec3_distance(&chain->positions[chain->joint_count - 1], target);
}

bool ik_is_target_reachable(uint32_t chain_id, const Vec3* target) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count || !target) {
        return false;
    }
    
    IKChain* chain = &g_ik_solver.chains[chain_id];
    Vec3 root = chain->positions[0];
    float distance = vec3_distance(&root, target);
    return distance <= chain->total_length;
}

void ik_get_solver_stats(IKSolverType solver_type, uint32_t* total_solves, uint32_t* successful_solves, float* average_error) {
    if (solver_type >= IK_SOLVER_COUNT) return;
    
    if (total_solves) *total_solves = g_ik_stats.total_solves[solver_type];
    if (successful_solves) *successful_solves = g_ik_stats.successful_solves[solver_type];
    if (average_error) {
        *average_error = g_ik_stats.solve_count[solver_type] > 0 ? 
            g_ik_stats.total_error[solver_type] / g_ik_stats.solve_count[solver_type] : 0.0f;
    }
}

void ik_reset_solver_stats(IKSolverType solver_type) {
    if (solver_type >= IK_SOLVER_COUNT) return;
    
    g_ik_stats.total_solves[solver_type] = 0;
    g_ik_stats.successful_solves[solver_type] = 0;
    g_ik_stats.total_error[solver_type] = 0.0f;
    g_ik_stats.solve_count[solver_type] = 0;
}

/* ============================================================================
 * DEBUG FUNCTIONS
 * ============================================================================ */

void ik_debug_print_chain(uint32_t chain_id) {
    if (!g_ik_solver.initialized || chain_id >= g_ik_solver.chain_count) {
        printf("Invalid chain ID: %u\n", chain_id);
        return;
    }
    
    IKChain* chain = &g_ik_solver.chains[chain_id];
    printf("Chain %u (%u joints):\n", chain_id, chain->joint_count);
    for (uint32_t i = 0; i < chain->joint_count; i++) {
        printf("  Joint %u: (%.3f, %.3f, %.3f)\n", i, 
               chain->positions[i].x, chain->positions[i].y, chain->positions[i].z);
    }
    printf("  Total length: %.3f\n", chain->total_length);
}

void ik_debug_print_all_chains(void) {
    if (!g_ik_solver.initialized) {
        printf("IK system not initialized\n");
        return;
    }
    
    printf("IK System - %u chains:\n", g_ik_solver.chain_count);
    for (uint32_t i = 0; i < g_ik_solver.chain_count; i++) {
        ik_debug_print_chain(i);
    }
}

void ik_debug_print_solver_stats(void) {
    const char* solver_names[] = {"FABRIK", "CCD", "Two-Bone", "Look-At"};
    
    printf("IK Solver Statistics:\n");
    for (int i = 0; i < IK_SOLVER_COUNT; i++) {
        uint32_t total = g_ik_stats.total_solves[i];
        uint32_t successful = g_ik_stats.successful_solves[i];
        float avg_error = g_ik_stats.solve_count[i] > 0 ? 
            g_ik_stats.total_error[i] / g_ik_stats.solve_count[i] : 0.0f;
        
        printf("  %s: %u solves, %u successful (%.1f%%), avg error: %.6f\n",
               solver_names[i], total, successful, 
               total > 0 ? (successful * 100.0f / total) : 0.0f, avg_error);
    }
}
