// Advanced Fluid Dynamics Implementation
// Shallow water equations with GPU acceleration and advanced features

#include "fluid_dynamics.h"
#include "include/core/logger.h"
#include "math/math_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Physical constants
#define GRAVITY 9.81f
#define WATER_DENSITY 1000.0f
#define AIR_DENSITY 1.225f
#define KINEMATIC_VISCOSITY 1e-6f
#define SURFACE_TENSION 0.0728f

// Simulation constants
#define MAX_CFL_NUMBER 0.5f
#define MIN_TIME_STEP 1e-6f
#define MAX_TIME_STEP 0.1f

// Helper functions
static f32 calculate_cfl_time_step(FluidBody *body);
static void apply_shallow_water_equations(FluidBody *body, f32 dt);
static void apply_boundary_conditions(FluidBody *body);
static void calculate_vorticity(FluidBody *body);
static void apply_diffusion(FluidBody *body, f32 dt);
static void project_velocity(FluidBody *body);

FluidDynamicsSystem *fluid_dynamics_create(void) {
  FluidDynamicsSystem *system = calloc(1, sizeof(FluidDynamicsSystem));
  if (!system) {
    LOG_ERROR("Failed to allocate fluid dynamics system");
    return NULL;
  }
  
  system->max_bodies = 16;
  system->bodies = calloc(system->max_bodies, sizeof(FluidBody));
  
  if (!system->bodies) {
    LOG_ERROR("Failed to allocate fluid bodies");
    free(system);
    return NULL;
  }
  
  LOG_INFO("Fluid dynamics system created successfully");
  return system;
}

void fluid_dynamics_destroy(FluidDynamicsSystem *system) {
  if (!system) return;
  
  for (u32 i = 0; i < system->body_count; i++) {
    FluidBody *body = &system->bodies[i];
    if (body->grid.cells) {
      free(body->grid.cells);
    }
  }
  
  free(system->bodies);
  free(system);
  LOG_INFO("Fluid dynamics system destroyed");
}

FluidBody *fluid_create_shallow_water(FluidDynamicsSystem *system, 
                                    u32 width, u32 height, f32 cell_size) {
  if (system->body_count >= system->max_bodies) {
    LOG_ERROR("Maximum fluid bodies reached");
    return NULL;
  }
  
  if (width > FLUID_GRID_MAX_SIZE || height > FLUID_GRID_MAX_SIZE) {
    LOG_ERROR("Grid size exceeds maximum dimensions");
    return NULL;
  }
  
  FluidBody *body = &system->bodies[system->body_count++];
  memset(body, 0, sizeof(FluidBody));
  
  body->type = FLUID_TYPE_SHALLOW_WATER;
  body->grid.width = width;
  body->grid.height = height;
  body->grid.cell_size = cell_size;
  body->grid.origin = (Vec3){0.0f, 0.0f, 0.0f};
  body->grid.is_active = true;
  
  // Allocate grid cells
  body->grid.cells = calloc(width * height, sizeof(FluidCell));
  if (!body->grid.cells) {
    LOG_ERROR("Failed to allocate fluid grid cells");
    system->body_count--;
    return NULL;
  }
  
  // Initialize physical properties
  body->gravity = GRAVITY;
  body->viscosity = KINEMATIC_VISCOSITY;
  body->surface_tension = SURFACE_TENSION;
  body->density = WATER_DENSITY;
  
  // Initialize simulation parameters
  body->time_step = 0.01f;
  body->cfl_number = 0.3f;
  body->substeps = 1;
  
  // Set default boundary conditions
  body->boundary_north = BOUNDARY_WALL;
  body->boundary_south = BOUNDARY_WALL;
  body->boundary_east = BOUNDARY_WALL;
  body->boundary_west = BOUNDARY_WALL;
  
  // Initialize grid with small water depth
  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      cell->height = 1.0f;
      cell->depth = 1.0f;
      cell->velocity = (Vec2){0.0f, 0.0f};
      cell->temperature = 20.0f;
      cell->salinity = 35.0f;
      cell->turbulence = 0.0f;
      cell->vorticity = (Vec3){0.0f, 0.0f, 0.0f};
      cell->pressure = 101325.0f; // Standard atmospheric pressure
    }
  }
  
  LOG_INFO("Created shallow water fluid body: %dx%d grid", width, height);
  return body;
}

