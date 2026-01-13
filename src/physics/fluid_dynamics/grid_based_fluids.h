/**
 * @file grid_based_fluids.h
 * @brief Grid-based fluid dynamics simulation system
 */

#ifndef RENDER_GRID_BASED_FLUIDS_H
#define RENDER_GRID_BASED_FLUIDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Grid-based fluid simulation parameters
#define MAX_GRID_SIZE 128
#define MAX_PARTICLES 10000

// Fluid simulation types
typedef struct {
    float u, v, w;        // Velocity components
    float p;              // Pressure
    float div;            // Divergence
} FluidCell;

typedef struct {
    float x, y, z;        // Position
    float vx, vy, vz;     // Velocity
    float life;           // Particle lifetime
} FluidParticle;

typedef struct {
    int width, height, depth;
    FluidCell cells[MAX_GRID_SIZE][MAX_GRID_SIZE][MAX_GRID_SIZE];
    FluidParticle particles[MAX_PARTICLES];
    int particle_count;
    float viscosity;
    float diffusion;
    float dt;
} FluidGrid;

// Function declarations
FluidGrid* fluid_grid_create(int width, int height, int depth);
void fluid_grid_destroy(FluidGrid* grid);
void fluid_grid_update(FluidGrid* grid, float dt);
void fluid_grid_add_density(FluidGrid* grid, int x, int y, int z, float amount);
void fluid_grid_add_velocity(FluidGrid* grid, int x, int y, int z, float vx, float vy, float vz);
void fluid_grid_diffuse(FluidGrid* grid, float diff, float dt);
void fluid_grid_advect(FluidGrid* grid, float dt);
void fluid_grid_project(FluidGrid* grid);
void fluid_grid_set_boundaries(FluidGrid* grid);

// GPU profiling hooks
void fluid_grid_start_gpu_profile();
void fluid_grid_end_gpu_profile();
float fluid_grid_get_gpu_time();

#ifdef __cplusplus
}
#endif

#endif /* RENDER_GRID_BASED_FLUIDS_H */
