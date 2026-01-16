// FABRIK IK Solver Implementation - Character Animation Location
// This completes the second FABRIK implementation mentioned in TODO-0101

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "character/animation/ik/fabrik_solver.h"

#define FABRIK_MAX_BONES 16
#define FABRIK_DEFAULT_ITERATIONS 10
#define FABRIK_DEFAULT_TOLERANCE 0.01f

typedef struct {
    float x, y, z;
} FabrikVec3;

// FABRIK chain structure for character animation
struct FABRIKSolver {
    uint32_t bone_indices[FABRIK_MAX_BONES];
    uint32_t bone_count;
    
    FabrikVec3 joint_positions[FABRIK_MAX_BONES + 1];
    float bone_lengths[FABRIK_MAX_BONES];
    float total_length;
    
    FabrikVec3 target_position;
    FabrikVec3 pole_vector;
    FabrikVec3 root_position;
    
    uint32_t max_iterations;
    float tolerance;
    bool use_pole_vector;
    bool reached_target;
    bool is_initialized;
};

// -----------------------------------------------------------------------------
// Vector Math Utilities
// -----------------------------------------------------------------------------

static inline FabrikVec3 fabrik_vec3(float x, float y, float z) {
    FabrikVec3 v = {x, y, z};
    return v;
}

