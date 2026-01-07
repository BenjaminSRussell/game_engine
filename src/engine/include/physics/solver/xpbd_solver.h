#pragma once

#include "include/core/types.h"
#include "include/math/vec3.h"
#include "include/math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct XPBDSolver XPBDSolver;
typedef struct RigidBody RigidBody;
typedef struct XPBDConstraint XPBDConstraint;

// Constraint types
typedef enum {
    XPBD_CONSTRAINT_DISTANCE,
    XPBD_CONSTRAINT_ANGLE,
    XPBD_CONSTRAINT_TWIST,
    XPBD_CONSTRAINT_BENDING,
    XPBD_CONSTRAINT_CONTACT,
    XPBD_CONSTRAINT_MOTOR,
    XPBD_CONSTRAINT_CUSTOM
} XPBDConstraintType;

// Constraint compliance (inverse stiffness)
typedef struct {
    f32 compliance;           // C (inverse stiffness, larger = softer)
    f32 damping;             // Damping coefficient
    f32 stabilization_factor; // For Baumgarte stabilization
} XPBDCompliance;

// Lambda accumulation for warm starting
typedef struct {
    f32 lambda;              // Accumulated Lagrange multiplier
    f32 dlambda;             // Change in lambda
} XPBDLambda;

// XPBD constraint structure
typedef struct XPBDConstraint {
    XPBDConstraintType type;
    u32 body_a_id;
    u32 body_b_id;
    Vec3 r_a;                // Local attachment point A
    Vec3 r_b;                // Local attachment point B
    f32 rest_distance;       // Rest value for constraint
    XPBDCompliance compliance;
    XPBDLambda lambda;
    f32 constraint_value;    // C(x) = 0 value
    Vec3 gradient;           // Gradient for force calculation
    bool active;
} XPBDConstraint;

// Solver configuration
typedef struct {
    u32 sub_steps;
    u32 constraint_iterations;
    f32 damping;
    f32 default_compliance;
    bool use_warm_starting;
    bool use_baumgarte;
    bool use_parallel;
    bool use_simd;
    f32 baumgarte_beta;      // Baumgarte stabilization parameter
    bool stabilize_contacts;
} XPBDSolverConfig;

/**
 * XPBDSolver: Extended Position Based Dynamics solver
 *
 * Properties:
 *   - Compliant constraint formulation (allows stretch/compression)
 *   - Second-order convergence (faster than PBD)
 *   - Configurable stiffness via compliance
 *   - Warm starting with lambda multiplier accumulation
 *   - Parallel constraint solving
 *   - SIMD-accelerated batching
 *   - Sub-stepping for stability
 *
 * Advantages over Sequential Impulse:
 *   - More intuitive stiffness control (compliance parameter)
 *   - Better constraint stabilization
 *   - Simpler to add new constraint types
 *   - Excellent for animation/visual effects
 *
 * Use cases:
 *   - Character animation
 *   - Cloth simulation
 *   - Hair dynamics
 *   - Particle constraints
 *   - Soft body simulation
 */

// ============================================================================
// Solver Creation and Configuration
// ============================================================================

/**
 * Create XPBD solver
 *
 * Args:
 *   config: Solver configuration
 *
 * Returns:
 *   New XPBDSolver (must be freed with xpbd_solver_destroy)
 */
XPBDSolver *xpbd_solver_create(XPBDSolverConfig config);

/**
 * Create with default configuration
 *
 * Returns:
 *   New solver with standard settings
 */
XPBDSolver *xpbd_solver_create_default(void);

/**
 * Destroy solver
 *
 * Args:
 *   solver: XPBDSolver to destroy (NULL-safe)
 */
void xpbd_solver_destroy(XPBDSolver *solver);

/**
 * Update solver configuration
 *
 * Args:
 *   solver: XPBDSolver
 *   config: New configuration
 */
void xpbd_solver_set_config(XPBDSolver *solver, XPBDSolverConfig config);

/**
 * Get current configuration
 *
 * Args:
 *   solver: XPBDSolver
 *
 * Returns:
 *   Current XPBDSolverConfig
 */
