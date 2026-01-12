// Advanced Fluid Dynamics System
// Implements shallow water equations with GPU acceleration
#ifndef FLUID_DYNAMICS_H
#define FLUID_DYNAMICS_H

#include "include/common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include <stdbool.h>

// Grid-based fluid simulation parameters
#define FLUID_GRID_MAX_SIZE 512
#define FLUID_MIN_CELL_SIZE 0.1f
#define FLUID_MAX_DEPTH 50.0f

// Fluid simulation types
typedef enum {
  FLUID_TYPE_SHALLOW_WATER,    // 2D shallow water equations
  FLUID_TYPE_DEEP_OCEAN,        // 3D deep water simulation
  FLUID_TYPE_RIVER_FLOW,        // River channel flow
  FLUID_TYPE_POND_STILL         // Static water body
} FluidType;

// Boundary conditions
typedef enum {
  BOUNDARY_WALL,        // Solid wall (no flow)
  BOUNDARY_OPEN,        // Open boundary (free flow)
  BOUNDARY_PERIODIC,    // Periodic (wrapping)
  BOUNDARY_OUTFLOW,     // Outflow only
  BOUNDARY_INFLOW       // Inflow with specified velocity
} BoundaryType;

// Fluid cell properties
typedef struct {
  f32 height;           // Water height (m)
  f32 depth;            // Water depth (m)
  Vec2 velocity;        // Velocity field (m/s)
  f32 temperature;      // Water temperature (°C)
  f32 salinity;         // Salt concentration (ppt)
  f32 turbulence;       // Turbulence intensity
  Vec3 vorticity;       // Vorticity vector
  f32 pressure;         // Pressure field
} FluidCell;

// Simulation grid
typedef struct {
  FluidCell *cells;
  u32 width;
  u32 height;
  f32 cell_size;
  Vec3 origin;
  bool is_active;
} FluidGrid;

// Fluid body definition
typedef struct {
  FluidType type;
  FluidGrid grid;
  
  // Physical properties
  f32 gravity;
  f32 viscosity;
  f32 surface_tension;
  f32 density;
  
  // Boundary conditions
  BoundaryType boundary_north;
  BoundaryType boundary_south;
  BoundaryType boundary_east;
  BoundaryType boundary_west;
  
  // Simulation parameters
  f32 time_step;
  f32 cfl_number;       // Courant-Friedrichs-Lewy number
  u32 substeps;
  
  // External forces
  Vec2 wind_force;
  Vec3 tidal_force;
  f32 evaporation_rate;
  
  // Interaction parameters
  f32 drag_coefficient;
  f32 absorption_rate;
  
} FluidBody;

// Fluid dynamics system
typedef struct {
  FluidBody *bodies;
  u32 body_count;
  u32 max_bodies;
  
  // GPU compute resources
  void *compute_device;
  void *compute_context;
  
  // Simulation state
  f32 total_time;
  bool is_paused;
  
} FluidDynamicsSystem;

// Wave properties for ocean simulation
typedef struct {
  f32 amplitude;
  f32 frequency;
  f32 phase;
  Vec2 direction;
  f32 wavelength;
  f32 speed;
} WaveComponent;

// Ocean wave spectrum
typedef struct {
  WaveComponent *components;
  u32 component_count;
  f32 time;
  
  // Spectrum parameters
  f32 wind_speed;
  Vec2 wind_direction;
  f32 fetch_distance;
  f32 significant_height;
  
} OceanWaveSpectrum;

// Particle system for fluid effects
typedef struct {
  Vec3 *positions;
  Vec3 *velocities;
  f32 *lifetimes;
  f32 *sizes;
  u32 count;
  u32 max_count;
  
  // Emission parameters
  Vec3 emission_position;
  Vec3 emission_direction;
  f32 emission_rate;
  f32 spread_angle;
  
} FluidParticle;

// Buoyancy and interaction
typedef struct {
  Vec3 position;
  Vec3 velocity;
  f32 mass;
  f32 volume;
  f32 drag_coefficient;
  
  // Buoyancy response
  Vec3 buoyancy_force;
  Vec3 drag_force;
  f32 submerged_volume;
  bool is_submerged;
  
} BuoyantObject;

#ifdef __cplusplus
extern "C" {
#endif

// System management
FluidDynamicsSystem *fluid_dynamics_create(void);
void fluid_dynamics_destroy(FluidDynamicsSystem *system);

// Fluid body creation and management
FluidBody *fluid_create_shallow_water(FluidDynamicsSystem *system, 
                                    u32 width, u32 height, f32 cell_size);
FluidBody *fluid_create_ocean_waves(FluidDynamicsSystem *system, 
                                  f32 wind_speed, Vec2 wind_direction);
FluidBody *fluid_create_river_flow(FluidDynamicsSystem *system, 
                                  Vec3 start, Vec3 end, f32 width);

// Simulation control
void fluid_update(FluidDynamicsSystem *system, f32 delta_time);
void fluid_reset(FluidBody *body);
void fluid_pause(FluidDynamicsSystem *system);
void fluid_resume(FluidDynamicsSystem *system);

// Grid operations
void fluid_set_height(FluidBody *body, u32 x, u32 y, f32 height);
void fluid_set_velocity(FluidBody *body, u32 x, u32 y, Vec2 velocity);
f32 fluid_get_height_at(FluidBody *body, Vec3 position);
Vec2 fluid_get_velocity_at(FluidBody *body, Vec3 position);
Vec3 fluid_get_vorticity_at(FluidBody *body, Vec3 position);

// Wave generation
void fluid_generate_waves(FluidBody *body, OceanWaveSpectrum *spectrum);
void fluid_add_wave_component(FluidBody *body, WaveComponent wave);
void fluid_update_waves(FluidBody *body, f32 time);

// Boundary conditions
void fluid_set_boundary(FluidBody *body, BoundaryType north, BoundaryType south,
                       BoundaryType east, BoundaryType west);
void fluid_apply_boundary_conditions(FluidBody *body);

// External forces
void fluid_apply_wind(FluidBody *body, Vec2 wind_force);
void fluid_apply_rainfall(FluidBody *body, Vec3 position, f32 intensity);
void fluid_apply_temperature_gradient(FluidBody *body, f32 temperature);

// Interaction with objects
void fluid_add_buoyant_object(FluidBody *body, BuoyantObject *object);
void fluid_remove_buoyant_object(FluidBody *body, BuoyantObject *object);
void fluid_update_buoyancy(FluidBody *body, BuoyantObject *object);

// Particle effects
FluidParticle *fluid_create_spray_particles(FluidBody *body, Vec3 position, u32 count);
FluidParticle *fluid_create_foam_particles(FluidBody *body, Vec3 position, u32 count);
void fluid_update_particles(FluidParticle *particles, f32 delta_time);

// Rendering support
void fluid_generate_mesh(FluidBody *body, Vec3 *vertices, u32 *indices, 
                        u32 *vertex_count, u32 *index_count);
void fluid_get_surface_normals(FluidBody *body, Vec3 *normals);
void fluid_get_surface_colors(FluidBody *body, Vec4 *colors);

// Debugging and analysis
f32 fluid_calculate_total_volume(FluidBody *body);
f32 fluid_calculate_kinetic_energy(FluidBody *body);
Vec3 fluid_calculate_momentum(FluidBody *body);
void fluid_validate_simulation(FluidBody *body);

#ifdef __cplusplus
}
#endif

#endif // FLUID_DYNAMICS_H
