#pragma once

#include "../../core/types.h"
#include "../../math/vec3.h"
#include "../../math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct SequentialImpulseSolver SequentialImpulseSolver;
typedef struct RigidBody RigidBody;
typedef struct ContactManifold ContactManifold;
typedef struct VelocityConstraint VelocityConstraint;
typedef struct PositionConstraint PositionConstraint;

// Solver configuration
typedef struct {
    f32 gravity;
    u32 velocity_iterations;
    u32 position_iterations;
    f32 position_sleep_threshold;
    f32 velocity_sleep_threshold;
    bool use_warm_starting;
    bool use_simd;
    u32 max_bodies;
    u32 max_contacts;
} SISolverConfig;

// Contact constraint information
typedef struct {
    RigidBody *body_a;
    RigidBody *body_b;
    Vec3 contact_point;
    Vec3 normal;
    f32 depth;
    f32 friction;
    f32 restitution;
    bool touching;
} ContactInfo;

// Velocity constraint for impulse-based solving
typedef struct VelocityConstraint {
    u32 body_a_id;
    u32 body_b_id;
    Vec3 r_a;              // Contact point relative to A
    Vec3 r_b;              // Contact point relative to B
    Vec3 normal;
    Vec3 tangent[2];
    f32 normal_mass;
    f32 tangent_mass[2];
    f32 restitution;
    f32 friction;
    f32 normal_impulse;    // Accumulated impulse
    Vec3 tangent_impulse;  // Tangential impulse
    f32 relative_velocity;
} VelocityConstraint;

// Position constraint for position correction
typedef struct PositionConstraint {
    u32 body_a_id;
    u32 body_b_id;
    Vec3 r_a;
    Vec3 r_b;
    Vec3 normal;
    f32 penetration_depth;
    f32 effective_mass;
} PositionConstraint;

/**
 * SequentialImpulseSolver: Industry-standard constraint-based physics solver
 *
 * Algorithm:
 *   1. Velocity phase: Solve velocity constraints with accumulated impulses
 *   2. Position phase: Solve position constraints to fix penetrations
 *   3. Warm starting: Reuse previous impulses for faster convergence
 *   4. Sleeping: Deactivate bodies that aren't moving
 *
 * Properties:
 *   - O(n) time complexity per iteration
 *   - Stable and robust contact handling
 *   - Friction cone support
 *   - Configurable stiffness via iterations
 *   - SIMD-accelerated constraint solving
 *
 * Use cases:
 *   - Rigid body dynamics
 *   - Game physics engines
 *   - Constraint-based animation
 */

// ============================================================================
// Solver Creation and Configuration
// ============================================================================

/**
 * Create sequential impulse solver
 *
 * Args:
 *   config: Solver configuration
 *
 * Returns:
 *   New SequentialImpulseSolver (must be freed with si_solver_destroy)
 */
SequentialImpulseSolver *si_solver_create(SISolverConfig config);

/**
 * Create with default configuration
 *
 * Returns:
 *   New solver with standard settings
 */
SequentialImpulseSolver *si_solver_create_default(void);

/**
 * Destroy solver
 *
 * Args:
 *   solver: SequentialImpulseSolver to destroy (NULL-safe)
 */
void si_solver_destroy(SequentialImpulseSolver *solver);

/**
 * Update solver configuration
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   config: New configuration
 */
void si_solver_set_config(SequentialImpulseSolver *solver, SISolverConfig config);

/**
 * Get current configuration
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   Current SISolverConfig
 */
SISolverConfig si_solver_get_config(SequentialImpulseSolver *solver);

// ============================================================================
// Body Management
// ============================================================================

/**
 * Add rigid body to solver
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body: RigidBody to add
 *
 * Returns:
 *   true if added successfully
 */
bool si_solver_add_body(SequentialImpulseSolver *solver, RigidBody *body);

/**
 * Remove rigid body from solver
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body: RigidBody to remove
 *
 * Returns:
 *   true if removed successfully
 */
bool si_solver_remove_body(SequentialImpulseSolver *solver, RigidBody *body);

/**
 * Get number of active bodies
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   Number of bodies in solver
 */
u32 si_solver_body_count(SequentialImpulseSolver *solver);

// ============================================================================
// Contact Management
// ============================================================================

/**
 * Add contact manifold to solver
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   manifold: ContactManifold to add
 *
 * Returns:
 *   true if added successfully
 */
bool si_solver_add_contact(SequentialImpulseSolver *solver, ContactManifold *manifold);