XPBDSolverConfig xpbd_solver_get_config(XPBDSolver *solver);

// ============================================================================
// Body Management
// ============================================================================

/**
 * Add body to solver
 *
 * Args:
 *   solver: XPBDSolver
 *   body: RigidBody to add
 *
 * Returns:
 *   Body ID in solver
 */
u32 xpbd_solver_add_body(XPBDSolver *solver, RigidBody *body);

/**
 * Remove body from solver
 *
 * Args:
 *   solver: XPBDSolver
 *   body: RigidBody to remove
 */
void xpbd_solver_remove_body(XPBDSolver *solver, RigidBody *body);

/**
 * Get body count
 *
 * Args:
 *   solver: XPBDSolver
 *
 * Returns:
 *   Number of bodies
 */
u32 xpbd_solver_body_count(XPBDSolver *solver);

// ============================================================================
// Constraint Management
// ============================================================================

/**
 * Create and add distance constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   body_a_id: First body ID
 *   body_b_id: Second body ID
 *   rest_distance: Desired distance between bodies
 *   compliance: Inverse stiffness (larger = softer)
 *
 * Returns:
 *   Constraint ID
 */
u32 xpbd_solver_add_distance_constraint(XPBDSolver *solver, u32 body_a_id, u32 body_b_id, f32 rest_distance, XPBDCompliance compliance);

/**
 * Create angle constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   body_a_id: First body
 *   body_b_id: Second body
 *   rest_angle: Desired angle in radians
 *   compliance: Inverse stiffness
 *
 * Returns:
 *   Constraint ID
 */
u32 xpbd_solver_add_angle_constraint(XPBDSolver *solver, u32 body_a_id, u32 body_b_id, f32 rest_angle, XPBDCompliance compliance);

/**
 * Create twist constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   body_a_id: First body
 *   body_b_id: Second body
 *   rest_twist: Desired twist angle
 *   compliance: Inverse stiffness
 *
 * Returns:
 *   Constraint ID
 */
u32 xpbd_solver_add_twist_constraint(XPBDSolver *solver, u32 body_a_id, u32 body_b_id, f32 rest_twist, XPBDCompliance compliance);

/**
 * Create bending constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   body_a_id: First body
 *   body_b_id: Second body
 *   rest_curvature: Desired curvature
 *   compliance: Inverse stiffness
 *
 * Returns:
 *   Constraint ID
 */
u32 xpbd_solver_add_bending_constraint(XPBDSolver *solver, u32 body_a_id, u32 body_b_id, f32 rest_curvature, XPBDCompliance compliance);

/**
 * Create contact constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   body_a_id: First body
 *   body_b_id: Second body
 *   contact_point: Contact location
 *   normal: Contact normal
 *   depth: Penetration depth
 *   compliance: Inverse stiffness
 *
 * Returns:
 *   Constraint ID
 */
u32 xpbd_solver_add_contact_constraint(XPBDSolver *solver, u32 body_a_id, u32 body_b_id, Vec3 contact_point, Vec3 normal, f32 depth, XPBDCompliance compliance);

/**
 * Remove constraint by ID
 *
 * Args:
 *   solver: XPBDSolver
 *   constraint_id: Constraint to remove
 *
 * Returns:
 *   true if constraint removed
 */
bool xpbd_solver_remove_constraint(XPBDSolver *solver, u32 constraint_id);

/**
 * Get constraint count
 *
 * Args:
 *   solver: XPBDSolver
 *
 * Returns:
 *   Number of active constraints
 */
u32 xpbd_solver_constraint_count(XPBDSolver *solver);

/**
 * Get constraint by ID
 *
 * Args:
 *   solver: XPBDSolver
 *   constraint_id: Constraint ID
 *
 * Returns:
 *   Pointer to XPBDConstraint or NULL
 */
XPBDConstraint *xpbd_solver_get_constraint(XPBDSolver *solver, u32 constraint_id);

// ============================================================================
// Compliance and Stiffness
// ============================================================================

