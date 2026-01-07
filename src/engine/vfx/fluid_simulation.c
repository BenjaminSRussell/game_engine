#include "../include/vfx/fluid_simulation.h"
#include "../core/logger.h"
#include "../core/memory.h"
#include "../core/math.h"
#include <string.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   FLUID SIMULATION (GPU) - IMPLEMENTATION
 * =================================================================================================
 */

// TASK_1800: Implement "SPH" (Smoothed Particle Hydrodynamics) Kernels
f32 fluid_sph_poly6_kernel(f32 r, f32 h) {
    if (r >= 0.0f && r <= h) {
        f32 h2 = h * h;
        f32 h9 = h2 * h2 * h2 * h2 * h;  // h^9
        f32 r2 = r * r;
        return (315.0f / (64.0f * PI_F * h9)) * powf(h2 - r2, 3.0f);
    }
    return 0.0f;
}

f32 fluid_sph_spiky_kernel(f32 r, f32 h) {
    if (r >= 0.0f && r <= h) {
        f32 h6 = h * h * h * h * h * h;  // h^6
        f32 hr = h - r;
        return (15.0f / (PI_F * h6)) * powf(hr, 3.0f);
    }
    return 0.0f;
}

f32 fluid_sph_spiky_gradient(f32 r, f32 h) {
    if (r >= 0.0001f && r <= h) {
        f32 h6 = h * h * h * h * h * h;  // h^6
        f32 hr = h - r;
        return -(45.0f / (PI_F * h6)) * powf(hr, 2.0f);
    }
    return 0.0f;
}

f32 fluid_sph_viscosity_kernel(f32 r, f32 h) {
    if (r >= 0.0f && r <= h) {
        f32 h3 = h * h * h;
        f32 r2 = r * r;
        f32 h2 = h * h;
        return (15.0f / (2.0f * PI_F * h3)) * (-r2 / (2.0f * h2) + r / h + 1.0f - (h / (2.0f * r)));
    }
    return 0.0f;
}

f32 fluid_sph_viscosity_laplacian(f32 r, f32 h) {
    if (r >= 0.0f && r <= h) {
        f32 h6 = h * h * h * h * h * h;  // h^6
        return (45.0f / (PI_F * h6)) * (h - r);
    }
    return 0.0f;
}

// TASK_1801: Implement "PBF" (Position Based Fluids) for incompressible looks
void fluid_pbf_solve_density_constraints(FluidSimulationSystem* system) {
    if (!system) return;
    
    const u32 max_iterations = 3;
    const f32 relaxation = 0.5f;
    
    for (u32 iter = 0; iter < max_iterations; iter++) {
        // Compute lambda values for each particle
        for (u32 i = 0; i < system->particle_count; i++) {
            FluidParticle* pi = &system->particles[i];
            
            f32 density_constraint = pi->density - system->rest_density;
            f32 gradient_sum = 0.0f;
            
            // Find neighbors using spatial grid
            u32 neighbor_indices[64];
            u32 neighbor_count = 0;
            fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                             system->smoothing_radius, 
                                             neighbor_indices, &neighbor_count);
            
            // Compute gradient of constraint function
            for (u32 j = 0; j < neighbor_count; j++) {
                u32 neighbor_idx = neighbor_indices[j];
                FluidParticle* pj = &system->particles[neighbor_idx];
                
                Vec3 diff = vec3_sub(pi->position, pj->position);
                f32 r = vec3_length(diff);
                
                if (r > 0.0001f && r < system->smoothing_radius) {
                    Vec3 gradient = vec3_scale(diff, fluid_sph_spiky_gradient(r, system->smoothing_radius));
                    gradient_sum += vec3_length_squared(gradient) / (system->rest_density * system->rest_density);
                }
            }
            
            // Self gradient
            gradient_sum += vec3_length_squared(vec3_zero()) / (system->rest_density * system->rest_density);
            
            // Store lambda (inverse of constraint gradient magnitude)
            pi->density = -density_constraint / (gradient_sum + 600.0f);  // 600 is relaxation parameter
        }
        
        // Update positions based on constraints
        for (u32 i = 0; i < system->particle_count; i++) {
            FluidParticle* pi = &system->particles[i];
            Vec3 delta_position = vec3_zero();
            
            // Find neighbors
            u32 neighbor_indices[64];
            u32 neighbor_count = 0;
            fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                             system->smoothing_radius, 
                                             neighbor_indices, &neighbor_count);
            
            // Compute position correction
            for (u32 j = 0; j < neighbor_count; j++) {
                u32 neighbor_idx = neighbor_indices[j];
                FluidParticle* pj = &system->particles[neighbor_idx];
                
                Vec3 diff = vec3_sub(pi->position, pj->position);
                f32 r = vec3_length(diff);
                
                if (r > 0.0001f && r < system->smoothing_radius) {
                    f32 s_corr = -0.0001f * powf(r / system->smoothing_radius, 6.0f);  // Surface tension
                    f32 gradient = fluid_sph_spiky_gradient(r, system->smoothing_radius);
                    Vec3 correction = vec3_scale(diff, (pi->density + pj->density + s_corr) * gradient);
                    delta_position = vec3_add(delta_position, correction);
                }
            }
            
            // Apply position correction
            delta_position = vec3_scale(delta_position, relaxation);
            pi->position = vec3_add(pi->position, delta_position);
        }
    }
}

