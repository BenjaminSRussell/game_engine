/*
 * particle_simulation.c
 * GPU-accelerated particle simulation system implementation
 * Uses engine's Vec3 type for consistency
 */

#include "effects/particles/particle_simulation.h"
#include "effects/particles/particle_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h> // For FLT_MAX

// Use core logger
#include "engine/include/core/logger.h"

// Macros to replace printf logging with core logging
#define log_error(fmt, ...) LOG_ERROR(fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) LOG_WARN(fmt, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   Internal Helper Functions
   ============================================================================ */

static void update_particle_physics(
    particle_t* particle,
    const simulation_params_t* params
) {
    if (!particle || !(particle->flags & PARTICLE_FLAG_ALIVE)) {
        return;
    }

    // Apply gravity
    particle_apply_gravity(particle, params->gravity, params->delta_time);
    
    // Apply wind force
    particle_apply_wind(particle, params->wind_velocity, 
                       params->drag_coefficient, params->delta_time);
    
    // Update position based on velocity
    // vec3_mul (scalar mult) from vec3.h
    Vec3 velocity_step = vec3_mul(particle->velocity, params->delta_time);
    Vec3 new_position = vec3_add(particle->position, velocity_step);
    particle->position = new_position;
    
    // Update age
    particle->age += params->delta_time;
    
    // Check if particle should die
    if (particle->age >= particle->lifetime) {
        particle->flags &= ~PARTICLE_FLAG_ALIVE;
        particle->flags |= PARTICLE_FLAG_DEAD;
    }
}

static void handle_particle_collisions(
    particle_t* particle,
    const simulation_params_t* params
) {
    if (!particle || !(particle->flags & PARTICLE_FLAG_ALIVE) || 
        !params->collision_enabled) {
        return;
    }
    
    // Simple ground collision (y = 0 plane)
    if (particle->position.y < 0.0f) {
        particle->position.y = 0.0f;
        particle->velocity.y = -particle->velocity.y * params->bounce_damping;
        particle->velocity.x *= 0.9f; // Friction
        particle->velocity.z *= 0.9f;
    }
}

/* ============================================================================
   System Management Functions
   ============================================================================ */

gpu_particle_system_t* particle_simulation_create(uint32_t max_particles) {
    if (max_particles == 0) {
        log_error("Cannot create particle system with zero particles");
        return NULL;
    }
    
    gpu_particle_system_t* system = (gpu_particle_system_t*)malloc(
        sizeof(gpu_particle_system_t)
    );
    if (!system) {
        log_error("Failed to allocate memory for particle system");
        return NULL;
    }
    
    // Allocate CPU particle buffer
    system->particles = (particle_t*)calloc(max_particles, sizeof(particle_t));
    if (!system->particles) {
        log_error("Failed to allocate memory for particle buffer");
        free(system);
        return NULL;
    }
    
    system->max_particles = max_particles;
    system->active_particles = 0;
    system->use_gpu_simulation = false;
    system->gpu_particles = NULL;
    system->compute_shader = NULL;
    system->uniform_buffer = NULL;
    system->particle_buffer = NULL;
    
    log_info("Created particle system with %u max particles", max_particles);
    return system;
}

void particle_simulation_destroy(gpu_particle_system_t* system) {
    if (!system) {
        return;
    }
    
    // Release GPU resources if initialized
    if (system->use_gpu_simulation) {
        particle_simulation_release_gpu(system);
    }
    
    // Free CPU resources
    if (system->particles) {
        free(system->particles);
    }
    
    free(system);
    log_info("Destroyed particle system");
}

/* ============================================================================
   CPU Simulation Functions
   ============================================================================ */

void particle_simulation_update_cpu(
    gpu_particle_system_t* system,
    const simulation_params_t* params
) {
    if (!system || !params || !system->particles) {
        return;
    }
    
    uint32_t active_count = 0;
    
    for (uint32_t i = 0; i < system->max_particles; i++) {
        particle_t* particle = &system->particles[i];
        
        if (particle->flags & PARTICLE_FLAG_ALIVE) {
            // Update physics
            update_particle_physics(particle, params);
            
            // Handle collisions
            handle_particle_collisions(particle, params);
            
            // Count active particles
            if (particle->flags & PARTICLE_FLAG_ALIVE) {
                active_count++;
            }
        }
    }
    
    system->active_particles = active_count;
}

bool particle_simulation_add_particle(
    gpu_particle_system_t* system,
    const particle_t* particle
) {
    if (!system || !particle || !system->particles) {
        return false;
    }
    
    // Find first dead particle slot
    for (uint32_t i = 0; i < system->max_particles; i++) {
        particle_t* slot = &system->particles[i];
        
        if (!(slot->flags & PARTICLE_FLAG_ALIVE)) {
            // Copy particle data
            *slot = *particle;
            slot->flags |= PARTICLE_FLAG_ALIVE;
            slot->flags &= ~PARTICLE_FLAG_DEAD;
            
            system->active_particles++;
            return true;
        }
    }
    
    return false; // No available slots
}