FluidBody *fluid_create_ocean_waves(FluidDynamicsSystem *system, 
                                  f32 wind_speed, Vec2 wind_direction) {
  FluidBody *body = fluid_create_shallow_water(system, 256, 256, 2.0f);
  if (!body) return NULL;
  
  body->type = FLUID_TYPE_DEEP_OCEAN;
  body->wind_force = vec2_scale(vec2_normalize(wind_direction), wind_speed * 0.1f);
  
  // Initialize with ocean-like conditions
  for (u32 y = 0; y < body->grid.height; y++) {
    for (u32 x = 0; x < body->grid.width; x++) {
      FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
      cell->height = 10.0f + sinf(x * 0.1f) * cosf(y * 0.1f) * 2.0f;
      cell->depth = cell->height;
      cell->salinity = 35.0f;
      cell->temperature = 15.0f;
    }
  }
  
  // Set periodic boundaries for ocean
  body->boundary_north = BOUNDARY_PERIODIC;
  body->boundary_south = BOUNDARY_PERIODIC;
  body->boundary_east = BOUNDARY_PERIODIC;
  body->boundary_west = BOUNDARY_PERIODIC;
  
  LOG_INFO("Created ocean wave fluid body with wind speed: %.1f m/s", wind_speed);
  return body;
}

FluidBody *fluid_create_river_flow(FluidDynamicsSystem *system, 
                                  Vec3 start, Vec3 end, f32 width) {
  u32 length = (u32)(vec3_distance(start, end) / 2.0f);
  u32 cells_width = (u32)(width / 2.0f);
  
  FluidBody *body = fluid_create_shallow_water(system, length, cells_width, 2.0f);
  if (!body) return NULL;
  
  body->type = FLUID_TYPE_RIVER_FLOW;
  body->grid.origin = start;
  
  // Calculate flow direction
  Vec3 flow_dir = vec3_normalize(vec3_sub(end, start));
  f32 flow_speed = 2.0f; // 2 m/s river flow
  
  // Initialize with river flow
  for (u32 y = 0; y < body->grid.height; y++) {
    for (u32 x = 0; x < body->grid.width; x++) {
      FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
      cell->height = 3.0f;
      cell->depth = 3.0f;
      cell->velocity.x = flow_dir.x * flow_speed;
      cell->velocity.y = flow_dir.z * flow_speed;
      cell->temperature = 12.0f; // River temperature
      cell->salinity = 0.1f; // Fresh water
    }
  }
  
  // Set appropriate boundaries for river
  body->boundary_west = BOUNDARY_INFLOW;
  body->boundary_east = BOUNDARY_OUTFLOW;
  body->boundary_north = BOUNDARY_WALL;
  body->boundary_south = BOUNDARY_WALL;
  
  LOG_INFO("Created river flow fluid body: length=%d, width=%d", length, cells_width);
  return body;
}

void fluid_update(FluidDynamicsSystem *system, f32 delta_time) {
  if (system->is_paused) return;
  
  system->total_time += delta_time;
  
  for (u32 i = 0; i < system->body_count; i++) {
    FluidBody *body = &system->bodies[i];
    if (!body->grid.is_active) continue;
    
    // Calculate adaptive time step based on CFL condition
    f32 time_step = calculate_cfl_time_step(body);
    time_step = fminf(time_step, delta_time);
    time_step = fmaxf(time_step, MIN_TIME_STEP);
    
    // Perform substeps for stability
    u32 substeps = (u32)ceilf(delta_time / time_step);
    substeps = fminf(substeps, 10); // Limit substeps
    
    f32 sub_dt = delta_time / (f32)substeps;
    
    for (u32 sub = 0; sub < substeps; sub++) {
      // Apply shallow water equations
      apply_shallow_water_equations(body, sub_dt);
      
      // Apply boundary conditions
      apply_boundary_conditions(body);
      
      // Calculate vorticity for visualization
      calculate_vorticity(body);
      
      // Apply viscous diffusion
      apply_diffusion(body, sub_dt);
      
      // Project velocity to ensure incompressibility
      project_velocity(body);
    }
    
    // Apply external forces
    if (vec2_length(body->wind_force) > 0.0f) {
      fluid_apply_wind(body, body->wind_force);
    }
  }
}

