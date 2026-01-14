/**
 * @file physics_solver_system.h
 * @brief Unified Physics Solver System
 *
 * Provides a unified interface for XPBD and Sequential Impulse solvers
 * with support for soft body, rigid body, and voxel physics.
 */

#ifndef PHYSICS_SOLVER_SYSTEM_H
#define PHYSICS_SOLVER_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct physics_solver_system physics_solver_system;
typedef struct RigidBody RigidBody;
typedef struct Contact Contact;

// ========================================
// Solver Types
// ========================================

typedef enum solver_type {
    SOLVER_TYPE_XPBD = 0,
    SOLVER_TYPE_SEQUENTIAL_IMPULSE = 1,
    SOLVER_TYPE_HYBRID = 2
} solver_type;

// ========================================
// Configuration Structure
// ========================================

typedef struct physics_solver_config {
    solver_type primary_solver;
    bool enable_hybrid_solving;
    float time_step;
    float gravity[3];
    int max_rigid_bodies;
    int max_contacts;
} physics_solver_config;

// ========================================
// Statistics Structure
// ========================================

typedef struct physics_solver_stats {
    // General stats
    int rigid_body_count;
    int contact_count;
    float solve_time_ms;
    int bodies_updated;
    int contacts_solved;
    int iterations_performed;
    
    // XPBD-specific stats
    int xpbd_particle_count;
    int xpbd_constraint_count;
    int xpbd_substep_count;
    
    // Sequential impulse-specific stats
    int impulse_iteration_count;
    float impulse_total_impulse;
} physics_solver_stats;

// ========================================
// Creation and Destruction
// ========================================

/**
 * Create unified physics solver system
 * @param config Configuration (NULL for defaults)
 * @return Solver system instance or NULL on failure
 */
physics_solver_system* physics_solver_create(const physics_solver_config *config);

/**
 * Destroy physics solver system
 * @param system Solver system instance
 */
void physics_solver_destroy(physics_solver_system *system);

// ========================================
// Rigid Body Management
// ========================================

/**
 * Add rigid body to simulation
 * @param system Solver system instance
 * @param body Rigid body data
 * @return Body ID or -1 on failure
 */
int physics_solver_add_rigid_body(physics_solver_system *system, const RigidBody *body);

/**
 * Remove rigid body from simulation
 * @param system Solver system instance
 * @param body_id Body ID to remove
 */
void physics_solver_remove_rigid_body(physics_solver_system *system, int body_id);

/**
 * Get rigid body by ID
 * @param system Solver system instance
 * @param body_id Body ID
 * @return Pointer to rigid body or NULL
 */
RigidBody* physics_solver_get_rigid_body(physics_solver_system *system, int body_id);

// ========================================
// Contact Management
// ========================================

/**
 * Add contact to simulation
 * @param system Solver system instance
 * @param contact Contact data
 * @return Contact ID or -1 on failure
 */
int physics_solver_add_contact(physics_solver_system *system, const Contact *contact);

/**
 * Clear all contacts
 * @param system Solver system instance
 */
void physics_solver_clear_contacts(physics_solver_system *system);

// ========================================
// Simulation Interface
// ========================================

/**
 * Step simulation with specific time step
 * @param system Solver system instance
 * @param dt Time step
 */
void physics_solver_step(physics_solver_system *system, float dt);

/**
 * Update simulation with automatic sub-stepping
 * @param system Solver system instance
 * @param dt Frame time
 */
void physics_solver_update(physics_solver_system *system, float dt);

// ========================================
// Configuration Management
// ========================================

/**
 * Set world gravity
 * @param system Solver system instance
 * @param gravity Gravity vector (x, y, z)
 */
void physics_solver_set_gravity(physics_solver_system *system, const float *gravity);

/**
 * Set fixed time step
 * @param system Solver system instance
 * @param time_step Time step in seconds
 */
void physics_solver_set_time_step(physics_solver_system *system, float time_step);

/**
 * Pause/resume simulation
 * @param system Solver system instance
 * @param paused Pause state
 */
void physics_solver_pause(physics_solver_system *system, bool paused);

/**
 * Reset simulation state
 * @param system Solver system instance
 */
void physics_solver_reset(physics_solver_system *system);

// ========================================
// Statistics and Debugging
// ========================================

/**
 * Get solver statistics
 * @param system Solver system instance
 * @param stats Output statistics structure
 */
void physics_solver_get_stats(const physics_solver_system *system, physics_solver_stats *stats);

/**
 * Validate solver system state
 * @param system Solver system instance
 * @return True if valid
 */
bool physics_solver_validate(const physics_solver_system *system);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default configuration
 * @return Default configuration
 */
physics_solver_config physics_solver_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_SOLVER_SYSTEM_H */