/**
 * Remove contact manifold
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   manifold: ContactManifold to remove
 *
 * Returns:
 *   true if removed successfully
 */
bool si_solver_remove_contact(SequentialImpulseSolver *solver, ContactManifold *manifold);

/**
 * Get number of active contacts
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   Number of contacts being solved
 */
u32 si_solver_contact_count(SequentialImpulseSolver *solver);

/**
 * Update contact information
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   manifold: Contact to update
 *   info: New contact info
 */
void si_solver_update_contact(SequentialImpulseSolver *solver, ContactManifold *manifold, ContactInfo *info);

// ============================================================================
// Constraint Creation
// ============================================================================

/**
 * Create velocity constraint
 *
 * Args:
 *   body_a: First body
 *   body_b: Second body
 *   contact_info: Contact information
 *
 * Returns:
 *   VelocityConstraint structure
 */
VelocityConstraint si_solver_create_velocity_constraint(RigidBody *body_a, RigidBody *body_b, ContactInfo *contact_info);

/**
 * Create position constraint
 *
 * Args:
 *   body_a: First body
 *   body_b: Second body
 *   contact_info: Contact information
 *
 * Returns:
 *   PositionConstraint structure
 */
PositionConstraint si_solver_create_position_constraint(RigidBody *body_a, RigidBody *body_b, ContactInfo *contact_info);

// ============================================================================
// Velocity Phase Solving
// ============================================================================

/**
 * Solve velocity constraints
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step
 *
 * Note: Apply accumulated impulses to velocities
 */
void si_solver_solve_velocity(SequentialImpulseSolver *solver, f32 dt);

/**
 * Solve single velocity constraint
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   constraint: VelocityConstraint to solve
 *   body_a: First body
 *   body_b: Second body
 *
 * Returns:
 *   Impulse magnitude applied
 */
f32 si_solver_solve_velocity_constraint(SequentialImpulseSolver *solver, VelocityConstraint *constraint, RigidBody *body_a, RigidBody *body_b);

/**
 * Solve normal impulse (separation)
 *
 * Args:
 *   constraint: Velocity constraint
 *   body_a: First body
 *   body_b: Second body
 *
 * Returns:
 *   Normal impulse magnitude
 */
f32 si_solver_solve_normal_impulse(VelocityConstraint *constraint, RigidBody *body_a, RigidBody *body_b);

/**
 * Solve friction impulses (tangential)
 *
 * Args:
 *   constraint: Velocity constraint
 *   body_a: First body
 *   body_b: Second body
 *   normal_impulse: Applied normal impulse
 *
 * Note: Modifies constraint->tangent_impulse
 */
void si_solver_solve_friction(VelocityConstraint *constraint, RigidBody *body_a, RigidBody *body_b, f32 normal_impulse);

// ============================================================================
// Position Phase Solving
// ============================================================================

/**
 * Solve position constraints
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step
 *
 * Note: Fix penetrations by adjusting positions
 */
void si_solver_solve_position(SequentialImpulseSolver *solver, f32 dt);

/**
 * Solve single position constraint
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   constraint: PositionConstraint to solve
 *   body_a: First body
 *   body_b: Second body
 *   dt: Time step
 *
 * Returns:
 *   Position correction impulse
 */
f32 si_solver_solve_position_constraint(SequentialImpulseSolver *solver, PositionConstraint *constraint, RigidBody *body_a, RigidBody *body_b, f32 dt);

// ============================================================================
// Warm Starting
// ============================================================================

/**
 * Apply warm starting impulses
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body_a: First body
 *   body_b: Second body
 *   constraint: Constraint with accumulated impulses
 */
void si_solver_apply_warm_start(SequentialImpulseSolver *solver, RigidBody *body_a, RigidBody *body_b, VelocityConstraint *constraint);

/**
 * Accumulate impulses for next frame
 *
 * Args:
 *   constraint: Constraint to store impulse
 *   normal_impulse: Normal impulse applied
 *   tangent_impulse: Tangential impulse applied
 */
void si_solver_accumulate_impulse(VelocityConstraint *constraint, f32 normal_impulse, Vec3 tangent_impulse);

/**
 * Clear impulse accumulation (hard reset)
 *
 * Args:
 *   solver: SequentialImpulseSolver
 */
void si_solver_clear_impulses(SequentialImpulseSolver *solver);

/**
 * Scale impulses by factor (for sub-stepping)
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   scale: Scaling factor
 */
void si_solver_scale_impulses(SequentialImpulseSolver *solver, f32 scale);

