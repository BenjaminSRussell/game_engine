/**
 * @file xpbd_solver.h
 * @brief Extended Position Based Dynamics (XPBD) solver
 *
 * Provides interface for XPBD simulation with support for various constraint
 * types, adaptive sub-stepping, and collision detection.
 */

#ifndef XPBD_SOLVER_H
#define XPBD_SOLVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct XPBDSolver XPBDSolver;

// ========================================
// Solver Configuration
// ========================================

typedef struct XPBDConfig {
    float gravity[3];              // Gravity vector
    float damping;                // Velocity damping factor
    int max_substeps;             // Maximum substeps per frame
    float min_substep_time;        // Minimum substep time
    float max_substep_time;        // Maximum substep time
    int position_iterations;       // Position constraint iterations
    int velocity_iterations;       // Velocity constraint iterations
    bool enable_collision;         // Enable collision detection
    bool enable_volume_preservation; // Enable volume preservation
    bool enable_adaptive_substepping; // Enable adaptive substepping
    
} XPBDConfig;

// ========================================
// Constants
// ========================================

#define MAX_XPBD_PARTICLES 4096
#define MAX_XPBD_CONSTRAINTS 8192

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create XPBD solver
 * @param max_particles Maximum number of particles
 * @param max_constraints Maximum number of constraints
 * @param config Solver configuration (NULL for default)
 * @return Solver instance or NULL on failure
 */
XPBDSolver* xpbd_create(int max_particles, int max_constraints, const XPBDConfig *config);

/**
 * Destroy XPBD solver
 * @param solver Solver instance
 */
void xpbd_destroy(XPBDSolver *solver);

// ========================================
// Particle Management
// ========================================

/**
 * Add particle to solver
 * @param solver Solver instance
 * @param position Particle position in world space
 * @param mass Particle mass
 * @param pinned Whether particle is fixed in world space
 * @return Particle ID or -1 on failure
 */
int xpbd_add_particle(XPBDSolver *solver, const float *position, float mass, bool pinned);

/**
 * Remove particle from solver
 * @param solver Solver instance
 * @param particle_id Particle ID
 */
void xpbd_remove_particle(XPBDSolver *solver, int particle_id);

/**
 * Get particle position
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param position Output position
 */
void xpbd_get_particle_position(XPBDSolver *solver, int particle_id, float *position);

/**
 * Get particle velocity
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param velocity Output velocity
 */
void xpbd_get_particle_velocity(XPBDSolver *solver, int particle_id, float *velocity);

/**
 * Set particle position
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param position New position
 */
void xpbd_set_particle_position(XPBDSolver *solver, int particle_id, const float *position);

/**
 * Set particle velocity
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param velocity New velocity
 */
void xpbd_set_particle_velocity(XPBDSolver *solver, int particle_id, const float *velocity);

/**
 * Pin/unpin particle
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param pinned Whether to pin the particle
 */
void xpbd_pin_particle(XPBDSolver *solver, int particle_id, bool pinned);

/**
 * Check if particle is pinned
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @return True if particle is pinned
 */
bool xpbd_is_particle_pinned(XPBDSolver *solver, int particle_id);

/**
 * Check if particle is active
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @return True if particle is active
 */
bool xpbd_is_particle_active(XPBDSolver *solver, int particle_id);

// ========================================
// Constraint Management
// ========================================

/**
 * Add distance constraint
 * @param solver Solver instance
 * @param particle_a First particle ID
 * @param particle_b Second particle ID
 * @param stiffness Constraint stiffness
 * @return Constraint ID or -1 on failure
 */
int xpbd_add_distance_constraint(XPBDSolver *solver, int particle_a, int particle_b, float stiffness);

/**
 * Add bending constraint
 * @param solver Solver instance
 * @param particle_a First particle ID
 * @param particle_b Second particle ID
 * @param particle_c Third particle ID
 * @param stiffness Constraint stiffness
 * @return Constraint ID or -1 on failure
 */
int xpbd_add_bending_constraint(XPBDSolver *solver, int particle_a, int particle_b, 
                               int particle_c, float stiffness);