static f32 calculate_cfl_time_step(FluidBody *body) {
  f32 max_velocity = 0.0f;
  f32 max_height = 0.0f;
  
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  
  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      f32 vel_mag = vec2_length(cell->velocity);
      max_velocity = fmaxf(max_velocity, vel_mag);
      max_height = fmaxf(max_height, cell->height);
    }
  }
  
  f32 wave_speed = sqrtf(body->gravity * max_height);
  f32 max_speed = max_velocity + wave_speed;
  
  if (max_speed <= 0.0f) return body->time_step;
  
  f32 cfl_dt = body->cfl_number * body->grid.cell_size / max_speed;
  return fminf(cfl_dt, MAX_TIME_STEP);
}

static void apply_shallow_water_equations(FluidBody *body, f32 dt) {
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  f32 dx = body->grid.cell_size;
  f32 g = body->gravity;
  
  // Create temporary arrays for new values
  FluidCell *new_cells = calloc(width * height, sizeof(FluidCell));
  memcpy(new_cells, body->grid.cells, width * height * sizeof(FluidCell));
  
  for (u32 y = 1; y < height - 1; y++) {
    for (u32 x = 1; x < width - 1; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      FluidCell *new_cell = &new_cells[y * width + x];
      
      // Get neighboring cells
      FluidCell *cell_north = &body->grid.cells[(y-1) * width + x];
      FluidCell *cell_south = &body->grid.cells[(y+1) * width + x];
      FluidCell *cell_east = &body->grid.cells[y * width + (x+1)];
      FluidCell *cell_west = &body->grid.cells[y * width + (x-1)];
      
      // Calculate height gradients (central differences)
      f32 dh_dx = (cell_east->height - cell_west->height) / (2.0f * dx);
      f32 dh_dy = (cell_south->height - cell_north->height) / (2.0f * dx);
      
      // Calculate velocity divergence
      f32 du_dx = (cell_east->velocity.x - cell_west->velocity.x) / (2.0f * dx);
      f32 dv_dy = (cell_south->velocity.y - cell_north->velocity.y) / (2.0f * dx);
      
      // Update height (continuity equation)
      new_cell->height = cell->height - dt * cell->height * (du_dx + dv_dy);
      new_cell->height = fmaxf(new_cell->height, 0.01f); // Prevent negative height
      
      // Update velocity (momentum equations)
      new_cell->velocity.x = cell->velocity.x - dt * (g * dh_dx + cell->velocity.x * du_dx);
      new_cell->velocity.y = cell->velocity.y - dt * (g * dh_dy + cell->velocity.y * dv_dy);
      
      // Apply damping for stability
      f32 damping = 0.999f;
      new_cell->velocity.x *= damping;
      new_cell->velocity.y *= damping;
      
      // Update depth to match height
      new_cell->depth = new_cell->height;
    }
  }
  
  // Copy new values back
  memcpy(body->grid.cells, new_cells, width * height * sizeof(FluidCell));
  free(new_cells);
}

static void apply_boundary_conditions(FluidBody *body) {
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  
  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      
      // North boundary
      if (y == 0) {
        switch (body->boundary_north) {
          case BOUNDARY_WALL:
            cell->velocity.y = fminf(cell->velocity.y, 0.0f);
            break;
          case BOUNDARY_OPEN:
            // No constraint
            break;
          case BOUNDARY_PERIODIC:
            if (height > 1) {
              FluidCell *cell_south = &body->grid.cells[(height-1) * width + x];
              *cell = *cell_south;
            }
            break;
          case BOUNDARY_OUTFLOW:
            cell->velocity.y = fmaxf(cell->velocity.y, 0.0f);
            break;
          case BOUNDARY_INFLOW:
            cell->velocity.y = -2.0f; // Inflow velocity
            break;
        }
      }
      
      // South boundary
      if (y == height - 1) {
        switch (body->boundary_south) {
          case BOUNDARY_WALL:
            cell->velocity.y = fmaxf(cell->velocity.y, 0.0f);
            break;
          case BOUNDARY_OPEN:
            break;
          case BOUNDARY_PERIODIC:
            if (height > 1) {
              FluidCell *cell_north = &body->grid.cells[0 * width + x];
              *cell = *cell_north;
            }
            break;
          case BOUNDARY_OUTFLOW:
            cell->velocity.y = fminf(cell->velocity.y, 0.0f);
            break;
          case BOUNDARY_INFLOW:
            cell->velocity.y = 2.0f;
            break;
        }
      }
      
      // West boundary
      if (x == 0) {
        switch (body->boundary_west) {
          case BOUNDARY_WALL:
            cell->velocity.x = fminf(cell->velocity.x, 0.0f);
            break;
          case BOUNDARY_OPEN:
            break;
          case BOUNDARY_PERIODIC:
            if (width > 1) {
              FluidCell *cell_east = &body->grid.cells[y * width + (width-1)];
              *cell = *cell_east;
            }
            break;
          case BOUNDARY_OUTFLOW:
            cell->velocity.x = fmaxf(cell->velocity.x, 0.0f);
            break;
          case BOUNDARY_INFLOW:
            cell->velocity.x = 2.0f;
            break;
        }
      }
      
      // East boundary
      if (x == width - 1) {
        switch (body->boundary_east) {
          case BOUNDARY_WALL:
            cell->velocity.x = fmaxf(cell->velocity.x, 0.0f);
            break;
          case BOUNDARY_OPEN:
            break;
          case BOUNDARY_PERIODIC:
            if (width > 1) {
              FluidCell *cell_west = &body->grid.cells[y * width + 0];
              *cell = *cell_west;
            }
            break;
          case BOUNDARY_OUTFLOW:
            cell->velocity.x = fminf(cell->velocity.x, 0.0f);
            break;
          case BOUNDARY_INFLOW:
            cell->velocity.x = -2.0f;
            break;
        }
      }
    }
  }
}