void fluid_pbf_predict_positions(FluidSimulationSystem* system, f32 delta_time) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* particle = &system->particles[i];
        
        // Store old position
        Vec3 old_position = particle->position;
        
        // Apply external forces
        Vec3 acceleration = vec3_scale(particle->force, 1.0f / particle->mass);
        acceleration = vec3_add(acceleration, system->gravity);
        
        // Predict new position using semi-implicit Euler
        particle->velocity = vec3_add(particle->velocity, vec3_scale(acceleration, delta_time));
        particle->position = vec3_add(old_position, vec3_scale(particle->velocity, delta_time));
    }
}

void fluid_pbf_update_velocities(FluidSimulationSystem* system, f32 delta_time) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* particle = &system->particles[i];
        
        // Update velocity based on position change
        // Note: This requires storing old positions from prediction step
        // For now, we'll use a simplified approach
        particle->velocity = vec3_scale(particle->force, delta_time / particle->mass);
    }
}

void fluid_pbf_apply_vorticity_confinement(FluidSimulationSystem* system) {
    if (!system) return;
    
    const f32 epsilon = 0.0001f;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        
        Vec3 vorticity = vec3_zero();
        Vec3 omega_gradient = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute vorticity
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pj->position, pi->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                Vec3 velocity_diff = vec3_sub(pj->velocity, pi->velocity);
                Vec3 gradient = vec3_scale(diff, fluid_sph_spiky_gradient(r, system->smoothing_radius));
                vorticity = vec3_add(vorticity, vec3_cross(velocity_diff, gradient));
            }
        }
        
        // Compute vorticity confinement force
        if (vec3_length_squared(vorticity) > epsilon) {
            Vec3 eta = vec3_zero();
            
            for (u32 j = 0; j < neighbor_count; j++) {
                u32 neighbor_idx = neighbor_indices[j];
                FluidParticle* pj = &system->particles[neighbor_idx];
                
                Vec3 diff = vec3_sub(pj->position, pi->position);
                f32 r = vec3_length(diff);
                
                if (r > 0.0001f && r < system->smoothing_radius) {
                    f32 neighbor_vorticity = vec3_length(vorticity);
                    Vec3 gradient = vec3_scale(diff, fluid_sph_spiky_gradient(r, system->smoothing_radius));
                    eta = vec3_add(eta, vec3_scale(gradient, neighbor_vorticity));
                }
            }
            
            if (vec3_length_squared(eta) > epsilon) {
                eta = vec3_normalize(eta);
                Vec3 force = vec3_scale(vec3_cross(eta, vorticity), 0.01f);  // Vorticity confinement strength
                particle->force = vec3_add(particle->force, force);
            }
        }
    }
}

