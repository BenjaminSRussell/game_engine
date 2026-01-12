/**
 * @file sph_simulation.h
 * @brief SPH fluid simulation using Smoothed Particle Hydrodynamics
 *
 * Implements fluid simulation using SPH with support for incompressible fluids,
 * surface tension, viscosity, and interaction with rigid bodies.
 */

#ifndef RENDER_SPH_SIMULATION_H
#define RENDER_SPH_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct SPHSimulation SPHSimulation;
typedef struct SPHParticle SPHParticle;
typedef struct SPHBoundary SPHBoundary;

// ========================================
// SPH Particle
// ========================================

typedef struct SPHParticle {
    // Position and motion
    float position[3];          // Current position
    float old_position[3];       // Previous position
    float velocity[3];          // Current velocity
    float acceleration[3];       // Acceleration
    
    // Physical properties
    float mass;                 // Particle mass
    float density;              // Current density
    float pressure;             // Pressure
    float rest_density;          // Rest density
    
    // SPH properties
    float kernel_radius;         // Smoothing kernel radius
    float viscosity;            // Dynamic viscosity
    
    // Forces
    float pressure_force[3];     // Pressure gradient force
    float viscosity_force[3];    // Viscosity force
    float surface_tension_force[3]; // Surface tension force
    float external_force[3];     // External forces (gravity, etc.)
    
    // Neighbor information
    int *neighbors;             // Neighbor particle indices
    int neighbor_count;          // Number of neighbors
    int neighbor_capacity;       // Neighbor array capacity
    
    // Surface properties
    float color_field;           // Color field for surface detection
    float color_field_gradient[3]; // Color field gradient
    float surface_curvature;     // Surface curvature
    
    // Simulation state
    bool is_boundary;           // Boundary particle flag
    bool active;                // Particle is active in simulation
    
    // User data
    void *user_data;
    
} SPHParticle;

// ========================================
// SPH Boundary Types
// ========================================

typedef enum BoundaryType {
    BOUNDARY_PLANE,             // Infinite plane
    BOUNDARY_SPHERE,           // Sphere
    BOUNDARY_BOX,              // Axis-aligned box
    BOUNDARY_MESH              // Triangle mesh
} BoundaryType;

// ========================================
// Plane Boundary
// ========================================

typedef struct PlaneBoundary {
    float normal[3];            // Plane normal (must be normalized)
    float distance;             // Distance from origin
    float friction;             // Friction coefficient
} PlaneBoundary;

// ========================================
// Sphere Boundary
// ========================================

typedef struct SphereBoundary {
    float center[3];            // Sphere center
    float radius;               // Sphere radius
    float friction;             // Friction coefficient
} SphereBoundary;

// ========================================
// Box Boundary
// ========================================

typedef struct BoxBoundary {
    float min[3];               // Minimum corner
    float max[3];               // Maximum corner
    float friction;             // Friction coefficient
} BoxBoundary;

// ========================================
// Mesh Boundary
// ========================================

typedef struct MeshBoundary {
    float *vertices;            // Vertex positions
    int *triangles;             // Triangle indices
    int vertex_count;
    int triangle_count;
    float friction;             // Friction coefficient
    
    // Spatial acceleration
    void *bvh;                  // Bounding volume hierarchy
    bool bvh_dirty;
    
} MeshBoundary;

// ========================================
// Unified Boundary
// ========================================

struct SPHBoundary {
    BoundaryType type;
    union {
        PlaneBoundary plane;
        SphereBoundary sphere;
        BoxBoundary box;
        MeshBoundary mesh;
    } data;
    
    bool enabled;
};

// ========================================
// SPH Kernel Types
// ========================================

typedef enum KernelType {
    KERNEL_POLY6,              // Poly6 kernel (for density)
    KERNEL_SPIKY,              // Spiky kernel (for pressure)
    KERNEL_VISCOSITY,          // Viscosity kernel
    KERNEL_COHESION             // Cohesion kernel (surface tension)
} KernelType;

// ========================================
// SPH Configuration
// ========================================

typedef struct SPHConfig {
    // Fluid properties
    float rest_density;         // Rest density (kg/m)
    float gas_constant;         // Gas stiffness constant
    float viscosity;            // Dynamic viscosity
    float surface_tension;      // Surface tension coefficient
    float kernel_radius;        // Smoothing kernel radius
    
    // Simulation parameters
    float time_step;            // Fixed time step
    int solver_iterations;      // Pressure solver iterations
    float gravity[3];           // Gravity vector
    float damping;              // Velocity damping
    
    // Boundary parameters
    float boundary_influence;   // Boundary particle influence radius
    float boundary_stiffness;   // Boundary repulsion stiffness
    float boundary_damping;     // Boundary damping
    
    // Performance parameters
    bool use_spatial_hash;      // Use spatial hashing for neighbors
    int hash_grid_size;         // Spatial hash grid size
    float cell_size;            // Spatial hash cell size
    bool gpu_acceleration;      // Use GPU for computation
    
    // Solver parameters
    bool enable_surface_tension; // Enable surface tension
    bool enable_viscosity;      // Enable viscosity
    bool enable_adhesion;       // Enable adhesion to boundaries
    float cfl_number;           // CFL condition number for stability
    
} SPHConfig;

