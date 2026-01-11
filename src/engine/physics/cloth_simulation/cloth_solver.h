/**
 * @file cloth_solver.h
 * @brief Cloth constraint solver using Position Based Dynamics (PBD)
 *
 * Implements cloth simulation using distance constraints, bending constraints,
 * and collision response. Supports self-collision and external forces.
 */

#ifndef RENDER_CLOTH_SOLVER_H
#define RENDER_CLOTH_SOLVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct ClothSolver ClothSolver;
typedef struct ClothParticle ClothParticle;
typedef struct ClothConstraint ClothConstraint;

// ========================================
// Cloth Particle
// ========================================

typedef struct ClothParticle {
    // Position and velocity
    float position[3];
    float old_position[3];
    float velocity[3];
    
    // Forces
    float force[3];
    
    // Physical properties
    float mass;
    float inv_mass;
    
    // Constraints
    bool pinned;           // Fixed in world space
    bool collision_enabled;
    
    // Collision data
    float normal[3];       // Collision normal
    float penetration;     // Collision penetration depth
    
} ClothParticle;

// ========================================
// Constraint Types
// ========================================

typedef enum ConstraintType {
    CONSTRAINT_DISTANCE,    // Distance constraint between two particles
    CONSTRAINT_BENDING,     // Bending resistance
    CONSTRAINT_COLLISION    // Collision constraint
} ConstraintType;

// ========================================
// Distance Constraint
// ========================================

typedef struct DistanceConstraint {
    int particle_a;         // Index of first particle
    int particle_b;         // Index of second particle
    float rest_length;      // Rest length of constraint
    float stiffness;        // Constraint stiffness (0-1)
    bool enabled;
} DistanceConstraint;

// ========================================
// Bending Constraint
// ========================================

typedef struct BendingConstraint {
    int particle_a;         // First particle
    int particle_b;         // Second particle  
    int particle_c;         // Third particle
    int particle_d;         // Fourth particle
    float rest_angle;       // Rest bending angle
    float stiffness;        // Bending stiffness
    bool enabled;
} BendingConstraint;

// ========================================
// Collision Constraint
// ========================================

typedef struct CollisionConstraint {
    int particle;           // Particle index
    float normal[3];        // Collision normal
    float penetration;      // Penetration depth
    float friction;         // Friction coefficient
    bool enabled;
} CollisionConstraint;

// ========================================
// Unified Constraint
// ========================================

struct ClothConstraint {
    ConstraintType type;
    union {
        DistanceConstraint distance;
        BendingConstraint bending;
        CollisionConstraint collision;
    } data;
    bool enabled;
};

// ========================================
// Cloth Configuration
// ========================================

typedef struct ClothConfig {
    // Simulation parameters
    float gravity[3];       // Gravity vector
    float damping;          // Velocity damping (0-1)
    float air_resistance;   // Air resistance
    
    // Constraint parameters
    int solver_iterations;  // Solver iterations per frame
    float distance_stiffness; // Distance constraint stiffness
    float bending_stiffness;  // Bending constraint stiffness
    
    // Collision parameters
    bool self_collision;    // Enable self-collision
    bool world_collision;   // Enable world collision
    float collision_thickness; // Collision thickness
    
    // Performance parameters
    bool use_spatial_hash;  // Use spatial hashing for collision
    int grid_size;          // Spatial hash grid size
    
} ClothConfig;

// ========================================
// Cloth Mesh Data
// ========================================

typedef struct ClothMesh {
    int width;              // Mesh width (particles)
    int height;             // Mesh height (particles)
    float spacing;          // Distance between particles
    
    // Particle indices (for mesh topology)
    int *triangles;         // Triangle indices (3 per triangle)
    int triangle_count;
    
} ClothMesh;

// ========================================
// Solver Creation/Destruction
// ========================================

/**
 * Create cloth solver with specified particle count
 */
ClothSolver* cloth_solver_create(int max_particles, int max_constraints, const ClothConfig *config);

/**
 * Destroy cloth solver and free resources
 */
void cloth_solver_destroy(ClothSolver *solver);

// ========================================
// Particle Management
// ========================================

/**
 * Add particle to solver
 */
int cloth_solver_add_particle(ClothSolver *solver, const float *position, float mass, bool pinned);

/**
 * Remove particle from solver
 */
void cloth_solver_remove_particle(ClothSolver *solver, int particle_id);

/**
 * Get particle data
 */
ClothParticle* cloth_solver_get_particle(ClothSolver *solver, int particle_id);

/**
 * Set particle position
 */
void cloth_solver_set_particle_position(ClothSolver *solver, int particle_id, const float *position);

/**
 * Pin particle in world space
 */
void cloth_solver_pin_particle(ClothSolver *solver, int particle_id, bool pinned);

// ========================================
// Constraint Management
// ========================================

/**
 * Add distance constraint between two particles
 */
int cloth_solver_add_distance_constraint(ClothSolver *solver, int particle_a, int particle_b, 
                                        float stiffness);

/**
 * Add bending constraint between four particles
 */
int cloth_solver_add_bending_constraint(ClothSolver *solver, int particle_a, int particle_b,
                                       int particle_c, int particle_d, float stiffness);

/**
 * Remove constraint
 */
void cloth_solver_remove_constraint(ClothSolver *solver, int constraint_id);

// ========================================
// Cloth Mesh Creation
// ========================================

/**
 * Create rectangular cloth mesh
 */
void cloth_solver_create_rect_mesh(ClothSolver *solver, int width, int height, 
                                   float spacing, const float *position, float mass);

/**
 * Create cloth mesh from custom topology
 */
void cloth_solver_create_custom_mesh(ClothSolver *solver, const ClothMesh *mesh, 
                                     const float *positions, float mass);

// ========================================
// Simulation Interface
// ========================================

/**
 * Update cloth simulation
 */
void cloth_solver_update(ClothSolver *solver, float dt);

/**
 * Apply external force to all particles
 */
void cloth_solver_apply_force(ClothSolver *solver, const float *force);

/**
 * Apply force to specific particle
 */
void cloth_solver_apply_particle_force(ClothSolver *solver, int particle_id, const float *force);

/**
 * Apply wind force (directional)
 */
void cloth_solver_apply_wind(ClothSolver *solver, const float *wind_direction, float strength);

// ========================================
// Collision Interface
// ========================================

/**
 * Add world collision constraint
 */
void cloth_solver_add_world_collision(ClothSolver *solver, int particle_id, 
                                     const float *normal, float penetration, float friction);

/**
 * Process self-collision
 */
void cloth_solver_process_self_collision(ClothSolver *solver);

/**
 * Process world collision (with spheres, planes, etc.)
 */
void cloth_solver_process_world_collision(ClothSolver *solver);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default cloth configuration
 */
ClothConfig cloth_solver_get_default_config(void);

/**
 * Update solver configuration
 */
void cloth_solver_set_config(ClothSolver *solver, const ClothConfig *config);

/**
 * Get solver statistics
 */
void cloth_solver_get_stats(const ClothSolver *solver, int *particle_count, 
                           int *constraint_count, int *collision_count);

/**
 * Reset solver state
 */
void cloth_solver_reset(ClothSolver *solver);

/**
 * Validate solver state
 */
bool cloth_solver_validate(const ClothSolver *solver);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_CLOTH_SOLVER_H */