void fluid_pbf_apply_viscosity_xspf(FluidSimulationSystem* system) {
    if (!system) return;
    
    const f32 c = 0.01f;  // Viscosity coefficient
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        Vec3 viscosity_force = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute viscosity force
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pj->position, pi->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                Vec3 velocity_diff = vec3_sub(pj->velocity, pi->velocity);
                f32 kernel = fluid_sph_viscosity_kernel(r, system->smoothing_radius);
                viscosity_force = vec3_add(viscosity_force, vec3_scale(velocity_diff, kernel));
            }
        }
        
        // Apply viscosity force
        viscosity_force = vec3_scale(viscosity_force, c);
        pi->force = vec3_add(pi->force, viscosity_force);
    }
}

// TASK_1802: Setup GPU particle storage for fluid simulation
bool fluid_gpu_storage_init(GPUFluidStorage* storage, u32 max_particles) {
    if (!storage || max_particles == 0) return false;
    
    memset(storage, 0, sizeof(GPUFluidStorage));
    storage->max_particles = max_particles;
    
    // Create GPU buffers (simplified - in real implementation would use OpenGL/Vulkan/DirectX)
    storage->position_buffer = 0;  // Would be GPU buffer ID
    storage->velocity_buffer = 0;
    storage->force_buffer = 0;
    storage->density_buffer = 0;
    storage->pressure_buffer = 0;
    
    // Create compute shaders (simplified)
    storage->density_compute_shader = 0;
    storage->force_compute_shader = 0;
    storage->integration_compute_shader = 0;
    
    // Initialize spatial grid for GPU
    fluid_spatial_grid_init(&storage->spatial_grid, FLUID_SMOOTHING_RADIUS, FLUID_GRID_SIZE);
    
    LOG_INFO("GPU fluid storage initialized for %u particles", max_particles);
    return true;
}

void fluid_gpu_storage_shutdown(GPUFluidStorage* storage) {
    if (!storage) return;
    
    // Clean up GPU buffers (simplified)
    storage->position_buffer = 0;
    storage->velocity_buffer = 0;
    storage->force_buffer = 0;
    storage->density_buffer = 0;
    storage->pressure_buffer = 0;
    
    // Clean up compute shaders
    storage->density_compute_shader = 0;
    storage->force_compute_shader = 0;
    storage->integration_compute_shader = 0;
    
    // Clean up spatial grid
    fluid_spatial_grid_shutdown(&storage->spatial_grid);
    
    memset(storage, 0, sizeof(GPUFluidStorage));
    LOG_INFO("GPU fluid storage shutdown");
}

void fluid_gpu_storage_upload_particles(GPUFluidStorage* storage, FluidParticle* particles, u32 count) {
    if (!storage || !particles || count == 0) return;
    
    storage->particle_count = count;
    
    // Upload particle data to GPU buffers (simplified)
    // In real implementation would use glBufferSubData or similar
    LOG_TRACE("Uploaded %u fluid particles to GPU", count);
}

// TASK_1810: Implement Neighbor Search (Grid-based or Spatial Hashing)
void fluid_spatial_grid_init(FluidSpatialGrid* grid, f32 cell_size, u32 grid_size) {
    if (!grid) return;
    
    memset(grid, 0, sizeof(FluidSpatialGrid));
    grid->cell_size = cell_size;
    grid->grid_size = grid_size;
    grid->total_cells = grid_size * grid_size * grid_size;
    
    // Allocate grid cells
    grid->cells = malloc(grid->total_cells * sizeof(FluidGridCell));
    if (!grid->cells) {
        LOG_ERROR("Failed to allocate spatial grid cells");
        return;
    }
    
    // Initialize all cells
    memset(grid->cells, 0, grid->total_cells * sizeof(FluidGridCell));
    
    LOG_INFO("Spatial grid initialized: %ux%ux%u cells, %.3f cell size", 
             grid_size, grid_size, grid_size, cell_size);
}