/**
 * Add volume constraint
 * @param solver Solver instance
 * @param particles Array of 4 particle IDs
 * @param stiffness Constraint stiffness
 * @return Constraint ID or -1 on failure
 */
int xpbd_add_volume_constraint(XPBDSolver *solver, const int particles[4], float stiffness);

/**
 * Remove constraint
 * @param solver Solver instance
 * @param constraint_id Constraint ID
 */
void xpbd_remove_constraint(XPBDSolver *solver, int constraint_id);

/**
 * Enable/disable constraint
 * @param solver Solver instance
 * @param constraint_id Constraint ID
 * @param enabled Whether constraint is enabled
 */
void xpbd_enable_constraint(XPBDSolver *solver, int constraint_id, bool enabled);

/**
 * Set constraint stiffness
 * @param solver Solver instance
 * @param constraint_id Constraint ID
 * @param stiffness New stiffness value
 */
void xpbd_set_constraint_stiffness(XPBDSolver *solver, int constraint_id, float stiffness);

// ========================================
// Simulation Interface
// ========================================

/**
 * Update XPBD simulation
 * @param solver Solver instance
 * @param dt Time step
 */
void xpbd_update(XPBDSolver *solver, float dt);

/**
 * Step simulation with fixed time step
 * @param solver Solver instance
 */
void xpbd_step(XPBDSolver *solver);

/**
 * Reset solver state
 * @param solver Solver instance
 */
void xpbd_reset(XPBDSolver *solver);

// ========================================
// Force Application
// ========================================

/**
 * Apply force to particle
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param force Force vector
 */
void xpbd_apply_force(XPBDSolver *solver, int particle_id, const float *force);

/**
 * Apply force to all particles
 * @param solver Solver instance
 * @param force Force vector
 */
void xpbd_apply_force_all(XPBDSolver *solver, const float *force);

/**
 * Apply impulse to particle
 * @param solver Solver instance
 * @param particle_id Particle ID
 * @param impulse Impulse vector
 */
void xpbd_apply_impulse(XPBDSolver *solver, int particle_id, const float *impulse);

/**
 * Apply gravity to all particles
 * @param solver Solver instance
 * @param gravity Gravity vector
 */
void xpbd_apply_gravity(XPBDSolver *solver, const float *gravity);

/**
 * Apply wind force
 * @param solver Solver instance
 * @param wind_direction Wind direction
 * @param strength Wind strength
 */
void xpbd_apply_wind(XPBDSolver *solver, const float *wind_direction, float strength);

// ========================================
// Collision Detection
// ========================================

/**
 * Add collision constraint
 * @param solver Solver instance
 * @param particle_a Particle ID
 * @param normal Collision normal
 * @param penetration Penetration depth
 * @return Constraint ID or -1 on failure
 */
int xpbd_add_collision_constraint(XPBDSolver *solver, int particle_a, 
                                   const float *normal, float penetration);

/**
 * Process particle-particle collisions
 * @param solver Solver instance
 * @param collision_thickness Collision detection thickness
 */
void xpbd_process_particle_collisions(XPBDSolver *solver, float collision_thickness);

/**
 * Process particle-world collisions
 * @param solver Solver instance
 * @param ground_height Ground plane height
 */
void xpbd_process_world_collisions(XPBDSolver *solver, float ground_height);

// ========================================
// Configuration Management
// ========================================

/**
 * Get default configuration
 * @return Default configuration
 */
XPBDConfig xpbd_get_default_config(void);

/**
 * Set solver configuration
 * @param solver Solver instance
 * @param config New configuration
 */
void xpbd_set_config(XPBDSolver *solver, const XPBDConfig *config);

/**
 * Get current configuration
 * @param solver Solver instance
 * @return Current configuration
 */
XPBDConfig xpbd_get_config(XPBDSolver *solver);

/**
 * Set gravity
 * @param solver Solver instance
 * @param gravity Gravity vector
 */
void xpbd_set_gravity(XPBDSolver *solver, const float *gravity);

