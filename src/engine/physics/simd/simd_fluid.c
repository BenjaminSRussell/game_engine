#include "simd_fluid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void simd_fluid_create(simd_fluid_system* fluid, u32 particle_count, const vec3* bounds_min, const vec3* bounds_max) {
    fluid->particle_count = particle_count;
    fluid->bounds_min = *bounds_min;
    fluid->bounds_max = *bounds_max;
    
    // Allocate particles
    fluid->particles = malloc(particle_count * sizeof(simd_fluid_particle));
    memset(fluid->particles, 0, particle_count * sizeof(simd_fluid_particle));
    
    // Initialize SPH kernel parameters
    fluid->kernel.h = _mm256_set1_ps(0.5f); // 0.5m smoothing radius
    fluid->kernel.h2 = _mm256_mul_ps(fluid->kernel.h, fluid->kernel.h);
    fluid->kernel.h6 = _mm256_mul_ps(_mm256_mul_ps(fluid->kernel.h2, fluid->kernel.h2), fluid->kernel.h2);
    fluid->kernel.h9 = _mm256_mul_ps(fluid->kernel.h6, fluid->kernel.h2);
    
    // Poly6 kernel constant: 315 / (64 * pi * h^9)
    fluid->kernel.poly6_const = _mm256_set1_ps(315.0f / (64.0f * 3.14159f));
    fluid->kernel.poly6_const = _mm256_div_ps(fluid->kernel.poly6_const, fluid->kernel.h9);
    
    // Spiky kernel constant: -45 / (pi * h^6)
    fluid->kernel.spiky_const = _mm256_set1_ps(-45.0f / 3.14159f);
    fluid->kernel.spiky_const = _mm256_div_ps(fluid->kernel.spiky_const, fluid->kernel.h6);
    
    // Viscosity kernel constant: 45 / (pi * h^6)
    fluid->kernel.visc_const = _mm256_set1_ps(45.0f / 3.14159f);
    fluid->kernel.visc_const = _mm256_div_ps(fluid->kernel.visc_const, fluid->kernel.h6);
    
    // Initialize fluid parameters
    fluid->rest_density = _mm256_set1_ps(1000.0f); // Water density
    fluid->gas_constant = _mm256_set1_ps(2000.0f);  // Gas stiffness
    fluid->viscosity = _mm256_set1_ps(0.01f);      // Dynamic viscosity
    fluid->damping = _mm256_set1_ps(0.998f);       // Velocity damping
    fluid->gravity = _mm256_set1_ps(-9.81f);       // Gravity
    
    // Initialize particles in a grid
    f32 spacing = 0.1f;
    u32 particles_per_dim = (u32)cbrtf(particle_count);
    
    for (u32 i = 0; i < particle_count; i += 8) {
        simd_fluid_particle* particle = &fluid->particles[i];
        
        u32 x = (i % particles_per_dim);
        u32 y = ((i / particles_per_dim) % particles_per_dim);
        u32 z = (i / (particles_per_dim * particles_per_dim));
        
        particle->position.x = _mm256_set1_ps(bounds_min->x + x * spacing);
        particle->position.y = _mm256_set1_ps(bounds_min->y + y * spacing);
        particle->position.z = _mm256_set1_ps(bounds_min->z + z * spacing);
        
        particle->velocity.x = _mm256_setzero_ps();
        particle->velocity.y = _mm256_setzero_ps();
        particle->velocity.z = _mm256_setzero_ps();
        
        particle->acceleration.x = _mm256_setzero_ps();
        particle->acceleration.y = _mm256_setzero_ps();
        particle->acceleration.z = _mm256_setzero_ps();
        
        particle->force.x = _mm256_setzero_ps();
        particle->force.y = _mm256_setzero_ps();
        particle->force.z = _mm256_setzero_ps();
        
        particle->density = fluid->rest_density;
        particle->pressure = _mm256_setzero_ps();
        particle->mass = _mm256_set1_ps(0.02f); // 20g per particle
        
        particle->id = i;
    }
}

