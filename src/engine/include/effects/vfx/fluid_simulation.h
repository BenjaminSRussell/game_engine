#ifndef FLUID_SIMULATION_H
#define FLUID_SIMULATION_H

#include "include/core/types.h"
#include "math/vec3.h"
#include "include/core/memory.h"

// Fluid simulation configuration
#define FLUID_MAX_PARTICLES 1000000
#define FLUID_GRID_SIZE 128
#define FLUID_SMOOTHING_RADIUS 0.5f
#define FLUID_REST_DENSITY 1000.0f
#define FLUID_GAS_CONSTANT 2000.0f
#define FLUID_VISCOSITY 0.01f
#define FLUID_SURFACE_TENSION 0.0728f
#define FLUID_GRAVITY -9.81f
#define FLUID_TIME_STEP 0.008f

// SPH Kernel types
typedef enum {
    KERNEL_POLY6,
    KERNEL_SPIKY,
    KERNEL_VISCOSITY
} SPHKernelType;

// Particle structure for GPU simulation
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
    f32 density;
    f32 pressure;
    f32 mass;
    u32 id;
} FluidParticle;

// Grid cell for spatial hashing
typedef struct {
    u32 particle_indices[64];  // Max particles per cell
    u32 particle_count;
} FluidGridCell;

// Spatial hash grid
typedef struct {
    FluidGridCell* cells;
    u32 grid_size;
    f32 cell_size;
    u32 total_cells;
} FluidSpatialGrid;

// GPU particle storage
typedef struct {
    // GPU buffers
    u32 position_buffer;
    u32 velocity_buffer;
    u32 force_buffer;
    u32 density_buffer;
    u32 pressure_buffer;
    
    // Compute shaders
    u32 density_compute_shader;
    u32 force_compute_shader;
    u32 integration_compute_shader;
    
    // Particle count
    u32 particle_count;
    u32 max_particles;
    
    // Grid data
    FluidSpatialGrid spatial_grid;
} GPUFluidStorage;

// Fluid simulation system
typedef struct {
    // Particle data
    FluidParticle* particles;
    u32 particle_count;
    u32 max_particles;
    
    // Spatial grid for neighbor search
    FluidSpatialGrid spatial_grid;
    
    // GPU storage
    GPUFluidStorage gpu_storage;
    
    // Simulation parameters
    f32 smoothing_radius;
    f32 rest_density;
    f32 gas_constant;
    f32 viscosity;
    f32 surface_tension;
    Vec3 gravity;
    f32 time_step;
    
    // Performance
    bool use_gpu_simulation;
    bool use_async_compute;
    u32 simulation_lod_levels;
    
    // Statistics
    u64 simulation_time_ms;
    u32 neighbor_search_time_ms;
    u32 density_computation_time_ms;
    u32 force_computation_time_ms;
} FluidSimulationSystem;

// Core API functions
void fluid_simulation_init(FluidSimulationSystem* system);
void fluid_simulation_shutdown(FluidSimulationSystem* system);
void fluid_simulation_update(FluidSimulationSystem* system, f32 delta_time);

// Particle management
u32 fluid_simulation_add_particle(FluidSimulationSystem* system, Vec3 position, Vec3 velocity);
void fluid_simulation_remove_particle(FluidSimulationSystem* system, u32 particle_id);
void fluid_simulation_clear_particles(FluidSimulationSystem* system);

// SPH Kernels (TASK_1800)
f32 fluid_sph_poly6_kernel(f32 r, f32 h);
f32 fluid_sph_spiky_kernel(f32 r, f32 h);
f32 fluid_sph_spiky_gradient(f32 r, f32 h);
f32 fluid_sph_viscosity_kernel(f32 r, f32 h);
f32 fluid_sph_viscosity_laplacian(f32 r, f32 h);

// Position Based Fluids (TASK_1801)
void fluid_pbf_solve_density_constraints(FluidSimulationSystem* system);
void fluid_pbf_predict_positions(FluidSimulationSystem* system, f32 delta_time);
void fluid_pbf_update_velocities(FluidSimulationSystem* system, f32 delta_time);
void fluid_pbf_apply_vorticity_confinement(FluidSimulationSystem* system);
void fluid_pbf_apply_viscosity_xspf(FluidSimulationSystem* system);