uint32_t particle_simulation_add_particles(
    gpu_particle_system_t* system,
    const particle_t* particles,
    uint32_t count
) {
    if (!system || !particles || count == 0) {
        return 0;
    }
    
    uint32_t added = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        if (particle_simulation_add_particle(system, &particles[i])) {
            added++;
        } else {
            break; // No more slots available
        }
    }
    
    return added;
}

/* ============================================================================
   GPU Simulation Functions
   ============================================================================ */

bool particle_simulation_init_gpu(gpu_particle_system_t* system) {
    if (!system) {
        return false;
    }
    
    // Allocate GPU particle buffer
    system->gpu_particles = (particle_t*)calloc(system->max_particles, sizeof(particle_t));
    if (!system->gpu_particles) {
        log_error("Failed to allocate GPU particle buffer");
        return false;
    }
    
    // TODO: Initialize actual GPU resources (compute shader, uniform buffer, etc.)
    // This would depend on the specific graphics API being used
    
    log_info("Initialized GPU particle simulation resources");
    return true;
}

void particle_simulation_release_gpu(gpu_particle_system_t* system) {
    if (!system) {
        return;
    }
    
    if (system->gpu_particles) {
        free(system->gpu_particles);
        system->gpu_particles = NULL;
    }
    
    // TODO: Release actual GPU resources
    
    log_info("Released GPU particle simulation resources");
}

void particle_simulation_update_gpu(
    gpu_particle_system_t* system,
    const simulation_params_t* params
) {
    if (!system || !params || !system->use_gpu_simulation) {
        return;
    }
    
    // TODO: Implement actual GPU compute shader dispatch
    // For now, fall back to CPU simulation
    particle_simulation_update_cpu(system, params);
}

void particle_simulation_gpu_to_cpu(gpu_particle_system_t* system) {
    if (!system || !system->gpu_particles || !system->particles) {
        return;
    }
    
    // Copy data from GPU buffer to CPU buffer
    memcpy(system->particles, system->gpu_particles, 
           system->max_particles * sizeof(particle_t));
}

void particle_simulation_cpu_to_gpu(gpu_particle_system_t* system) {
    if (!system || !system->gpu_particles || !system->particles) {
        return;
    }
    
    // Copy data from CPU buffer to GPU buffer
    memcpy(system->gpu_particles, system->particles, 
           system->max_particles * sizeof(particle_t));
}

/* ============================================================================
   Physics Functions (using Vec3)
   ============================================================================ */

void particle_apply_gravity(
    particle_t* particle,
    Vec3 gravity,
    float delta_time
) {
    if (!particle) {
        return;
    }
    
    // Apply gravity acceleration to velocity
    // vec3_mul (scalar mult) from vec3.h
    Vec3 gravity_impulse = vec3_mul(gravity, delta_time);
    particle->velocity = vec3_add(particle->velocity, gravity_impulse);
}

void particle_apply_wind(
    particle_t* particle,
    Vec3 wind_velocity,
    float drag_coefficient,
    float delta_time
) {
    if (!particle) {
        return;
    }
    
    // Calculate velocity difference
    Vec3 velocity_diff = vec3_sub(wind_velocity, particle->velocity);
    
    // Apply wind force (simplified drag model)
    // vec3_mul (scalar mult) from vec3.h
    Vec3 wind_force = vec3_mul(velocity_diff, drag_coefficient * delta_time);
    particle->velocity = vec3_add(particle->velocity, wind_force);
}

bool particle_ground_collision(
    particle_t* particle,
    float ground_height,
    float bounce_damping
) {
    if (!particle || particle->position.y > ground_height) {
        return false;
    }
    
    // Position correction
    particle->position.y = ground_height;
    
    // Velocity reflection with damping
    particle->velocity.y = -particle->velocity.y * bounce_damping;
    
    // Apply friction
    particle->velocity.x *= 0.9f;
    particle->velocity.z *= 0.9f;
    
    return true;
}

bool particle_sphere_collision(
    particle_t* particle,
    Vec3 sphere_center,
    float sphere_radius,
    float bounce_damping
) {
    if (!particle) {
        return false;
    }
    
    // Calculate distance to sphere center
    Vec3 to_center = vec3_sub(particle->position, sphere_center);
    float distance = vec3_length(to_center);
    
    if (distance > sphere_radius) {
        return false;
    }
    
    // Normalize direction vector
    Vec3 normal = vec3_normalize(to_center);
    
    // Position correction (push particle outside sphere)
    float penetration = sphere_radius - distance;
    Vec3 offset = vec3_mul(normal, penetration + 0.001f);
    particle->position = vec3_add(particle->position, offset);
    
    // Velocity reflection
    float velocity_dot_normal = vec3_dot(particle->velocity, normal);
    if (velocity_dot_normal < 0.0f) {
        Vec3 reflection = vec3_mul(normal, 2.0f * velocity_dot_normal);
        particle->velocity = vec3_sub(particle->velocity, reflection);
        particle->velocity = vec3_mul(particle->velocity, bounce_damping);
    }
    
    return true;
}