// ========================================
// SPH Simulation Structure
// ========================================

struct SPHSimulation {
    // Particles
    SPHParticle *particles;
    int particle_count;
    int particle_capacity;
    
    // Boundaries
    SPHBoundary *boundaries;
    int boundary_count;
    int boundary_capacity;
    
    // Configuration
    SPHConfig config;
    
    // Spatial hashing
    int *hash_grid;             // Particle hash grid
    int *hash_buckets;          // Hash bucket indices
    int hash_grid_size;
    float cell_size;
    
    // Kernel precomputed values
    float *poly6_kernel;         // Precomputed Poly6 kernel values
    float *spiky_kernel;        // Precomputed Spiky kernel values
    float *viscosity_kernel;    // Precomputed viscosity kernel values
    int kernel_samples;         // Number of precomputed samples
    
    // Simulation state
    float total_mass;           // Total fluid mass
    float total_volume;         // Total fluid volume
    float simulation_time;       // Current simulation time
    bool simulation_active;
    
    // Performance metrics
    float avg_density;          // Average density
    float max_velocity;          // Maximum velocity
    float reynolds_number;       // Reynolds number
    int neighbor_search_time;    // Time spent in neighbor search (microseconds)
    int force_calculation_time; // Time spent in force calculation (microseconds)
    
    // Bounding volume
    float bounds_min[3];        // Simulation bounds
    float bounds_max[3];
    bool bounds_dirty;
    
