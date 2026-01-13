/*
 * particle_simulation.h
 * GPU-accelerated particle simulation system
 * Uses engine's Vec3 type for consistency
 */

#ifndef PARTICLE_SIMULATION_H
#define PARTICLE_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include "particle_types.h"
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   GPU Particle Simulation System
   ============================================================================ */

typedef struct gpu_particle_system {
    particle_t* particles;           // CPU particle buffer
    particle_t* gpu_particles;       // GPU particle buffer
    uint32_t max_particles;
    uint32_t active_particles;
    bool use_gpu_simulation;
    
    // GPU resources
    void* compute_shader;            // Compute shader for GPU simulation
    void* uniform_buffer;            // Uniform buffer for simulation parameters
    void* particle_buffer;           // GPU buffer for particle data
} gpu_particle_system_t;

/* ============================================================================
   Simulation Parameters
   ============================================================================ */

typedef struct simulation_params {
    Vec3 gravity;                    // Gravity vector
    Vec3 wind_velocity;              // Wind velocity vector
    float delta_time;                // Time step
    float drag_coefficient;          // Air resistance
    float bounce_damping;            // Energy loss on collision
    uint32_t collision_enabled;      // Enable/disable collisions
} simulation_params_t;

/* ============================================================================
   CPU Simulation Functions
   ============================================================================ */

// Initialize particle simulation system
gpu_particle_system_t* particle_simulation_create(uint32_t max_particles);

// Destroy particle simulation system
void particle_simulation_destroy(gpu_particle_system_t* system);

// Update particles on CPU
void particle_simulation_update_cpu(
    gpu_particle_system_t* system,
    const simulation_params_t* params
);

// Update particles on GPU
void particle_simulation_update_gpu(
    gpu_particle_system_t* system,
    const simulation_params_t* params
);

// Add particle to system
bool particle_simulation_add_particle(
    gpu_particle_system_t* system,
    const particle_t* particle
);

// Add multiple particles (burst emission)
uint32_t particle_simulation_add_particles(
    gpu_particle_system_t* system,
    const particle_t* particles,
    uint32_t count
);

/* ============================================================================
   GPU Simulation Functions
   ============================================================================ */

// Initialize GPU resources
bool particle_simulation_init_gpu(gpu_particle_system_t* system);

// Release GPU resources
void particle_simulation_release_gpu(gpu_particle_system_t* system);

// Update particles using GPU compute shader
void particle_simulation_gpu_update(
    gpu_particle_system_t* system,
    const simulation_params_t* params
);

// Copy particles from GPU to CPU
void particle_simulation_gpu_to_cpu(gpu_particle_system_t* system);

// Copy particles from CPU to GPU
void particle_simulation_cpu_to_gpu(gpu_particle_system_t* system);

/* ============================================================================
   Physics Functions (using Vec3)
   ============================================================================ */

// Apply gravity to particle
void particle_apply_gravity(
    particle_t* particle,
    Vec3 gravity,
    float delta_time
);

// Apply wind force to particle
void particle_apply_wind(
    particle_t* particle,
    Vec3 wind_velocity,
    float drag_coefficient,
    float delta_time
);

// Handle collision with ground plane
bool particle_ground_collision(
    particle_t* particle,
    float ground_height,
    float bounce_damping
);

// Handle collision with sphere
bool particle_sphere_collision(
    particle_t* particle,
    Vec3 sphere_center,
    float sphere_radius,
    float bounce_damping
);

// Handle collision with box
bool particle_box_collision(
    particle_t* particle,
    Vec3 box_min,
    Vec3 box_max,
    float bounce_damping
);

/* ============================================================================
   Utility Functions
   ============================================================================ */

// Get simulation statistics
particle_stats_t particle_simulation_get_stats(
    const gpu_particle_system_t* system
);

// Reset simulation
void particle_simulation_reset(gpu_particle_system_t* system);

// Enable/disable GPU simulation
void particle_simulation_set_gpu_mode(
    gpu_particle_system_t* system,
    bool use_gpu
);

// Check if GPU simulation is available
bool particle_simulation_gpu_available(void);

#ifdef __cplusplus
}
#endif

#endif /* PARTICLE_SIMULATION_H */