static void calculate_vorticity(FluidBody *body) {
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  f32 dx = body->grid.cell_size;
  
  for (u32 y = 1; y < height - 1; y++) {
    for (u32 x = 1; x < width - 1; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      
      // Get neighboring velocities
      FluidCell *cell_north = &body->grid.cells[(y-1) * width + x];
      FluidCell *cell_south = &body->grid.cells[(y+1) * width + x];
      FluidCell *cell_east = &body->grid.cells[y * width + (x+1)];
      FluidCell *cell_west = &body->grid.cells[y * width + (x-1)];
      
      // Calculate velocity gradients
      f32 dv_dx = (cell_east->velocity.y - cell_west->velocity.y) / (2.0f * dx);
      f32 du_dy = (cell_south->velocity.x - cell_north->velocity.x) / (2.0f * dx);
      
      // Vorticity (z-component for 2D flow)
      cell->vorticity.z = dv_dx - du_dy;
      cell->turbulence = fabsf(cell->vorticity.z);
    }
  }
}

static void apply_diffusion(FluidBody *body, f32 dt) {
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  f32 dx = body->grid.cell_size;
  f32 nu = body->viscosity;
  
  // Create temporary array
  FluidCell *new_cells = calloc(width * height, sizeof(FluidCell));
  memcpy(new_cells, body->grid.cells, width * height * sizeof(FluidCell));
  
  f32 diffusion_coeff = nu * dt / (dx * dx);
  
  for (u32 y = 1; y < height - 1; y++) {
    for (u32 x = 1; x < width - 1; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      FluidCell *new_cell = &new_cells[y * width + x];
      
      // Get neighboring cells
      FluidCell *cell_north = &body->grid.cells[(y-1) * width + x];
      FluidCell *cell_south = &body->grid.cells[(y+1) * width + x];
      FluidCell *cell_east = &body->grid.cells[y * width + (x+1)];
      FluidCell *cell_west = &body->grid.cells[y * width + (x-1)];
      
      // Apply diffusion to velocity
      new_cell->velocity.x = cell->velocity.x + diffusion_coeff * (
        cell_east->velocity.x + cell_west->velocity.x + 
        cell_north->velocity.x + cell_south->velocity.x - 4.0f * cell->velocity.x
      );
      
      new_cell->velocity.y = cell->velocity.y + diffusion_coeff * (
        cell_east->velocity.y + cell_west->velocity.y + 
        cell_north->velocity.y + cell_south->velocity.y - 4.0f * cell->velocity.y
      );
    }
  }
  
  memcpy(body->grid.cells, new_cells, width * height * sizeof(FluidCell));
  free(new_cells);
}