void fluid_spatial_grid_shutdown(FluidSpatialGrid* grid) {
    if (!grid) return;
    
    if (grid->cells) {
        free(grid->cells);
        grid->cells = NULL;
    }
    
    memset(grid, 0, sizeof(FluidSpatialGrid));
    LOG_INFO("Spatial grid shutdown");
}

static inline u32 fluid_spatial_hash_function(FluidSpatialGrid* grid, i32 x, i32 y, i32 z) {
    // Simple hash function for spatial hashing
    const u32 p1 = 73856093;
    const u32 p2 = 19349663;
    const u32 p3 = 83492791;
    
    return ((u32)x * p1) ^ ((u32)y * p2) ^ ((u32)z * p3) % grid->total_cells;
}

void fluid_spatial_grid_update(FluidSpatialGrid* grid, FluidParticle* particles, u32 count) {
    if (!grid || !particles) return;
    
    // Clear all cells
    for (u32 i = 0; i < grid->total_cells; i++) {
        grid->cells[i].particle_count = 0;
    }
    
    // Insert particles into grid
    for (u32 i = 0; i < count; i++) {
        FluidParticle* particle = &particles[i];
        
        // Convert world position to grid coordinates
        i32 grid_x = (i32)floorf(particle->position.x / grid->cell_size);
        i32 grid_y = (i32)floorf(particle->position.y / grid->cell_size);
        i32 grid_z = (i32)floorf(particle->position.z / grid->cell_size);
        
        // Hash to get cell index
        u32 cell_index = fluid_spatial_hash_function(grid, grid_x, grid_y, grid_z);
        
        // Add particle to cell (if there's space)
        FluidGridCell* cell = &grid->cells[cell_index];
        if (cell->particle_count < 64) {  // Max particles per cell
            cell->particle_indices[cell->particle_count] = i;
            cell->particle_count++;
        }
    }
}

void fluid_spatial_grid_find_neighbors(FluidSpatialGrid* grid, Vec3 position, f32 radius, 
                                      u32* neighbor_indices, u32* neighbor_count) {
    if (!grid || !neighbor_indices || !neighbor_count) return;
    
    *neighbor_count = 0;
    
    // Convert world position to grid coordinates
    i32 center_x = (i32)floorf(position.x / grid->cell_size);
    i32 center_y = (i32)floorf(position.y / grid->cell_size);
    i32 center_z = (i32)floorf(position.z / grid->cell_size);
    
    // Calculate search radius in grid cells
    i32 cell_radius = (i32)ceilf(radius / grid->cell_size);
    
    // Search neighboring cells
    for (i32 dx = -cell_radius; dx <= cell_radius; dx++) {
        for (i32 dy = -cell_radius; dy <= cell_radius; dy++) {
            for (i32 dz = -cell_radius; dz <= cell_radius; dz++) {
                i32 grid_x = center_x + dx;
                i32 grid_y = center_y + dy;
                i32 grid_z = center_z + dz;
                
                // Get cell index
                u32 cell_index = fluid_spatial_hash_function(grid, grid_x, grid_y, grid_z);
                FluidGridCell* cell = &grid->cells[cell_index];
                
                // Check all particles in this cell
                for (u32 i = 0; i < cell->particle_count && *neighbor_count < 64; i++) {
                    u32 particle_idx = cell->particle_indices[i];
                    
                    // Skip if already added (avoid duplicates)
                    bool already_added = false;
                    for (u32 j = 0; j < *neighbor_count; j++) {
                        if (neighbor_indices[j] == particle_idx) {
                            already_added = true;
                            break;
                        }
                    }
                    
                    if (!already_added) {
                        neighbor_indices[*neighbor_count] = particle_idx;
                        (*neighbor_count)++;
                    }
                }
            }
        }
    }
}