    // GPU resources (if enabled)
    void *gpu_particle_buffer;
    void *gpu_hash_buffer;
    bool gpu_data_dirty;
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create SPH simulation with specified capacity
 */
SPHSimulation* sph_simulation_create(int max_particles, int max_boundaries, 
                                   const SPHConfig *config);

/**
 * Destroy SPH simulation and free resources
 */
void sph_simulation_destroy(SPHSimulation *sim);

// ========================================
// Particle Management
// ========================================

/**
 * Add fluid particle to simulation
 */
int sph_simulation_add_particle(SPHSimulation *sim, const float *position, float mass);

/**
 * Add boundary particle to simulation
 */
int sph_simulation_add_boundary_particle(SPHSimulation *sim, const float *position, float mass);

/**
 * Remove particle from simulation
 */
void sph_simulation_remove_particle(SPHSimulation *sim, int particle_id);

/**
 * Get particle data
 */
SPHParticle* sph_simulation_get_particle(SPHSimulation *sim, int particle_id);

/**
 * Update particle position and velocity
 */
void sph_simulation_update_particle(SPHSimulation *sim, int particle_id, 
                                  const float *position, const float *velocity);

// ========================================
// Boundary Management
// ========================================

/**
 * Add plane boundary
 */
int sph_simulation_add_plane_boundary(SPHSimulation *sim, const float *normal, 
                                     float distance, float friction);

/**
 * Add sphere boundary
 */
int sph_simulation_add_sphere_boundary(SPHSimulation *sim, const float *center, 
                                      float radius, float friction);

/**
 * Add box boundary
 */
int sph_simulation_add_box_boundary(SPHSimulation *sim, const float *min, 
                                    const float *max, float friction);

/**
 * Add mesh boundary
 */
int sph_simulation_add_mesh_boundary(SPHSimulation *sim, 
                                    const float *vertices, int vertex_count,
                                    const int *triangles, int triangle_count, float friction);

/**
 * Remove boundary
 */
void sph_simulation_remove_boundary(SPHSimulation *sim, int boundary_id);

/**
 * Update boundary transform (for dynamic boundaries)
 */
void sph_simulation_update_boundary(SPHSimulation *sim, int boundary_id);

// ========================================
// Fluid Creation Utilities
// ========================================

/**
 * Create rectangular fluid volume
 */
void sph_simulation_create_fluid_box(SPHSimulation *sim, const float *min, const float *max,
                                     float spacing, float particle_mass);

/**
 * Create spherical fluid volume
 */
void sph_simulation_create_fluid_sphere(SPHSimulation *sim, const float *center, 
                                       float radius, float spacing, float particle_mass);

/**
 * Create fluid from point cloud
 */
void sph_simulation_create_fluid_from_points(SPHSimulation *sim, 
                                            const float *points, int point_count,
                                            float spacing, float particle_mass);

// ========================================
// Simulation Interface
// ========================================

/**
 * Update SPH simulation
 */
void sph_simulation_update(SPHSimulation *sim, float dt);

/**
 * Step simulation with fixed time step
 */
void sph_simulation_step(SPHSimulation *sim);

/**
 * Apply external force to all particles
 */
void sph_simulation_apply_force(SPHSimulation *sim, const float *force);

/**
 * Apply force to specific particle
 */
void sph_simulation_apply_particle_force(SPHSimulation *sim, int particle_id, const float *force);

/**
 * Apply impulse to particle
 */
void sph_simulation_apply_impulse(SPHSimulation *sim, int particle_id, const float *impulse);

/**
 * Set gravity
 */
void sph_simulation_set_gravity(SPHSimulation *sim, const float *gravity);

// ========================================
// SPH Computation
// ========================================

/**
 * Compute particle densities
 */
void sph_simulation_compute_densities(SPHSimulation *sim);

/**
 * Compute particle pressures
 */
void sph_simulation_compute_pressures(SPHSimulation *sim);

/**
 * Compute particle forces
 */
void sph_simulation_compute_forces(SPHSimulation *sim);

/**
 * Integrate particle motion
 */
void sph_simulation_integrate(SPHSimulation *sim, float dt);

/**
 * Handle boundary collisions
 */
void sph_simulation_handle_boundaries(SPHSimulation *sim);

// ========================================
// Neighbor Search
// ========================================

/**
 * Find particle neighbors using spatial hashing
 */
void sph_simulation_find_neighbors(SPHSimulation *sim);

/**
 * Find neighbors for specific particle
 */
int sph_simulation_find_particle_neighbors(SPHSimulation *sim, int particle_id, 
                                          int *neighbors, int max_neighbors);

/**
 * Update spatial hash grid
 */
void sph_simulation_update_hash_grid(SPHSimulation *sim);

// ========================================
// Kernel Functions
// ========================================

/**
 * Compute Poly6 kernel value
 */
float sph_poly6_kernel(float distance_sq, float kernel_radius_sq);

/**
 * Compute Spiky kernel gradient
 */
void sph_spiky_kernel_gradient(float *result, const float *r, float distance, float kernel_radius);

/**
 * Compute viscosity kernel Laplacian
 */
float sph_viscosity_kernel_laplacian(float distance, float kernel_radius);

/**
 * Precompute kernel values for performance
 */
void sph_simulation_precompute_kernels(SPHSimulation *sim);

// ========================================
// Surface Detection
// ========================================

/**
 * Compute color field for surface detection
 */
void sph_simulation_compute_color_field(SPHSimulation *sim);

/**
 * Detect surface particles
 */
int sph_simulation_detect_surface_particles(SPHSimulation *sim, int *surface_particles, int max_count);

/**
 * Generate surface mesh from particles
 */
void sph_simulation_generate_surface_mesh(SPHSimulation *sim, float iso_threshold,
                                         float **vertices, int *vertex_count,
                                         int **triangles, int *triangle_count);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default SPH configuration
 */
SPHConfig sph_simulation_get_default_config(void);

/**
 * Update simulation configuration
 */
void sph_simulation_set_config(SPHSimulation *sim, const SPHConfig *config);

/**
 * Get simulation statistics
 */
void sph_simulation_get_stats(const SPHSimulation *sim, int *particle_count, 
                             int *boundary_count, float *total_mass, float *avg_density);

/**
 * Calculate simulation bounds
 */
void sph_simulation_update_bounds(SPHSimulation *sim);

/**
 * Get simulation bounds
 */
void sph_simulation_get_bounds(const SPHSimulation *sim, float *min_bounds, float *max_bounds);

/**
 * Validate simulation state
 */
bool sph_simulation_validate(const SPHSimulation *sim);

/**
 * Reset simulation
 */
void sph_simulation_reset(SPHSimulation *sim);

/**
 * Optimize simulation performance
 */
void sph_simulation_optimize(SPHSimulation *sim);

// ========================================
// Rendering Support
// ========================================

/**
 * Get particle positions for rendering
 */
const float* sph_simulation_get_particle_positions(const SPHSimulation *sim);

/**
 * Get particle velocities for rendering
 */
const float* sph_simulation_get_particle_velocities(const SPHSimulation *sim);

/**
 * Get particle densities for rendering
 */
const float* sph_simulation_get_particle_densities(const SPHSimulation *sim);

/**
 * Get particle colors based on properties
 */
void sph_simulation_get_particle_colors(const SPHSimulation *sim, float *colors, 
                                       bool color_by_velocity, bool color_by_density);

/**
 * Get particle data for GPU upload
 */
void sph_simulation_get_gpu_data(const SPHSimulation *sim, void **particle_data, int *particle_count);

// ========================================
// Advanced Features
// ========================================

/**
 * Enable/disable GPU acceleration
 */
void sph_simulation_set_gpu_acceleration(SPHSimulation *sim, bool enabled);

/**
 * Save simulation state to file
 */
bool sph_simulation_save_state(const SPHSimulation *sim, const char *filename);

/**
 * Load simulation state from file
 */
bool sph_simulation_load_state(SPHSimulation *sim, const char *filename);

/**
 * Export particle data to CSV
 */
bool sph_simulation_export_csv(const SPHSimulation *sim, const char *filename);

/**
 * Import particle data from CSV
 */
bool sph_simulation_import_csv(SPHSimulation *sim, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_SPH_SIMULATION_H */
