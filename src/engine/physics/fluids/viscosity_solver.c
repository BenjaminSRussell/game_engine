// Viscosity Solver - Complete (7 TODOs)
#include "physics/fluids/viscosity_solver.h"
#include <include/math/math.h>

#define SPH_KERNEL_RADIUS 1.0f

// XSPH Viscosity Kernel
float xsph_kernel(float r, float h) {
    if (r >= h) return 0.0f;
    float q = r / h;
    return 15.0f / (2.0f * 3.14159f * h * h * h) * powf(1.0f - q * q, 3.0f);
}

void viscosity_solve(Particle *particles, uint32_t count, float viscosity, float dt) {
    // XSPH smoothing of velocities
    for (uint32_t i = 0; i < count; i++) {
        Vec3 vel_correction = vec3(0, 0, 0);
        
        for (uint32_t j = 0; j < count; j++) {
            if (i == j) continue;
            
            Vec3 r = vec3_sub(particles[i].position, particles[j].position);
            float dist = vec3_length(r);
            
            if (dist < SPH_KERNEL_RADIUS) {
                float w = xsph_kernel(dist, SPH_KERNEL_RADIUS);
                Vec3 vel_diff = vec3_sub(particles[j].velocity, particles[i].velocity);
                vel_correction = vec3_add(vel_correction, vec3_scale(vel_diff, w));
            }
        }
        
        particles[i].velocity = vec3_add(particles[i].velocity, vec3_scale(vel_correction, viscosity * dt));
    }
}

// Temperature-dependent viscosity
float get_viscosity_for_temperature(float temp_celsius) {
    // Water: decreases with temperature
    // Simplified model
    if (temp_celsius < 0.0f) return 1000.0f; // Ice
    if (temp_celsius < 20.0f) return 1.0f + (20.0f - temp_celsius) * 0.05f;
    if (temp_celsius < 100.0f) return 1.0f;
    return 0.3f; // Steam
}

// Non-Newtonian behavior (shear-thinning/thickening)
float apply_non_newtonian(float strain_rate, float base_viscosity, float power_law_index) {
    // Power law: η = K * (γ̇)^(n-1)
    // n < 1: shear thinning (ketchup, blood)
    // n > 1: shear thickening (cornstarch solution)
    return base_viscosity * powf(fmaxf(strain_rate, 0.001f), power_law_index - 1.0f);
}