// TASK_1811: Compute Density and Pressure per particle
void fluid_compute_density_pressure(FluidSimulationSystem* system) {
    if (!system) return;
    
    // Update spatial grid
    fluid_spatial_grid_update(&system->spatial_grid, system->particles, system->particle_count);
    
    // Compute density for each particle
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        f32 density = 0.0f;
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute density using SPH interpolation
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pi->position, pj->position);
            f32 r = vec3_length(diff);
            
            if (r <= system->smoothing_radius) {
                f32 kernel = fluid_sph_poly6_kernel(r, system->smoothing_radius);
                density += pj->mass * kernel;
            }
        }
        
        pi->density = density;
        
        // Compute pressure using ideal gas law
        pi->pressure = system->gas_constant * (pi->density - system->rest_density);
    }
}

void fluid_compute_density_gpu(FluidSimulationSystem* system) {
    if (!system || !system->use_gpu_simulation) return;
    
    // GPU implementation would use compute shaders
    // For now, fall back to CPU implementation
    fluid_compute_density_pressure(system);
    
    LOG_TRACE("GPU density computation (fallback to CPU)");
}

// TASK_1812: Compute Pressure Forces and Viscosity
void fluid_compute_pressure_forces(FluidSimulationSystem* system) {
    if (!system) return;
    
    // Reset forces
    for (u32 i = 0; i < system->particle_count; i++) {
        system->particles[i].force = vec3_zero();
    }
    
    // Compute pressure forces for each particle
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        Vec3 pressure_force = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute pressure force contribution from neighbors
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pi->position, pj->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                // Pressure force using SPH formulation
                f32 pressure_term = (pi->pressure + pj->pressure) / (2.0f * pi->density * pj->density);
                f32 gradient = fluid_sph_spiky_gradient(r, system->smoothing_radius);
                Vec3 force = vec3_scale(diff, -pj->mass * pressure_term * gradient);
                pressure_force = vec3_add(pressure_force, force);
            }
        }
        
        // Apply pressure force
        pi->force = vec3_add(pi->force, pressure_force);
    }
}

void fluid_compute_viscosity_forces(FluidSimulationSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        Vec3 viscosity_force = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute viscosity force contribution from neighbors
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pj->position, pi->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                // Viscosity force using SPH formulation
                Vec3 velocity_diff = vec3_sub(pj->velocity, pi->velocity);
                f32 laplacian = fluid_sph_viscosity_laplacian(r, system->smoothing_radius);
                Vec3 force = vec3_scale(velocity_diff, system->viscosity * pj->mass * laplacian / pj->density);
                viscosity_force = vec3_add(viscosity_force, force);
            }
        }
        
        // Apply viscosity force
        pi->force = vec3_add(pi->force, viscosity_force);
    }
}

// TASK_1813: Implement Surface Tension modeling
void fluid_compute_surface_normals(FluidSimulationSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        Vec3 normal = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute surface normal using color field gradient
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pi->position, pj->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                f32 gradient = fluid_sph_spiky_gradient(r, system->smoothing_radius);
                Vec3 contribution = vec3_scale(diff, pj->mass * gradient / pj->density);
                normal = vec3_add(normal, contribution);
            }
        }
        
        // Store normal (could be used for rendering)
        // For now, we don't store it in the particle structure
    }
}

void fluid_compute_surface_curvature(FluidSimulationSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        f32 curvature = 0.0f;
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute surface curvature using color field Laplacian
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pi->position, pj->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                f32 laplacian = fluid_sph_viscosity_laplacian(r, system->smoothing_radius);
                curvature += pj->mass * laplacian / pj->density;
            }
        }
        
        // Store curvature (could be used for surface tension)
    }
}