/**
 * Set constraint stiffness (0-1, where 1 is very stiff)
 *
 * Args:
 *   constraint: XPBDConstraint
 *   stiffness: Stiffness from 0.0 (soft) to 1.0 (rigid)
 *
 * Note: Converts to compliance internally
 */
void xpbd_constraint_set_stiffness(XPBDConstraint *constraint, f32 stiffness);

/**
 * Set constraint compliance (inverse stiffness)
 *
 * Args:
 *   constraint: XPBDConstraint
 *   compliance: Compliance value (larger = softer)
 */
void xpbd_constraint_set_compliance(XPBDConstraint *constraint, f32 compliance);

/**
 * Add damping to constraint
 *
 * Args:
 *   constraint: XPBDConstraint
 *   damping: Damping coefficient
 */
void xpbd_constraint_set_damping(XPBDConstraint *constraint, f32 damping);

/**
 * Get current constraint value (distance/angle error)
 *
 * Args:
 *   constraint: XPBDConstraint
 *
 * Returns:
 *   Current C(x) value
 */
f32 xpbd_constraint_value(XPBDConstraint *constraint);

/**
 * Tune constraint for desired frequency
 *
 * Args:
 *   constraint: XPBDConstraint
 *   frequency_hz: Desired oscillation frequency
 *   dt: Time step
 *
 * Note: Automatically sets compliance for target frequency
 */
void xpbd_constraint_tune_frequency(XPBDConstraint *constraint, f32 frequency_hz, f32 dt);

// ============================================================================
// Constraint Solving
// ============================================================================

/**
 * Solve constraints once
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Time step
 *
 * Note: Single constraint iteration
 */
void xpbd_solver_solve_constraints(XPBDSolver *solver, f32 dt);

/**
 * Solve with multiple iterations
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Time step
 *   iterations: Number of iterations
 */
void xpbd_solver_solve_constraints_iterations(XPBDSolver *solver, f32 dt, u32 iterations);

/**
 * Solve single constraint
 *
 * Args:
 *   solver: XPBDSolver
 *   constraint: XPBDConstraint to solve
 *   dt: Time step
 */
void xpbd_solver_solve_constraint(XPBDSolver *solver, XPBDConstraint *constraint, f32 dt);

/**
 * Compute constraint force
 *
 * Args:
 *   constraint: XPBDConstraint
 *   lambda: Lagrange multiplier
 *
 * Returns:
 *   Force vector
 */
Vec3 xpbd_solver_compute_constraint_force(XPBDConstraint *constraint, f32 lambda);

// ============================================================================
// Warm Starting
// ============================================================================

/**
 * Enable warm starting
 *
 * Args:
 *   solver: XPBDSolver
 *   enable: Whether to use warm starting
 */
void xpbd_solver_set_warm_starting(XPBDSolver *solver, bool enable);

/**
 * Accumulate lambda multiplier
 *
 * Args:
 *   constraint: XPBDConstraint
 *   dlambda: Change in lambda
 */
void xpbd_solver_accumulate_lambda(XPBDConstraint *constraint, f32 dlambda);

/**
 * Apply warm starting impulses
 *
 * Args:
 *   solver: XPBDSolver
 *   constraint: XPBDConstraint
 */
void xpbd_solver_apply_warm_start(XPBDSolver *solver, XPBDConstraint *constraint);

/**
 * Clear lambda accumulation
 *
 * Args:
 *   solver: XPBDSolver
 */
void xpbd_solver_clear_lambdas(XPBDSolver *solver);

/**
 * Scale lambda for sub-stepping
 *
 * Args:
 *   solver: XPBDSolver
 *   scale: Scaling factor
 */
void xpbd_solver_scale_lambdas(XPBDSolver *solver, f32 scale);

// ============================================================================
// Stabilization
// ============================================================================

/**
 * Enable Baumgarte stabilization
 *
 * Args:
 *   solver: XPBDSolver
 *   enable: Whether to use Baumgarte
 */
void xpbd_solver_set_baumgarte(XPBDSolver *solver, bool enable);

/**
 * Set Baumgarte parameters
 *
 * Args:
 *   solver: XPBDSolver
 *   beta: Stabilization factor (0.0-1.0)
 */