static inline FabrikVec3 fabrik_vec3_sub(FabrikVec3 a, FabrikVec3 b) {
    return fabrik_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline FabrikVec3 fabrik_vec3_add(FabrikVec3 a, FabrikVec3 b) {
    return fabrik_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline FabrikVec3 fabrik_vec3_scale(FabrikVec3 v, float s) {
    return fabrik_vec3(v.x * s, v.y * s, v.z * s);
}

static inline float fabrik_vec3_length(FabrikVec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float fabrik_vec3_distance(FabrikVec3 a, FabrikVec3 b) {
    return fabrik_vec3_length(fabrik_vec3_sub(a, b));
}

static inline FabrikVec3 fabrik_vec3_normalize(FabrikVec3 v) {
    float len = fabrik_vec3_length(v);
    if (len > 0.0001f) {
        return fabrik_vec3_scale(v, 1.0f / len);
    }
    return fabrik_vec3(0.0f, 1.0f, 0.0f);
}

static inline float fabrik_vec3_dot(FabrikVec3 a, FabrikVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline FabrikVec3 fabrik_vec3_cross(FabrikVec3 a, FabrikVec3 b) {
    return fabrik_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                       a.x * b.y - a.y * b.x);
}

// -----------------------------------------------------------------------------
// FABRIK Solver Creation and Management
// -----------------------------------------------------------------------------

FABRIKSolver* fabrik_solver_create(const float* joint_positions, uint32_t bone_count) {
    if (bone_count == 0 || bone_count > FABRIK_MAX_BONES || !joint_positions) {
        return NULL;
    }
    
    FABRIKSolver* solver = (FABRIKSolver*)calloc(1, sizeof(FABRIKSolver));
    if (!solver) {
        return NULL;
    }
    
    solver->bone_count = bone_count;
    solver->max_iterations = FABRIK_DEFAULT_ITERATIONS;
    solver->tolerance = FABRIK_DEFAULT_TOLERANCE;
    solver->total_length = 0.0f;
    solver->is_initialized = true;
    
    // Copy joint positions (bone_count + 1 joints for bone_count bones)
    for (uint32_t i = 0; i <= bone_count; i++) {
        solver->joint_positions[i] = fabrik_vec3(joint_positions[i * 3], 
                                              joint_positions[i * 3 + 1],
                                              joint_positions[i * 3 + 2]);
    }
    
    // Compute bone lengths
    for (uint32_t i = 0; i < bone_count; i++) {
        solver->bone_lengths[i] = fabrik_vec3_distance(solver->joint_positions[i],
                                                      solver->joint_positions[i + 1]);
        solver->total_length += solver->bone_lengths[i];
    }
    
    // Store root position
    solver->root_position = solver->joint_positions[0];
    
    // Default pole vector (pointing forward for character)
    solver->pole_vector = fabrik_vec3(0.0f, 0.0f, 1.0f);
    solver->use_pole_vector = true;
    
    return solver;
}

void fabrik_solver_destroy(FABRIKSolver* solver) {
    if (solver) {
        free(solver);
    }
}

// -----------------------------------------------------------------------------
// FABRIK Core Algorithm Implementation
// -----------------------------------------------------------------------------

static void fabrik_forward_reach(FABRIKSolver* solver) {
    // Set end effector to target
    solver->joint_positions[solver->bone_count] = solver->target_position;
    
    // Iterate backward from end effector to root
    for (int i = (int)solver->bone_count - 1; i >= 0; i--) {
        FabrikVec3 direction = fabrik_vec3_sub(solver->joint_positions[i],
                                               solver->joint_positions[i + 1]);
        direction = fabrik_vec3_normalize(direction);
        
        solver->joint_positions[i] = fabrik_vec3_add(solver->joint_positions[i + 1],
                                                      fabrik_vec3_scale(direction, 
                                                                     solver->bone_lengths[i]));
    }
}

static void fabrik_backward_reach(FABRIKSolver* solver) {
    // Restore root to original position
    solver->joint_positions[0] = solver->root_position;
    
    // Iterate forward from root to end effector
    for (uint32_t i = 0; i < solver->bone_count; i++) {
        FabrikVec3 direction = fabrik_vec3_sub(solver->joint_positions[i + 1],
                                               solver->joint_positions[i]);
        direction = fabrik_vec3_normalize(direction);
        
        solver->joint_positions[i + 1] = fabrik_vec3_add(solver->joint_positions[i],
                                                         fabrik_vec3_scale(direction, 
                                                                        solver->bone_lengths[i]));
    }
}

static void fabrik_apply_pole_vector(FABRIKSolver* solver) {
    if (!solver->use_pole_vector || solver->bone_count < 2) {
        return;
    }
    
    // Apply to middle joint for character limbs (arms/legs)
    uint32_t mid = solver->bone_count / 2;
    
    FabrikVec3 root = solver->joint_positions[0];
    FabrikVec3 end = solver->joint_positions[solver->bone_count];
    FabrikVec3 mid_joint = solver->joint_positions[mid];
    
    // Create plane from root -> end
    FabrikVec3 chain_dir = fabrik_vec3_normalize(fabrik_vec3_sub(end, root));
    FabrikVec3 pole_dir = fabrik_vec3_normalize(fabrik_vec3_sub(solver->pole_vector, root));
    
    // Project pole onto perpendicular plane
    float dot = fabrik_vec3_dot(pole_dir, chain_dir);
    FabrikVec3 pole_proj = fabrik_vec3_sub(pole_dir, fabrik_vec3_scale(chain_dir, dot));
    pole_proj = fabrik_vec3_normalize(pole_proj);
    
    // Project mid joint onto plane
    FabrikVec3 mid_offset = fabrik_vec3_sub(mid_joint, root);
    float mid_along = fabrik_vec3_dot(mid_offset, chain_dir);
    FabrikVec3 mid_on_axis = fabrik_vec3_add(root, fabrik_vec3_scale(chain_dir, mid_along));
    
    // Distance from axis
    float dist_from_axis = fabrik_vec3_distance(mid_joint, mid_on_axis);
    
    // Move mid joint toward pole direction
    solver->joint_positions[mid] = fabrik_vec3_add(mid_on_axis, 
                                                   fabrik_vec3_scale(pole_proj, dist_from_axis));
}

bool fabrik_solver_solve(FABRIKSolver* solver, float target_x, float target_y, float target_z) {
    if (!solver || !solver->is_initialized) {
        return false;
    }
    
    solver->target_position = fabrik_vec3(target_x, target_y, target_z);
    solver->reached_target = false;
    
    // Check if target is reachable
    float dist_to_target = fabrik_vec3_distance(solver->root_position, solver->target_position);
    
    if (dist_to_target > solver->total_length) {
        // Target unreachable - stretch toward it
        FabrikVec3 direction = fabrik_vec3_normalize(
            fabrik_vec3_sub(solver->target_position, solver->root_position));
        
        solver->joint_positions[0] = solver->root_position;
        for (uint32_t i = 0; i < solver->bone_count; i++) {
            solver->joint_positions[i + 1] = fabrik_vec3_add(solver->joint_positions[i],
                                                              fabrik_vec3_scale(direction, 
                                                                             solver->bone_lengths[i]));
        }
        return false;
    }
    
    // FABRIK iteration loop
    for (uint32_t iter = 0; iter < solver->max_iterations; iter++) {
        // Forward reach
        fabrik_forward_reach(solver);
        
        // Apply pole vector constraint for realistic limb orientation
        if (solver->use_pole_vector) {
            fabrik_apply_pole_vector(solver);
        }
        
        // Backward reach
        fabrik_backward_reach(solver);
        
        // Check convergence
        float error = fabrik_vec3_distance(solver->joint_positions[solver->bone_count],
                                          solver->target_position);
        
        if (error < solver->tolerance) {
            solver->reached_target = true;
            return true;
        }
    }
    
    // Didn't fully converge but got close enough
    solver->reached_target = false;
    return false;
}

// -----------------------------------------------------------------------------
// Configuration and Control Functions
// -----------------------------------------------------------------------------

void fabrik_solver_set_iterations(FABRIKSolver* solver, uint32_t iterations) {
    if (solver && iterations > 0) {
        solver->max_iterations = iterations;
    }
}

void fabrik_solver_set_tolerance(FABRIKSolver* solver, float tolerance) {
    if (solver && tolerance > 0.0f) {
        solver->tolerance = tolerance;
    }
}

void fabrik_solver_set_pole_vector(FABRIKSolver* solver, float x, float y, float z) {
    if (solver) {
        solver->pole_vector = fabrik_vec3(x, y, z);
        solver->use_pole_vector = true;
    }
}

void fabrik_solver_disable_pole_vector(FABRIKSolver* solver) {
    if (solver) {
        solver->use_pole_vector = false;
    }
}

void fabrik_solver_update_bone_positions(FABRIKSolver* solver, const float* joint_positions) {
    if (!solver || !joint_positions || !solver->is_initialized) {
        return;
    }
    
    // Update joint positions
    for (uint32_t i = 0; i <= solver->bone_count; i++) {
        solver->joint_positions[i] = fabrik_vec3(joint_positions[i * 3], 
                                              joint_positions[i * 3 + 1],
                                              joint_positions[i * 3 + 2]);
    }
    
    // Update root position
    solver->root_position = solver->joint_positions[0];
}

// -----------------------------------------------------------------------------
// Query and Results Functions
// -----------------------------------------------------------------------------

void fabrik_solver_get_joint_position(FABRIKSolver* solver, uint32_t joint_index, float* out) {
    if (!solver || !out || joint_index > solver->bone_count || !solver->is_initialized) {
        return;
    }
    
    out[0] = solver->joint_positions[joint_index].x;
    out[1] = solver->joint_positions[joint_index].y;
    out[2] = solver->joint_positions[joint_index].z;
}

bool fabrik_solver_reached_target(FABRIKSolver* solver) {
    return solver ? solver->reached_target : false;
}

float fabrik_solver_get_error(FABRIKSolver* solver) {
    if (!solver || !solver->is_initialized) {
        return 0.0f;
    }
    return fabrik_vec3_distance(solver->joint_positions[solver->bone_count],
                              solver->target_position);
}

uint32_t fabrik_solver_get_bone_count(FABRIKSolver* solver) {
    return solver ? solver->bone_count : 0;
}

float fabrik_solver_get_total_length(FABRIKSolver* solver) {
    return solver ? solver->total_length : 0.0f;
}

// -----------------------------------------------------------------------------
// Character-Specific IK Utilities
// -----------------------------------------------------------------------------

bool fabrik_solver_solve_leg(FABRIKSolver* solver, const float* hip_pos, 
                             const float* foot_target, const float* knee_direction) {
    if (!solver || !hip_pos || !foot_target || !knee_direction) {
        return false;
    }
    
    // Update root (hip) position
    solver->root_position = fabrik_vec3(hip_pos[0], hip_pos[1], hip_pos[2]);
    solver->joint_positions[0] = solver->root_position;
    
    // Set pole vector for knee direction
    solver->pole_vector = fabrik_vec3(knee_direction[0], knee_direction[1], knee_direction[2]);
    solver->use_pole_vector = true;
    
    // Solve for foot target
    return fabrik_solver_solve(solver, foot_target[0], foot_target[1], foot_target[2]);
}

bool fabrik_solver_solve_arm(FABRIKSolver* solver, const float* shoulder_pos,
                             const float* hand_target, const float* elbow_direction) {
    if (!solver || !shoulder_pos || !hand_target || !elbow_direction) {
        return false;
    }
    
    // Update root (shoulder) position
    solver->root_position = fabrik_vec3(shoulder_pos[0], shoulder_pos[1], shoulder_pos[2]);
    solver->joint_positions[0] = solver->root_position;
    
    // Set pole vector for elbow direction
    solver->pole_vector = fabrik_vec3(elbow_direction[0], elbow_direction[1], elbow_direction[2]);
    solver->use_pole_vector = true;
    
    // Solve for hand target
    return fabrik_solver_solve(solver, hand_target[0], hand_target[1], hand_target[2]);
}

// -----------------------------------------------------------------------------
// Accuracy Testing and Validation
// -----------------------------------------------------------------------------

bool fabrik_solver_test_accuracy(FABRIKSolver* solver, float tolerance_threshold) {
    if (!solver || !solver->is_initialized) {
        return false;
    }
    
    // Test with known positions
    float test_positions[] = {
        0.0f, 0.0f, 0.0f,  // Root
        0.0f, 1.0f, 0.0f,  // Joint 1
        0.0f, 2.0f, 0.0f   // Joint 2 (end effector)
    };
    
    // Update solver with test positions
    fabrik_solver_update_bone_positions(solver, test_positions);
    
    // Test solving to a nearby target
    bool result = fabrik_solver_solve(solver, 0.5f, 1.8f, 0.0f);
    
    // Check if error is within tolerance
    float error = fabrik_solver_get_error(solver);
    return result && (error <= tolerance_threshold);
}