void fluid_apply_cohesion_forces(FluidSimulationSystem* system) {
    if (!system) return;
    
    const f32 cohesion_strength = 0.001f;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* pi = &system->particles[i];
        Vec3 cohesion_force = vec3_zero();
        
        // Find neighbors
        u32 neighbor_indices[64];
        u32 neighbor_count = 0;
        fluid_spatial_grid_find_neighbors(&system->spatial_grid, pi->position, 
                                         system->smoothing_radius, 
                                         neighbor_indices, &neighbor_count);
        
        // Compute cohesion force
        for (u32 j = 0; j < neighbor_count; j++) {
            u32 neighbor_idx = neighbor_indices[j];
            FluidParticle* pj = &system->particles[neighbor_idx];
            
            Vec3 diff = vec3_sub(pj->position, pi->position);
            f32 r = vec3_length(diff);
            
            if (r > 0.0001f && r < system->smoothing_radius) {
                // Cohesion force pulls particles together
                f32 kernel = fluid_sph_poly6_kernel(r, system->smoothing_radius);
                Vec3 force = vec3_scale(diff, cohesion_strength * kernel);
                cohesion_force = vec3_add(cohesion_force, force);
            }
        }
        
        // Apply cohesion force
        pi->force = vec3_add(pi->force, cohesion_force);
    }
}

// TASK_1814: Add "External Forces" (Gravity, Wind, Stirring)
void fluid_apply_gravity(FluidSimulationSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* particle = &system->particles[i];
        Vec3 gravity_force = vec3_scale(system->gravity, particle->mass);
        particle->force = vec3_add(particle->force, gravity_force);
    }
}

void fluid_apply_wind_force(FluidSimulationSystem* system, Vec3 wind_velocity) {
    if (!system) return;
    
    const f32 drag_coefficient = 0.47f;  // Sphere drag coefficient
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* particle = &system->particles[i];
        
        // Compute relative velocity
        Vec3 relative_velocity = vec3_sub(wind_velocity, particle->velocity);
        f32 speed = vec3_length(relative_velocity);
        
        if (speed > 0.0001f) {
            // Drag force: F = 0.5 * ρ * v² * C_d * A
            f32 cross_section_area = 0.01f;  // Approximate particle cross-section
            f32 drag_magnitude = 0.5f * 1.2f * speed * speed * drag_coefficient * cross_section_area;
            Vec3 drag_force = vec3_scale(vec3_normalize(relative_velocity), drag_magnitude);
            particle->force = vec3_add(particle->force, drag_force);
        }
    }
}

void fluid_apply_stirring_force(FluidSimulationSystem* system, Vec3 center, f32 radius, f32 strength) {
    if (!system) return;
    
    for (u32 i = 0; i < system->particle_count; i++) {
        FluidParticle* particle = &system->particles[i];
        
        // Distance from stirring center
        Vec3 to_center = vec3_sub(center, particle->position);
        f32 distance = vec3_length(to_center);
        
        if (distance < radius && distance > 0.0001f) {
            // Tangential force for stirring effect
            Vec3 radial = vec3_normalize(to_center);
            Vec3 tangent = vec3_cross(radial, vec3_up());  // Stir around Y axis
            if (vec3_length_squared(tangent) < 0.0001f) {
                tangent = vec3_cross(radial, vec3_forward());
            }
            
            f32 falloff = 1.0f - (distance / radius);
            Vec3 stir_force = vec3_scale(tangent, strength * falloff);
            particle->force = vec3_add(particle->force, stir_force);
        }
    }
}

