/**
 * Sequential Impulse Constraint Solver
 * 
 * Implements Box2D-style sequential impulse solver for contact constraints.
 * Solves position and velocity constraints iteratively.
 */

#ifndef CONSTRAINT_SOLVER_H
#define CONSTRAINT_SOLVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct RigidBody RigidBody;
typedef struct ContactManifold ContactManifold;

// ========================================
// Constraint Types
// ========================================

typedef enum ConstraintType {
    CONSTRAINT_CONTACT,
    CONSTRAINT_FRICTION,
    CONSTRAINT_RESTITUTION,
    CONSTRAINT_JOINT
} ConstraintType;

// ========================================
// Contact Constraint
// ========================================

typedef struct ContactConstraint {
    // Body references
    RigidBody *body_a;
    RigidBody *body_b;
    
    // Contact geometry
    float normal[3];           // Contact normal (from B to A)
    float point[3];            // Contact point in world space
    float penetration;         // Penetration depth
    
    // Material properties
    float restitution;         // Bounciness (0-1)
    float static_friction;     // Static friction coefficient
    float kinetic_friction;    // Kinetic friction coefficient
    
    // Constraint forces
    float normal_impulse;      // Accumulated normal impulse
    float tangent_impulse[2];  // Accumulated tangent impulses
    
    // Effective mass
    float normal_mass;         // Effective mass for normal constraint
    float tangent_mass[2];     // Effective mass for tangent constraints
    
    // Relative velocities
    float relative_velocity[3];
    float normal_velocity;     // Velocity along normal
    float tangent_velocity[2]; // Velocity along tangents
    
    // Constraint state
    bool enabled;
    bool persistent;           // Warm starting enabled
    
} ContactConstraint;

// ========================================
// Solver Configuration
// ========================================

typedef struct SolverConfig {
    int velocity_iterations;    // Velocity solver iterations
    int position_iterations;    // Position solver iterations
    bool warm_starting;        // Enable warm starting
    bool position_correction;   // Enable position based correction
    float baumgarte_factor;    // Position correction factor
    float slop;                // Position correction slop
    float max_linear_correction; // Max position correction
    float max_angular_correction; // Max angular correction
} SolverConfig;

// ========================================
// Solver Context
// ========================================

typedef struct ConstraintSolver {
    ContactConstraint *constraints;
    int constraint_count;
    int constraint_capacity;
    
    SolverConfig config;
    
    // Temporary storage
    float *velocity_buffer;
    float *position_buffer;
    
} ConstraintSolver;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create constraint solver with given capacity
 */
ConstraintSolver* constraint_solver_create(int max_constraints, const SolverConfig *config);

/**
 * Destroy constraint solver and free resources
 */
void constraint_solver_destroy(ConstraintSolver *solver);

// ========================================
// Constraint Management
// ========================================

/**
 * Add contact constraint from collision manifold
 */
int constraint_solver_add_contact(ConstraintSolver *solver, 
                                  const ContactManifold *manifold,
                                  RigidBody *body_a, RigidBody *body_b);

/**
 * Clear all constraints
 */
void constraint_solver_clear(ConstraintSolver *solver);

/**
 * Remove inactive constraints
 */
void constraint_solver_cull(ConstraintSolver *solver);

// ========================================
// Solver Interface
// ========================================

/**
 * Solve velocity constraints
 */
void constraint_solver_solve_velocities(ConstraintSolver *solver, float dt);

/**
 * Solve position constraints
 */
void constraint_solver_solve_positions(ConstraintSolver *solver);

/**
 * Warm start constraints from previous frame
 */
void constraint_solver_warm_start(ConstraintSolver *solver);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default solver configuration
 */
SolverConfig constraint_solver_get_default_config(void);

/**
 * Update solver configuration
 */
void constraint_solver_set_config(ConstraintSolver *solver, const SolverConfig *config);

/**
 * Get solver statistics
 */
void constraint_solver_get_stats(const ConstraintSolver *solver, 
                                int *active_constraints, 
                                int *total_iterations);

#ifdef __cplusplus
}
#endif

#endif // CONSTRAINT_SOLVER_H
