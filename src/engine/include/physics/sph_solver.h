#ifndef SPH_SOLVER_H
#define SPH_SOLVER_H

#include <common.h>
#include <math/vec3.h>
#include <stdbool.h>

// Maximum neighbors per particle
#define SPH_MAX_NEIGHBORS 64

// SPH Particle structure
typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 force;
  float density;
  float pressure;
  float mass;

  // Neighbor cache
  uint32_t neighbors[SPH_MAX_NEIGHBORS];
  uint32_t neighbor_count;

  // Additional properties
  Vec3 color; // For visualization/mixing
  float temperature;
  uint32_t id;
} SPHParticle;

// SPH Solver configuration
typedef struct {
  // Physical parameters (SI units)
  float rest_density;     //  (kg/m) - 1000 for water
  float particle_mass;    // m (kg)
  float smoothing_length; // h (m) - kernel radius
  float viscosity;        //  (Pas) - 0.001 for water
  float surface_tension;  //  (N/m) - 0.0728 for water
  float gas_constant;     // k (stiffness parameter)

  // Simulation parameters
  float time_step; // t (s)
  float damping;   // Velocity damping
  Vec3 gravity;    // g (m/s)

  // Boundary
  Vec3 bounds_min;
  Vec3 bounds_max;
  float boundary_stiffness;

  // Performance
  uint32_t max_iterations; // For PCISPH
  float density_error_threshold;
} SPHConfig;

// Spatial hash for neighbor search
typedef struct {
  uint32_t *cell_start;       // Start index in particle_indices for each cell
  uint32_t *cell_end;         // End index
  uint32_t *particle_indices; // Sorted particle indices
  uint32_t *particle_cells;   // Cell index for each particle

  uint32_t grid_size_x;
  uint32_t grid_size_y;
  uint32_t grid_size_z;
  float cell_size;
  Vec3 grid_origin;
} SpatialHash;

// SPH Solver
typedef struct {
  SPHParticle *particles;
  uint32_t particle_count;
  uint32_t capacity;

  SPHConfig config;
  SpatialHash spatial_hash;

  // Statistics
  float avg_density;
  float max_velocity;
  uint32_t step_count;

  // GPU buffer (optional)
  void *gpu_buffer;
  bool use_gpu;
} SPHSolver;

// ============================================================================
// Public API
// ============================================================================

// Initialization
SPHSolver *sph_solver_create(uint32_t capacity, const SPHConfig *config);
void sph_solver_free(SPHSolver *solver);
SPHConfig sph_get_default_config(void);

// Particle management
uint32_t sph_add_particle(SPHSolver *solver, Vec3 position, Vec3 velocity);
void sph_remove_particle(SPHSolver *solver, uint32_t index);
void sph_clear_particles(SPHSolver *solver);

// Simulation
void sph_step(SPHSolver *solver, float dt);
void sph_update_spatial_hash(SPHSolver *solver);
void sph_find_neighbors(SPHSolver *solver);
void sph_compute_density_pressure(SPHSolver *solver);
void sph_compute_forces(SPHSolver *solver);
void sph_integrate(SPHSolver *solver, float dt);

// Boundary handling
void sph_apply_boundary_forces(SPHSolver *solver);
void sph_clamp_to_bounds(SPHSolver *solver);

// Utilities
void sph_get_statistics(const SPHSolver *solver, float *avg_density,
                        float *max_velocity, uint32_t *particle_count);
void sph_set_gravity(SPHSolver *solver, Vec3 gravity);

// ============================================================================
// SPH Kernels (Smoothing functions)
// ============================================================================

// Poly6 kernel for density
float sph_kernel_poly6(float r, float h);
Vec3 sph_kernel_poly6_gradient(Vec3 r, float h);
float sph_kernel_poly6_laplacian(float r, float h);

// Spiky kernel for pressure
Vec3 sph_kernel_spiky_gradient(Vec3 r, float h);

// Viscosity kernel
float sph_kernel_viscosity_laplacian(float r, float h);

// ============================================================================
// Advanced Features
// ============================================================================

// PCISPH (Predictive-Corrective Incompressible SPH)
void sph_pcisph_step(SPHSolver *solver, float dt);

// Surface tension
void sph_compute_surface_tension(SPHSolver *solver);

// Two-way coupling with rigid bodies
void sph_apply_rigid_body_forces(SPHSolver *solver, void *rigid_bodies,
                                 uint32_t body_count);

#endif // SPH_SOLVER_H