// Core fluid simulation system functions
void fluid_simulation_init(FluidSimulationSystem* system) {
    if (!system) return;
    
    memset(system, 0, sizeof(FluidSimulationSystem));
    
    // Set default parameters
    system->smoothing_radius = FLUID_SMOOTHING_RADIUS;
    system->rest_density = FLUID_REST_DENSITY;
    system->gas_constant = FLUID_GAS_CONSTANT;
    system->viscosity = FLUID_VISCOSITY;
    system->surface_tension = FLUID_SURFACE_TENSION;
    system->gravity = vec3(0.0f, FLUID_GRAVITY, 0.0f);
    system->time_step = FLUID_TIME_STEP;
    system->max_particles = FLUID_MAX_PARTICLES;
    
    // Initialize spatial grid
    fluid_spatial_grid_init(&system->spatial_grid, system->smoothing_radius, FLUID_GRID_SIZE);
    
    // Initialize GPU storage
    system->use_gpu_simulation = false;  // Start with CPU simulation
    fluid_gpu_storage_init(&system->gpu_storage, system->max_particles);
    
    // Allocate particle array
    system->particles = malloc(system->max_particles * sizeof(FluidParticle));
    if (!system->particles) {
        LOG_ERROR("Failed to allocate fluid particle array");
        fluid_simulation_shutdown(system);
        return;
    }
    
    memset(system->particles, 0, system->max_particles * sizeof(FluidParticle));
    
    LOG_INFO("Fluid simulation system initialized");
    LOG_INFO("  Max particles: %u", system->max_particles);
    LOG_INFO("  Smoothing radius: %.3f", system->smoothing_radius);
    LOG_INFO("  Rest density: %.1f", system->rest_density);
}

void fluid_simulation_shutdown(FluidSimulationSystem* system) {
    if (!system) return;
    
    // Clean up particles
    if (system->particles) {
        free(system->particles);
        system->particles = NULL;
    }
    
    // Clean up spatial grid
    fluid_spatial_grid_shutdown(&system->spatial_grid);
    
    // Clean up GPU storage
    fluid_gpu_storage_shutdown(&system->gpu_storage);
    
    memset(system, 0, sizeof(FluidSimulationSystem));
    LOG_INFO("Fluid simulation system shutdown");
}

void fluid_simulation_update(FluidSimulationSystem* system, f32 delta_time) {
    if (!system || system->particle_count == 0) return;
    
    u64 start_time = time_get_current_ms();
    
    // Fluid simulation pipeline
    fluid_pbf_predict_positions(system, delta_time);
    
    fluid_compute_density_pressure(system);
    fluid_compute_pressure_forces(system);
    fluid_compute_viscosity_forces(system);
    fluid_apply_cohesion_forces(system);
    
    fluid_apply_gravity(system);
    
    fluid_pbf_solve_density_constraints(system);
    fluid_pbf_update_velocities(system, delta_time);
    
    fluid_pbf_apply_vorticity_confinement(system);
    fluid_pbf_apply_viscosity_xspf(system);
    
    // Update GPU storage if enabled
    if (system->use_gpu_simulation) {
        fluid_gpu_storage_upload_particles(&system->gpu_storage, system->particles, system->particle_count);
    }
    
    u64 end_time = time_get_current_ms();
    system->simulation_time_ms += (end_time - start_time);
}

u32 fluid_simulation_add_particle(FluidSimulationSystem* system, Vec3 position, Vec3 velocity) {
    if (!system || system->particle_count >= system->max_particles) return 0;
    
    u32 id = system->particle_count + 1;
    FluidParticle* particle = &system->particles[system->particle_count];
    
    particle->position = position;
    particle->velocity = velocity;
    particle->force = vec3_zero();
    particle->density = system->rest_density;
    particle->pressure = 0.0f;
    particle->mass = 0.02f;  // 20g per particle
    particle->id = id;
    
    system->particle_count++;
    return id;
}

void fluid_simulation_remove_particle(FluidSimulationSystem* system, u32 particle_id) {
    if (!system || particle_id == 0 || system->particle_count == 0) return;
    
    // Find particle by ID
    for (u32 i = 0; i < system->particle_count; i++) {
        if (system->particles[i].id == particle_id) {
            // Move last particle to this position
            if (i < system->particle_count - 1) {
                system->particles[i] = system->particles[system->particle_count - 1];
            }
            system->particle_count--;
            return;
        }
    }
}

void fluid_simulation_clear_particles(FluidSimulationSystem* system) {
    if (!system) return;
    system->particle_count = 0;
}