bool particle_box_collision(
    particle_t* particle,
    Vec3 box_min,
    Vec3 box_max,
    float bounce_damping
) {
    if (!particle) {
        return false;
    }
    
    // Check if particle is inside box
    if (particle->position.x < box_min.x || particle->position.x > box_max.x ||
        particle->position.y < box_min.y || particle->position.y > box_max.y ||
        particle->position.z < box_min.z || particle->position.z > box_max.z) {
        return false;
    }
    
    // Find closest face and normal
    Vec3 normal = vec3_zero();
    float min_distance = FLT_MAX;
    
    // Check each face
    float distances[6] = {
        particle->position.x - box_min.x,  // -X face
        box_max.x - particle->position.x,  // +X face
        particle->position.y - box_min.y,  // -Y face
        box_max.y - particle->position.y,  // +Y face
        particle->position.z - box_min.z,  // -Z face
        box_max.z - particle->position.z   // +Z face
    };
    
    // Find minimum distance face
    for (int i = 0; i < 6; i++) {
        if (distances[i] < min_distance) {
            min_distance = distances[i];
            switch (i) {
                case 0: normal = vec3(-1, 0, 0); break;
                case 1: normal = vec3(1, 0, 0); break;
                case 2: normal = vec3(0, -1, 0); break;
                case 3: normal = vec3(0, 1, 0); break;
                case 4: normal = vec3(0, 0, -1); break;
                case 5: normal = vec3(0, 0, 1); break;
            }
        }
    }
    
    // Position correction
    Vec3 offset = vec3_mul(normal, min_distance + 0.001f);
    particle->position = vec3_add(particle->position, offset);
    
    // Velocity reflection
    float velocity_dot_normal = vec3_dot(particle->velocity, normal);
    if (velocity_dot_normal < 0.0f) {
        Vec3 reflection = vec3_mul(normal, 2.0f * velocity_dot_normal);
        particle->velocity = vec3_sub(particle->velocity, reflection);
        particle->velocity = vec3_mul(particle->velocity, bounce_damping);
    }
    
    return true;
}

/* ============================================================================
   Utility Functions
   ============================================================================ */

particle_stats_t particle_simulation_get_stats(
    const gpu_particle_system_t* system
) {
    particle_stats_t stats = {0};
    
    if (!system) {
        return stats;
    }
    
    stats.active_particles = system->active_particles;
    stats.max_particles = system->max_particles;
    stats.using_gpu = system->use_gpu_simulation;
    
    // Count particles spawned and killed
    for (uint32_t i = 0; i < system->max_particles; i++) {
        const particle_t* particle = &system->particles[i];
        
        if (particle->flags & PARTICLE_FLAG_ALIVE) {
            stats.particles_spawned++;
        } else if (particle->flags & PARTICLE_FLAG_DEAD) {
            stats.particles_killed++;
        }
    }
    
    return stats;
}

void particle_simulation_reset(gpu_particle_system_t* system) {
    if (!system || !system->particles) {
        return;
    }
    
    // Reset all particles
    for (uint32_t i = 0; i < system->max_particles; i++) {
        particle_t* particle = &system->particles[i];
        particle->flags = PARTICLE_FLAG_DEAD;
        particle->age = 0.0f;
        particle->position = vec3_zero();
        particle->velocity = vec3_zero();
        particle->acceleration = vec3_zero();
    }
    
    system->active_particles = 0;
    log_info("Reset particle simulation");
}

void particle_simulation_set_gpu_mode(
    gpu_particle_system_t* system,
    bool use_gpu
) {
    if (!system) {
        return;
    }
    
    if (use_gpu && !system->use_gpu_simulation) {
        // Enable GPU mode
        if (particle_simulation_gpu_available() && particle_simulation_init_gpu(system)) {
            system->use_gpu_simulation = true;
            log_info("Enabled GPU particle simulation");
        } else {
            log_warn("Cannot enable GPU simulation, falling back to CPU");
            system->use_gpu_simulation = false;
        }
    } else if (!use_gpu && system->use_gpu_simulation) {
        // Disable GPU mode
        particle_simulation_release_gpu(system);
        system->use_gpu_simulation = false;
        log_info("Disabled GPU particle simulation");
    }
}

bool particle_simulation_gpu_available(void) {
    // TODO: Check if GPU compute capabilities are available
    // For now, return false to always use CPU simulation
    return false;
}

#ifdef __cplusplus
}
#endif