void simd_fluid_destroy(simd_fluid_system* fluid) {
    if (fluid->particles) {
        free(fluid->particles);
        fluid->particles = NULL;
    }
    fluid->particle_count = 0;
}

__m256 simd_sph_poly6_kernel(__m256 r_sq, const simd_sph_kernel* kernel) {
    __m256 h_sq = kernel->h2;
    __m256 diff = _mm256_sub_ps(h_sq, r_sq);
    __m256 mask = _mm256_cmp_ps(r_sq, h_sq, _CMP_LT_OQ); // r < h
    
    __m256 result = _mm256_mul_ps(kernel->poly6_const, _mm256_mul_ps(diff, diff));
    result = _mm256_mul_ps(result, diff);
    
    return _mm256_and_ps(mask, result);
}

__m256 simd_sph_spiky_gradient(__m256 r, const simd_sph_kernel* kernel) {
    __m256 h = kernel->h;
    __m256 diff = _mm256_sub_ps(h, r);
    __m256 mask = _mm256_cmp_ps(r, _mm256_setzero_ps(), _CMP_GT_OQ); // r > 0
    mask = _mm256_and_ps(mask, _mm256_cmp_ps(r, h, _CMP_LT_OQ)); // r < h
    
    __m256 result = _mm256_mul_ps(kernel->spiky_const, _mm256_mul_ps(diff, diff));
    
    return _mm256_and_ps(mask, result);
}

__m256 simd_sph_viscosity_laplacian(__m256 r, const simd_sph_kernel* kernel) {
    __m256 mask = _mm256_cmp_ps(r, kernel->h, _CMP_LT_OQ); // r < h
    __m256 result = kernel->visc_const;
    
    return _mm256_and_ps(mask, result);
}

void simd_fluid_compute_density_pressure(simd_fluid_system* fluid) {
    // Reset densities
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        fluid->particles[i].density = _mm256_setzero_ps();
    }
    
    // Compute density using SPH
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        simd_fluid_particle* pi = &fluid->particles[i];
        
        for (u32 j = 0; j < fluid->particle_count; j += 8) {
            simd_fluid_particle* pj = &fluid->particles[j];
            
            // Calculate distance
            __m256 dx = _mm256_sub_ps(pj->position.x, pi->position.x);
            __m256 dy = _mm256_sub_ps(pj->position.y, pi->position.y);
            __m256 dz = _mm256_sub_ps(pj->position.z, pi->position.z);
            
            __m256 r_sq = _mm256_add_ps(_mm256_add_ps(
                _mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
            
            // Apply Poly6 kernel
            __m256 w = simd_sph_poly6_kernel(r_sq, &fluid->kernel);
            __m256 mass_contribution = _mm256_mul_ps(pj->mass, w);
            pi->density = _mm256_add_ps(pi->density, mass_contribution);
        }
        
        // Compute pressure from density
        __m256 density_diff = _mm256_sub_ps(pi->density, fluid->rest_density);
        pi->pressure = _mm256_mul_ps(fluid->gas_constant, density_diff);
    }
}