void xpbd_solver_set_baumgarte_beta(XPBDSolver *solver, f32 beta);

/**
 * Enable contact stabilization
 *
 * Args:
 *   solver: XPBDSolver
 *   enable: Whether to stabilize contacts
 */
void xpbd_solver_set_contact_stabilization(XPBDSolver *solver, bool enable);

// ============================================================================
// Sub-stepping
// ============================================================================

/**
 * Set number of sub-steps per frame
 *
 * Args:
 *   solver: XPBDSolver
 *   sub_steps: Number of sub-steps (1 = no sub-stepping)
 */
void xpbd_solver_set_sub_steps(XPBDSolver *solver, u32 sub_steps);

/**
 * Solve with sub-stepping
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Frame time step
 *
 * Note: Automatically divides into configured sub-steps
 */
void xpbd_solver_step(XPBDSolver *solver, f32 dt);

/**
 * Adaptive sub-stepping based on error
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Frame time step
 *   target_error: Target constraint error
 *
 * Returns:
 *   Number of sub-steps used
 */
u32 xpbd_solver_step_adaptive(XPBDSolver *solver, f32 dt, f32 target_error);

// ============================================================================
// Parallel Solving
// ============================================================================

/**
 * Enable parallel constraint solving
 *
 * Args:
 *   solver: XPBDSolver
 *   enable: Whether to use parallel
 */
void xpbd_solver_set_parallel(XPBDSolver *solver, bool enable);

/**
 * Set number of threads for parallel solving
 *
 * Args:
 *   solver: XPBDSolver
 *   num_threads: Number of threads (0 = auto-detect)
 */
void xpbd_solver_set_num_threads(XPBDSolver *solver, u32 num_threads);

/**
 * Solve constraints in parallel
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Time step
 *
 * Note: Distributes constraints across threads
 */
void xpbd_solver_solve_parallel(XPBDSolver *solver, f32 dt);

// ============================================================================
// SIMD Batching
// ============================================================================

/**
 * Enable SIMD batching
 *
 * Args:
 *   solver: XPBDSolver
 *   enable: Whether to use SIMD
 */
void xpbd_solver_set_simd(XPBDSolver *solver, bool enable);

/**
 * Solve compatible constraints with SIMD
 *
 * Args:
 *   solver: XPBDSolver
 *   dt: Time step
 *
 * Note: Processes multiple constraints in parallel
 */
void xpbd_solver_solve_simd(XPBDSolver *solver, f32 dt);

// ============================================================================
// Visualization and Diagnostics
// ============================================================================

/**
 * Get constraint error metrics
 *
 * Args:
 *   solver: XPBDSolver
 *   max_error: Output for maximum error
 *   avg_error: Output for average error
 */
void xpbd_solver_get_error_metrics(XPBDSolver *solver, f32 *max_error, f32 *avg_error);

/**
 * Get solver statistics
 *
 * Args:
 *   solver: XPBDSolver
 *   stats_out: Output buffer
 *   stats_size: Buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 xpbd_solver_get_statistics(XPBDSolver *solver, char *stats_out, u32 stats_size);

/**
 * Print diagnostic information
 *
 * Args:
 *   solver: XPBDSolver
 */
void xpbd_solver_print_diagnostics(XPBDSolver *solver);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Validate solver state
 *
 * Args:
 *   solver: XPBDSolver
 *
 * Returns:
 *   true if all constraints valid
 */
bool xpbd_solver_validate(XPBDSolver *solver);

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 xpbd_solver_run_tests(void);

/**
 * Test cloth simulation
 *
 * Args:
 *   width: Cloth grid width
 *   height: Cloth grid height
 *
 * Returns:
 *   true if cloth simulation stable
 */
bool xpbd_solver_test_cloth(u32 width, u32 height);

/**
 * Test hair dynamics
 *
 * Args:
 *   num_segments: Hair segments
 *
 * Returns:
 *   true if simulation stable
 */
bool xpbd_solver_test_hair(u32 num_segments);

#ifdef __cplusplus
}
#endif

#endif // XPBD_SOLVER_H