// ============================================================================
// Restitution and Friction
// ============================================================================

/**
 * Compute restitution-based velocity
 *
 * Args:
 *   relative_velocity: Relative velocity at contact
 *   restitution: Coefficient of restitution (0-1)
 *
 * Returns:
 *   Desired velocity after bounce
 */
f32 si_solver_compute_restitution(f32 relative_velocity, f32 restitution);

/**
 * Clamp friction impulse in cone
 *
 * Args:
 *   constraint: Velocity constraint
 *   tangent_impulse: Proposed tangential impulse
 *   normal_impulse: Applied normal impulse
 *
 * Returns:
 *   Clamped tangent impulse
 */
Vec3 si_solver_clamp_friction_cone(VelocityConstraint *constraint, Vec3 tangent_impulse, f32 normal_impulse);

/**
 * Set friction coefficients
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body_a: First body
 *   body_b: Second body
 *   friction: Friction coefficient
 *   restitution: Restitution coefficient
 */
void si_solver_set_friction_restitution(SequentialImpulseSolver *solver, RigidBody *body_a, RigidBody *body_b, f32 friction, f32 restitution);

// ============================================================================
// Main Solve Loop
// ============================================================================

/**
 * Solve one time step
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step duration
 *
 * Note: Performs both velocity and position solving
 */
void si_solver_step(SequentialImpulseSolver *solver, f32 dt);

/**
 * Solve with sub-stepping for stability
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step duration
 *   num_sub_steps: Number of sub-steps
 *
 * Returns:
 *   Number of sub-steps actually performed
 */
u32 si_solver_step_adaptive(SequentialImpulseSolver *solver, f32 dt, u32 num_sub_steps);

// ============================================================================
// Sleeping and Deactivation
// ============================================================================

/**
 * Update sleep state for all bodies
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step
 */
void si_solver_update_sleep_states(SequentialImpulseSolver *solver, f32 dt);

/**
 * Check if body should sleep
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body: RigidBody to check
 *
 * Returns:
 *   true if body meets sleep criteria
 */
bool si_solver_should_sleep(SequentialImpulseSolver *solver, RigidBody *body);

/**
 * Put body to sleep
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body: RigidBody to sleep
 */
void si_solver_put_to_sleep(SequentialImpulseSolver *solver, RigidBody *body);

/**
 * Wake body up
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   body: RigidBody to wake
 */
void si_solver_wake(SequentialImpulseSolver *solver, RigidBody *body);

/**
 * Get number of sleeping bodies
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   Number of sleeping bodies
 */
u32 si_solver_sleeping_body_count(SequentialImpulseSolver *solver);

// ============================================================================
// SIMD Optimization
// ============================================================================

/**
 * Enable SIMD constraint solving
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   enable: Whether to use SIMD
 */
void si_solver_set_simd(SequentialImpulseSolver *solver, bool enable);

/**
 * Check if SIMD is available and enabled
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   true if SIMD solving active
 */
bool si_solver_has_simd(SequentialImpulseSolver *solver);

/**
 * Solve constraints using SIMD
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   dt: Time step
 *
 * Note: Processes multiple constraints in parallel
 */
void si_solver_solve_simd(SequentialImpulseSolver *solver, f32 dt);

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

/**
 * Get solver statistics
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *   stats_out: Output buffer for statistics
 *   stats_size: Buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 si_solver_get_statistics(SequentialImpulseSolver *solver, char *stats_out, u32 stats_size);

/**
 * Get average constraint iterations
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   Average iterations to converge
 */
f32 si_solver_average_iterations(SequentialImpulseSolver *solver);

/**
 * Benchmark solver performance
 *
 * Args:
 *   num_bodies: Number of bodies to simulate
 *   num_contacts: Number of contact constraints
 *   duration_ms: Test duration
 *
 * Returns:
 *   Average milliseconds per step
 */
f64 si_solver_benchmark(u32 num_bodies, u32 num_contacts, u32 duration_ms);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Validate solver state
 *
 * Args:
 *   solver: SequentialImpulseSolver
 *
 * Returns:
 *   true if all invariants hold
 */
bool si_solver_validate(SequentialImpulseSolver *solver);

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 si_solver_run_tests(void);

/**
 * Stacking stability test
 *
 * Args:
 *   num_blocks: Number of blocks to stack
 *
 * Returns:
 *   true if stack remains stable
 */
bool si_solver_test_stacking(u32 num_blocks);

#ifdef __cplusplus
}
#endif

#endif // SEQUENTIAL_IMPULSE_SOLVER_H