void simd_fluid_compute_forces(simd_fluid_system* fluid) {
    // Reset forces
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        simd_fluid_particle* particle = &fluid->particles[i];
        particle->force.x = _mm256_setzero_ps();
        particle->force.y = _mm256_mul_ps(particle->mass, fluid->gravity);
        particle->force.z = _mm256_setzero_ps();
    }
    
    // Compute pressure and viscosity forces
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        simd_fluid_particle* pi = &fluid->particles[i];
        
        for (u32 j = 0; j < fluid->particle_count; j += 8) {
            if (i == j) continue;
            
            simd_fluid_particle* pj = &fluid->particles[j];
            
            // Calculate distance and direction
            __m256 dx = _mm256_sub_ps(pj->position.x, pi->position.x);
            __m256 dy = _mm256_sub_ps(pj->position.y, pi->position.y);
            __m256 dz = _mm256_sub_ps(pj->position.z, pi->position.z);
            
            __m256 r_sq = _mm256_add_ps(_mm256_add_ps(
                _mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
            __m256 r = _mm256_sqrt_ps(r_sq);
            
            __m256 min_r = _mm256_set1_ps(0.001f);
            r = _mm256_max_ps(r, min_r);
            
            // Normalize direction
            __m256 inv_r = _mm256_div_ps(_mm256_set1_ps(1.0f), r);
            __m256 dir_x = _mm256_mul_ps(dx, inv_r);
            __m256 dir_y = _mm256_mul_ps(dy, inv_r);
            __m256 dir_z = _mm256_mul_ps(dz, inv_r);
            
            // Pressure force (Spiky kernel)
            __m256 spiky_grad = simd_sph_spiky_gradient(r, &fluid->kernel);
            __m256 pressure_term = _mm256_div_ps(
                _mm256_add_ps(pi->pressure, pj->pressure), _mm256_set1_ps(2.0f));
            __m256 pressure_force = _mm256_mul_ps(
                _mm256_mul_ps(pj->mass, pressure_term), spiky_grad);
            
            // Viscosity force
            __m256 visc_lap = simd_sph_viscosity_laplacian(r, &fluid->kernel);
            __m256 vel_diff_x = _mm256_sub_ps(pj->velocity.x, pi->velocity.x);
            __m256 vel_diff_y = _mm256_sub_ps(pj->velocity.y, pi->velocity.y);
            __m256 vel_diff_z = _mm256_sub_ps(pj->velocity.z, pi->velocity.z);
            
            __m256 viscosity_force = _mm256_mul_ps(
                _mm256_mul_ps(pj->mass, fluid->viscosity), visc_lap);
            viscosity_force = _mm256_mul_ps(viscosity_force, _mm256_set1_ps(-1.0f));
            
            // Apply forces
            pi->force.x = _mm256_fmadd_ps(dir_x, pressure_force, pi->force.x);
            pi->force.y = _mm256_fmadd_ps(dir_y, pressure_force, pi->force.y);
            pi->force.z = _mm256_fmadd_ps(dir_z, pressure_force, pi->force.z);
            
            pi->force.x = _mm256_fmadd_ps(vel_diff_x, viscosity_force, pi->force.x);
            pi->force.y = _mm256_fmadd_ps(vel_diff_y, viscosity_force, pi->force.y);
            pi->force.z = _mm256_fmadd_ps(vel_diff_z, viscosity_force, pi->force.z);
        }
    }
}

void simd_fluid_integrate(simd_fluid_system* fluid, f32 dt) {
    __m256 dt_vec = _mm256_set1_ps(dt);
    
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        simd_fluid_particle* particle = &fluid->particles[i];
        
        // Update acceleration
        particle->acceleration.x = _mm256_mul_ps(particle->force.x, 
            _mm256_div_ps(_mm256_set1_ps(1.0f), particle->mass));
        particle->acceleration.y = _mm256_mul_ps(particle->force.y, 
            _mm256_div_ps(_mm256_set1_ps(1.0f), particle->mass));
        particle->acceleration.z = _mm256_mul_ps(particle->force.z, 
            _mm256_div_ps(_mm256_set1_ps(1.0f), particle->mass));
        
        // Update velocity
        particle->velocity.x = _mm256_fmadd_ps(particle->acceleration.x, dt_vec, particle->velocity.x);
        particle->velocity.y = _mm256_fmadd_ps(particle->acceleration.y, dt_vec, particle->velocity.y);
        particle->velocity.z = _mm256_fmadd_ps(particle->acceleration.z, dt_vec, particle->velocity.z);
        
        // Apply damping
        particle->velocity.x = _mm256_mul_ps(particle->velocity.x, fluid->damping);
        particle->velocity.y = _mm256_mul_ps(particle->velocity.y, fluid->damping);
        particle->velocity.z = _mm256_mul_ps(particle->velocity.z, fluid->damping);
        
        // Update position
        particle->position.x = _mm256_fmadd_ps(particle->velocity.x, dt_vec, particle->position.x);
        particle->position.y = _mm256_fmadd_ps(particle->velocity.y, dt_vec, particle->position.y);
        particle->position.z = _mm256_fmadd_ps(particle->velocity.z, dt_vec, particle->position.z);
        
        // Reset forces
        particle->force.x = _mm256_setzero_ps();
        particle->force.y = _mm256_setzero_ps();
        particle->force.z = _mm256_setzero_ps();
    }
}

