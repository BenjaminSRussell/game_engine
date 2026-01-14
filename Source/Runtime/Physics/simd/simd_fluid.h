#pragma once

#include "simd_physics.h"
#include "../math/types.h"

// SIMD-Optimized Fluid Simulation
// High-performance SPH fluid simulation using vectorized operations

// Fluid Particle with SIMD support
typedef struct {
    simd_vec3 position;
    simd_vec3 velocity;
    simd_vec3 acceleration;
    simd_vec3 force;
    __m256 density;
    __m256 pressure;
    __m256 mass;
    u32 id;
} simd_fluid_particle;

// SPH Kernel Parameters
typedef struct {
    __m256 h;           // Smoothing kernel radius
    __m256 h2;          // h squared
    __m256 h6;          // h to the 6th power
    __m256 h9;          // h to the 9th power
    __m256 poly6_const; // Poly6 kernel constant
    __m256 spiky_const; // Spiky kernel constant
    __m256 visc_const;  // Viscosity kernel constant
} simd_sph_kernel;

// SIMD Fluid System
typedef struct {
    simd_fluid_particle* particles;
    u32 particle_count;
    simd_sph_kernel kernel;
    __m256 rest_density;
    __m256 gas_constant;
    __m256 viscosity;
    __m256 damping;
    __m256 gravity;
    vec3 bounds_min;
    vec3 bounds_max;
} simd_fluid_system;

// SIMD Fluid Operations
void simd_fluid_create(simd_fluid_system* fluid, u32 particle_count, const vec3* bounds_min, const vec3* bounds_max);
void simd_fluid_destroy(simd_fluid_system* fluid);
void simd_fluid_update(simd_fluid_system* fluid, f32 dt);
void simd_fluid_compute_density_pressure(simd_fluid_system* fluid);
void simd_fluid_compute_forces(simd_fluid_system* fluid);
void simd_fluid_integrate(simd_fluid_system* fluid, f32 dt);
void simd_fluid_handle_boundaries(simd_fluid_system* fluid);

// SIMD SPH Kernels
__m256 simd_sph_poly6_kernel(__m256 r_sq, const simd_sph_kernel* kernel);
__m256 simd_sph_spiky_gradient(__m256 r, const simd_sph_kernel* kernel);
__m256 simd_sph_viscosity_laplacian(__m256 r, const simd_sph_kernel* kernel);

// SIMD Boundary Handling
void simd_fluid_box_collision(simd_fluid_system* fluid);

#endif // SIMD_FLUID_H
