/*
 * ik_consolidated.h
 * Consolidated IK Solver Interface
 * 
 * Unified interface for all IK solvers including:
 * - FABRIK (Forward And Backward Reaching Inverse Kinematics)
 * - CCD (Cyclic Coordinate Descent)
 * - Two-Bone IK
 * - Look-at IK
 */

#ifndef ANIMATION_IK_CONSOLIDATED_H
#define ANIMATION_IK_CONSOLIDATED_H

#include <stdint.h>
#include <stdbool.h>
#include "engine/include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    IK_SOLVER_FABRIK = 0,
    IK_SOLVER_CCD,
    IK_SOLVER_TWO_BONE,
    IK_SOLVER_LOOK_AT,
    IK_SOLVER_COUNT
} IKSolverType;

typedef struct {
    Vec3 positions[32];
    float lengths[31];
    uint32_t joint_count;
    float total_length;
    bool active;
} IKChain;

typedef struct {
    Vec3 axis;
    float min_angle;
    float max_angle;
    bool enabled;
} IKConstraint;

typedef struct {
    IKChain chains[64];
    IKConstraint constraints[64][32];
    uint32_t chain_count;
    bool initialized;
    float tolerance;
    int max_iterations;
} IKSolver;

typedef struct {
    uint32_t chain_id;
    IKSolverType solver_type;
    bool success;
    float error_distance;
    int iterations_used;
} IKSolveResult;

/* ============================================================================
 * UNIFIED API
 * ============================================================================ */

/* System initialization */
int ik_system_init(void);
void ik_system_shutdown(void);
bool ik_system_is_initialized(void);

/* Chain management */
uint32_t ik_create_chain(IKSolverType solver_type, const Vec3* positions, uint32_t joint_count);
bool ik_destroy_chain(uint32_t chain_id);
bool ik_is_chain_valid(uint32_t chain_id);
uint32_t ik_get_chain_count(void);

/* Solving */
IKSolveResult ik_solve_to_target(uint32_t chain_id, const Vec3* target);
IKSolveResult ik_solve_with_iterations(uint32_t chain_id, const Vec3* target, int max_iterations);

/* Constraints */
bool ik_add_constraint(uint32_t chain_id, uint32_t joint_index, const Vec3* axis, float min_angle, float max_angle);
bool ik_remove_constraint(uint32_t chain_id, uint32_t joint_index);
bool ik_has_constraint(uint32_t chain_id, uint32_t joint_index);

/* State access */
Vec3 ik_get_joint_position(uint32_t chain_id, uint32_t joint_index);
bool ik_set_joint_position(uint32_t chain_id, uint32_t joint_index, const Vec3* position);
uint32_t ik_get_joint_count(uint32_t chain_id);
float ik_get_chain_length(uint32_t chain_id);

/* Utility */
void ik_reset_chain(uint32_t chain_id);
void ik_reset_all_chains(void);
float ik_get_distance_to_target(uint32_t chain_id, const Vec3* target);
bool ik_is_target_reachable(uint32_t chain_id, const Vec3* target);

/* Statistics */
void ik_get_solver_stats(IKSolverType solver_type, uint32_t* total_solves, uint32_t* successful_solves, float* average_error);
void ik_reset_solver_stats(IKSolverType solver_type);

/* Debug */
void ik_debug_print_chain(uint32_t chain_id);
void ik_debug_print_all_chains(void);
void ik_debug_print_solver_stats(void);

/* ============================================================================
 * SOLVER-SPECIFIC API (for advanced usage)
 * ============================================================================ */

/* FABRIK specific */
uint32_t ik_fabrik_create_chain(const Vec3* positions, uint32_t joint_count);
bool ik_fabrik_solve_chain(uint32_t chain_id, const Vec3* target, int max_iterations);

/* CCD specific */
uint32_t ik_ccd_create_chain(const Vec3* positions, uint32_t joint_count);
bool ik_ccd_solve_chain(uint32_t chain_id, const Vec3* target, int max_iterations);

/* Two-bone specific */
uint32_t ik_two_bone_create_chain(const Vec3* positions);
bool ik_two_bone_solve_chain(uint32_t chain_id, const Vec3* target, const Vec3* pole_vector);

/* Look-at specific */
uint32_t ik_look_at_create_chain(const Vec3* positions, uint32_t joint_count);
bool ik_look_at_solve_chain(uint32_t chain_id, const Vec3* target, const Vec3* up_vector);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_IK_CONSOLIDATED_H */