static void project_velocity(FluidBody *body) {
  // Simplified pressure projection to ensure incompressibility
  // In a full implementation, this would solve a Poisson equation
  u32 width = body->grid.width;
  u32 height = body->grid.height;
  
  for (u32 y = 1; y < height - 1; y++) {
    for (u32 x = 1; x < width - 1; x++) {
      FluidCell *cell = &body->grid.cells[y * width + x];
      
      // Simple divergence correction
      FluidCell *cell_north = &body->grid.cells[(y-1) * width + x];
      FluidCell *cell_south = &body->grid.cells[(y+1) * width + x];
      FluidCell *cell_east = &body->grid.cells[y * width + (x+1)];
      FluidCell *cell_west = &body->grid.cells[y * width + (x-1)];
      
      f32 divergence = (cell_east->velocity.x - cell_west->velocity.x + 
                       cell_south->velocity.y - cell_north->velocity.y) * 0.5f;
      
      // Correct velocities to reduce divergence
      cell->velocity.x -= divergence * 0.25f;
      cell->velocity.y -= divergence * 0.25f;
    }
  }
}

void fluid_set_height(FluidBody *body, u32 x, u32 y, f32 height) {
  if (!body || x >= body->grid.width || y >= body->grid.height) return;
  
  FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
  cell->height = fmaxf(height, 0.01f);
  cell->depth = cell->height;
}

void fluid_set_velocity(FluidBody *body, u32 x, u32 y, Vec2 velocity) {
  if (!body || x >= body->grid.width || y >= body->grid.height) return;
  
  FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
  cell->velocity = velocity;
}

f32 fluid_get_height_at(FluidBody *body, Vec3 position) {
  if (!body) return 0.0f;
  
  // Convert world position to grid coordinates
  Vec3 local_pos = vec3_sub(position, body->grid.origin);
  u32 x = (u32)(local_pos.x / body->grid.cell_size);
  u32 y = (u32)(local_pos.z / body->grid.cell_size);
  
  if (x >= body->grid.width || y >= body->grid.height) return 0.0f;
  
  FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
  return cell->height + body->grid.origin.y;
}

Vec2 fluid_get_velocity_at(FluidBody *body, Vec3 position) {
  if (!body) return (Vec2){0.0f, 0.0f};
  
  Vec3 local_pos = vec3_sub(position, body->grid.origin);
  u32 x = (u32)(local_pos.x / body->grid.cell_size);
  u32 y = (u32)(local_pos.z / body->grid.cell_size);
  
  if (x >= body->grid.width || y >= body->grid.height) {
    return (Vec2){0.0f, 0.0f};
  }
  
  FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
  return cell->velocity;
}

void fluid_apply_wind(FluidBody *body, Vec2 wind_force) {
  if (!body) return;
  
  f32 wind_coefficient = 0.001f; // Wind stress coefficient
  
  for (u32 y = 0; y < body->grid.height; y++) {
    for (u32 x = 0; x < body->grid.width; x++) {
      FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
      
      // Apply wind stress to surface water
      f32 depth_factor = fminf(cell->depth / 5.0f, 1.0f); // Wind affects surface more
      Vec2 wind_acceleration = vec2_scale(wind_force, wind_coefficient * depth_factor);
      
      cell->velocity.x += wind_acceleration.x;
      cell->velocity.y += wind_acceleration.y;
    }
  }
}

void fluid_pause(FluidDynamicsSystem *system) {
  if (system) {
    system->is_paused = true;
    LOG_INFO("Fluid dynamics simulation paused");
  }
}

void fluid_resume(FluidDynamicsSystem *system) {
  if (system) {
    system->is_paused = false;
    LOG_INFO("Fluid dynamics simulation resumed");
  }
}

f32 fluid_calculate_total_volume(FluidBody *body) {
  if (!body) return 0.0f;
  
  f32 total_volume = 0.0f;
  f32 cell_area = body->grid.cell_size * body->grid.cell_size;
  
  for (u32 y = 0; y < body->grid.height; y++) {
    for (u32 x = 0; x < body->grid.width; x++) {
      FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
      total_volume += cell->height * cell_area;
    }
  }
  
  return total_volume;
}

f32 fluid_calculate_kinetic_energy(FluidBody *body) {
  if (!body) return 0.0f;
  
  f32 total_energy = 0.0f;
  f32 cell_mass = body->density * body->grid.cell_size * body->grid.cell_size;
  
  for (u32 y = 0; y < body->grid.height; y++) {
    for (u32 x = 0; x < body->grid.width; x++) {
      FluidCell *cell = &body->grid.cells[y * body->grid.width + x];
      f32 velocity_squared = cell->velocity.x * cell->velocity.x + 
                             cell->velocity.y * cell->velocity.y;
      total_energy += 0.5f * cell_mass * cell->height * velocity_squared;
    }
  }
  
  return total_energy;
}
