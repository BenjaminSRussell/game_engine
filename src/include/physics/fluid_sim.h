// Physics/Fluid Simulation (Grid-based and Particle-based)
#ifndef FLUID_SIM_H
#define FLUID_SIM_H

#include "include/common.h"
#include "math/vec3.h"
#import <Metal/Metal.h>

// Grid-based fluid (Successive Over-Relaxation)
#define FLUID_GRID_SIZE 128

typedef struct {
  f32 density[FLUID_GRID_SIZE * FLUID_GRID_SIZE * FLUID_GRID_SIZE];
  Vec3 velocity[FLUID_GRID_SIZE * FLUID_GRID_SIZE * FLUID_GRID_SIZE];
  f32 pressure[FLUID_GRID_SIZE * FLUID_GRID_SIZE * FLUID_GRID_SIZE];
  f32 divergence[FLUID_GRID_SIZE * FLUID_GRID_SIZE * FLUID_GRID_SIZE];

  // Simulation parameters
  f32 viscosity;
  f32 diffusion;
  f32 decay;
  f32 vorticity_confinement;

  // GPU resources
  id<MTLTexture> density_texture;
  id<MTLTexture> velocity_texture;
  id<MTLTexture> pressure_texture;

  id<MTLComputePipelineState> advect_pipeline;
  id<MTLComputePipelineState> divergence_pipeline;
  id<MTLComputePipelineState> jacobi_pipeline;
  id<MTLComputePipelineState> projection_pipeline;

} FluidGrid;

// Particle-based fluid (SPH - Smoothed Particle Hydrodynamics)
typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 force;
  f32 density;
  f32 pressure;
  u32 grid_cell_index;
} FluidParticle;

typedef struct {
  FluidParticle *particles;
  u32 particle_count;
  u32 max_particles;

  // SPH Constants
  f32 smoothing_radius;
  f32 target_density;
  f32 pressure_multiplier;
  f32 viscosity;
  f32 gravity;

  // Spatial hashing for neighbor search
  u32 *spatial_grid;  // Head pointers
  u32 *next_particle; // Linked list
  u32 grid_size;

  // GPU resources
  id<MTLBuffer> particle_buffer;
  id<MTLBuffer> grid_buffer;
  id<MTLComputePipelineState> density_pressure_pipeline;
  id<MTLComputePipelineState> force_pipeline;
  id<MTLComputePipelineState> integrate_pipeline;

} FluidSPH;

#ifdef __cplusplus
extern "C" {
#endif

// Grid Fluid
FluidGrid *fluid_grid_create(id<MTLDevice> device);
void fluid_grid_destroy(FluidGrid *fluid);
void fluid_grid_update(FluidGrid *fluid, id<MTLCommandBuffer> cmd,
                       f32 delta_time);
void fluid_grid_add_density(FluidGrid *fluid, Vec3 pos, f32 amount, f32 radius);
void fluid_grid_add_velocity(FluidGrid *fluid, Vec3 pos, Vec3 velocity,
                             f32 radius);

// SPH Fluid
FluidSPH *fluid_sph_create(id<MTLDevice> device, u32 max_particles);
void fluid_sph_destroy(FluidSPH *fluid);
void fluid_sph_update(FluidSPH *fluid, id<MTLCommandBuffer> cmd,
                      f32 delta_time);
void fluid_sph_spawn_emitter(FluidSPH *fluid, Vec3 position, Vec3 direction,
                             f32 rate);

#ifdef __cplusplus
}
#endif

#endif // FLUID_SIM_H