// GPU particle storage (TASK_1802)
bool fluid_gpu_storage_init(GPUFluidStorage* storage, u32 max_particles);
void fluid_gpu_storage_shutdown(GPUFluidStorage* storage);
void fluid_gpu_storage_upload_particles(GPUFluidStorage* storage, FluidParticle* particles, u32 count);
void fluid_gpu_storage_download_particles(GPUFluidStorage* storage, FluidParticle* particles, u32 count);

// Neighbor search (TASK_1810)
void fluid_spatial_grid_init(FluidSpatialGrid* grid, f32 cell_size, u32 grid_size);
void fluid_spatial_grid_shutdown(FluidSpatialGrid* grid);
void fluid_spatial_grid_update(FluidSpatialGrid* grid, FluidParticle* particles, u32 count);
void fluid_spatial_grid_find_neighbors(FluidSpatialGrid* grid, Vec3 position, f32 radius, u32* neighbor_indices, u32* neighbor_count);

// Density and pressure computation (TASK_1811)
void fluid_compute_density_pressure(FluidSimulationSystem* system);
void fluid_compute_density_gpu(FluidSimulationSystem* system);
void fluid_compute_pressure_gpu(FluidSimulationSystem* system);

// Force computation (TASK_1812)
void fluid_compute_pressure_forces(FluidSimulationSystem* system);
void fluid_compute_viscosity_forces(FluidSimulationSystem* system);
void fluid_compute_surface_tension_forces(FluidSimulationSystem* system);
void fluid_apply_external_forces(FluidSimulationSystem* system);

// Surface tension (TASK_1813)
void fluid_compute_surface_normals(FluidSimulationSystem* system);
void fluid_compute_surface_curvature(FluidSimulationSystem* system);
void fluid_apply_cohesion_forces(FluidSimulationSystem* system);

// External forces (TASK_1814)
void fluid_apply_gravity(FluidSimulationSystem* system);
void fluid_apply_wind_force(FluidSimulationSystem* system, Vec3 wind_velocity);
void fluid_apply_stirring_force(FluidSimulationSystem* system, Vec3 center, f32 radius, f32 strength);

// Fluid-solid interaction (TASK_1820)
void fluid_handle_collision_with_sphere(FluidSimulationSystem* system, Vec3 center, f32 radius);
void fluid_handle_collision_with_box(FluidSimulationSystem* system, Vec3 min, Vec3 max);
void fluid_handle_collision_with_mesh(FluidSimulationSystem* system, void* mesh_data);

// Buoyancy (TASK_1821)
void fluid_apply_buoyancy_force(FluidSimulationSystem* system, void* rigid_body);
f32 fluid_get_buoyancy_factor(FluidSimulationSystem* system, Vec3 position);

// Fluid-air interface (TASK_1822)
void fluid_generate_foam_particles(FluidSimulationSystem* system);
void fluid_generate_bubble_particles(FluidSimulationSystem* system);
void fluid_generate_spray_particles(FluidSimulationSystem* system);

// Rendering (TASK_1830-1833)
void fluid_render_screen_space(FluidSimulationSystem* system);
void fluid_render_marching_cubes(FluidSimulationSystem* system);
void fluid_render_ray_marched_volume(FluidSimulationSystem* system);
void fluid_render_refraction_caustics(FluidSimulationSystem* system);

// Optimization (TASK_1840-1842)
void fluid_enable_async_compute(FluidSimulationSystem* system, bool enable);
void fluid_set_simulation_lod(FluidSimulationSystem* system, u32 lod_level);
void fluid_optimize_grid_hash(FluidSimulationSystem* system);

// Debugging (TASK_1850-1851)
void fluid_debug_render_pressure_colors(FluidSimulationSystem* system);
void fluid_debug_render_density_colors(FluidSimulationSystem* system);
void fluid_debug_render_simulation_grid(FluidSimulationSystem* system);

#endif // FLUID_SIMULATION_H