/**
 * Set damping factor
 * @param solver Solver instance
 * @param damping Damping factor (0-1)
 */
void xpbd_set_damping(XPBDSolver *solver, float damping);

/**
 * Set iteration counts
 * @param solver Solver instance
 * @param position_iterations Position constraint iterations
 * @param velocity_iterations Velocity constraint iterations
 */
void xpbd_set_iterations(XPBDSolver *solver, int position_iterations, int velocity_iterations);

/**
 * Enable/disable features
 * @param solver Solver instance
 * @param collision Enable collision detection
 * @param volume_preservation Enable volume preservation
 * @param adaptive_substepping Enable adaptive substepping
 */
void xpbd_enable_features(XPBDSolver *solver, bool collision, bool volume_preservation, bool adaptive_substepping);

// ========================================
// Statistics and Debugging
// ========================================

/**
 * Get solver statistics
 * @param solver Solver instance
 * @param particle_count Number of particles
 * @param constraint_count Number of constraints
 * @param substep_count Number of substeps in last update
 * @param solve_time Time spent solving (milliseconds)
 */
void xpbd_get_stats(const XPBDSolver *solver, int *particle_count, int *constraint_count, 
                    int *substep_count, float *solve_time);

/**
 * Get particle count
 * @param solver Solver instance
 * @return Number of active particles
 */
int xpbd_get_particle_count(XPBDSolver *solver);

/**
 * Get constraint count
 * @param solver Solver instance
 * @return Number of active constraints
 */
int xpbd_get_constraint_count(XPBDSolver *solver);

/**
 * Get simulation time
 * @param solver Solver instance
 * @return Current simulation time
 */
float xpbd_get_simulation_time(XPBDSolver *solver);

/**
 * Validate solver state
 * @param solver Solver instance
 * @return True if state is valid
 */
bool xpbd_validate(const XPBDSolver *solver);

// ========================================
// Advanced Features
// ========================================

/**
 * Create cloth mesh from particles
 * @param solver Solver instance
 * @param width Mesh width
 * @param height Mesh height
 * @param spacing Particle spacing
 * @param position Mesh origin
 * @param mass Particle mass
 */
void xpbd_create_cloth_mesh(XPBDSolver *solver, int width, int height, float spacing, 
                             const float *position, float mass);

/**
 * Create rope from particles
 * @param solver Solver instance
 * @param particle_count Number of particles
 * @param spacing Particle spacing
 * @param start Rope start position
 * @param end Rope end position
 * @param mass Particle mass
 */
void xpbd_create_rope(XPBDSolver *solver, int particle_count, float spacing,
                      const float *start, const float *end, float mass);

/**
 * Create soft body from tetrahedral mesh
 * @param solver Solver instance
 * @param vertices Mesh vertices
 * @param vertex_count Number of vertices
 * @param tetrahedra Tetrahedron indices
 * @param tetrahedron_count Number of tetrahedra
 * @param mass Particle mass
 */
void xpbd_create_soft_body(XPBDSolver *solver, const float *vertices, int vertex_count,
                           const int *tetrahedra, int tetrahedron_count, float mass);

/**
 * Export particle positions for rendering
 * @param solver Solver instance
 * @param positions Output position array
 * @param max_positions Maximum positions to copy
 * @return Number of positions copied
 */
int xpbd_export_positions(const XPBDSolver *solver, float *positions, int max_positions);

/**
 * Export particle velocities for rendering
 * @param solver Solver instance
 * @param velocities Output velocity array
 * @param max_velocities Maximum velocities to copy
 * @return Number of velocities copied
 */
int xpbd_export_velocities(const XPBDSolver *solver, float *velocities, int max_velocities);

/**
 * Save solver state to file
 * @param solver Solver instance
 * @param filename Output filename
 * @return True if successful
 */
bool xpbd_save_state(XPBDSolver *solver, const char *filename);

/**
 * Load solver state from file
 * @param solver Solver instance
 * @param filename Input filename
 * @return True if successful
 */
bool xpbd_load_state(XPBDSolver *solver, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* XPBD_SOLVER_H */