void simd_fluid_handle_boundaries(simd_fluid_system* fluid) {
    __m256 restitution = _mm256_set1_ps(0.5f);
    
    for (u32 i = 0; i < fluid->particle_count; i += 8) {
        simd_fluid_particle* particle = &fluid->particles[i];
        
        // X boundaries
        __m256 min_x = _mm256_set1_ps(fluid->bounds_min.x);
        __m256 max_x = _mm256_set1_ps(fluid->bounds_max.x);
        
        __m256 hit_min_x = _mm256_cmp_ps(particle->position.x, min_x, _CMP_LT_OQ);
        __m256 hit_max_x = _mm256_cmp_ps(particle->position.x, max_x, _CMP_GT_OQ);
        
        particle->position.x = _mm256_max_ps(particle->position.x, min_x);
        particle->position.x = _mm256_min_ps(particle->position.x, max_x);
        
        __m256 vel_bounce_x = _mm256_mul_ps(particle->velocity.x, _mm256_set1_ps(-restitution.m256_f32[0]));
        particle->velocity.x = _mm256_blendv_ps(particle->velocity.x, vel_bounce_x, 
            _mm256_or_ps(hit_min_x, hit_max_x));
        
        // Y boundaries
        __m256 min_y = _mm256_set1_ps(fluid->bounds_min.y);
        __m256 max_y = _mm256_set1_ps(fluid->bounds_max.y);
        
        __m256 hit_min_y = _mm256_cmp_ps(particle->position.y, min_y, _CMP_LT_OQ);
        __m256 hit_max_y = _mm256_cmp_ps(particle->position.y, max_y, _CMP_GT_OQ);
        
        particle->position.y = _mm256_max_ps(particle->position.y, min_y);
        particle->position.y = _mm256_min_ps(particle->position.y, max_y);
        
        __m256 vel_bounce_y = _mm256_mul_ps(particle->velocity.y, _mm256_set1_ps(-restitution.m256_f32[0]));
        particle->velocity.y = _mm256_blendv_ps(particle->velocity.y, vel_bounce_y, 
            _mm256_or_ps(hit_min_y, hit_max_y));
        
        // Z boundaries
        __m256 min_z = _mm256_set1_ps(fluid->bounds_min.z);
        __m256 max_z = _mm256_set1_ps(fluid->bounds_max.z);
        
        __m256 hit_min_z = _mm256_cmp_ps(particle->position.z, min_z, _CMP_LT_OQ);
        __m256 hit_max_z = _mm256_cmp_ps(particle->position.z, max_z, _CMP_GT_OQ);
        
        particle->position.z = _mm256_max_ps(particle->position.z, min_z);
        particle->position.z = _mm256_min_ps(particle->position.z, max_z);
        
        __m256 vel_bounce_z = _mm256_mul_ps(particle->velocity.z, _mm256_set1_ps(-restitution.m256_f32[0]));
        particle->velocity.z = _mm256_blendv_ps(particle->velocity.z, vel_bounce_z, 
            _mm256_or_ps(hit_min_z, hit_max_z));
    }
}

void simd_fluid_update(simd_fluid_system* fluid, f32 dt) {
    simd_fluid_compute_density_pressure(fluid);
    simd_fluid_compute_forces(fluid);
    simd_fluid_integrate(fluid, dt);
    simd_fluid_handle_boundaries(fluid);
}
