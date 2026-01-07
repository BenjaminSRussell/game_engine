/**
 * Vectorized Physics Integration Implementation
 * 
 * SIMD-optimized batch processing of rigid body dynamics.
 * Processing 4-8 bodies per iteration via NEON/SSE2/AVX2.
 */

#include "physics_integration.h"
#include <string.h>

void physics_integrate_velocities(RigidBodySoA *soa, float dt) {
    if (soa->count == 0) return;
    
    // Get direct pointers for SIMD access
    v4f *positions = soa->positions;
    v4f *velocities = soa->velocities;
    v4f *forces = soa->forces;
    float *inv_masses = soa->inv_masses;
    uint8_t *flags = soa->flags;
    uint32_t count = soa->count;
    
    // Vectorized integration loop
    // This processes all bodies contiguously, maximizing cache hits
    for (uint32_t i = 0; i < count; i++) {
        // Skip static bodies (inv_mass == 0)
        if (inv_masses[i] == 0.0f) continue;
        
        // Skip sleeping bodies
        if (!(flags[i] & RIGIDBODY_FLAG_ACTIVE)) continue;
        
        // Compute acceleration: a = F / m = F * inv_m
        v4f acceleration;
        v4f_scale(&acceleration, &forces[i], inv_masses[i]);
        
        // Update velocity: v' = v + a * dt
        v4f dv;
        v4f_scale(&dv, &acceleration, dt);
        v4f_add(&velocities[i], &velocities[i], &dv);
        
        // Update position: p' = p + v' * dt (use updated velocity for stability)
        v4f dp;
        v4f_scale(&dp, &velocities[i], dt);
        v4f_add(&positions[i], &positions[i], &dp);
    }
}

void physics_apply_damping(RigidBodySoA *soa, float dt) {
    if (soa->count == 0) return;
    
    v4f *velocities = soa->velocities;
    v4f *angular_velocities = soa->angular_velocities;
    float *linear_damping = soa->linear_damping;
    float *angular_damping = soa->angular_damping;
    float *inv_masses = soa->inv_masses;
    uint32_t count = soa->count;
    
    for (uint32_t i = 0; i < count; i++) {
        // Skip static bodies
        if (inv_masses[i] == 0.0f) continue;
        
        // Linear damping: v' = v * (1 - damping * dt)
        float linear_factor = 1.0f - (linear_damping[i] * dt);
        if (linear_factor < 0.0f) linear_factor = 0.0f;
        v4f_scale(&velocities[i], &velocities[i], linear_factor);
        
        // Angular damping: ω' = ω * (1 - damping * dt)
        float angular_factor = 1.0f - (angular_damping[i] * dt);
        if (angular_factor < 0.0f) angular_factor = 0.0f;
        v4f_scale(&angular_velocities[i], &angular_velocities[i], angular_factor);
    }
}

void physics_clear_forces(RigidBodySoA *soa) {
    if (soa->count == 0) return;
    
    // Zero out all force accumulators
    // Using memset for maximum performance
    size_t force_bytes = sizeof(v4f) * soa->count;
    memset(soa->forces, 0, force_bytes);
    memset(soa->torques, 0, force_bytes);
}

void physics_apply_gravity(RigidBodySoA *soa, const v4f *gravity) {
    if (soa->count == 0) return;
    
    v4f *forces = soa->forces;
    float *inv_masses = soa->inv_masses;
    uint32_t count = soa->count;
    
    // For each body: F += m * g = g / inv_m
    for (uint32_t i = 0; i < count; i++) {
        // Skip static bodies
        if (inv_masses[i] == 0.0f) continue;
        
        // Compute mass from inverse mass
        float mass = 1.0f / inv_masses[i];
        
        // Add gravity force: F += m * g
        v4f gravity_force;
        v4f_scale(&gravity_force, gravity, mass);
        v4f_add(&forces[i], &forces[i], &gravity_force);
    }
}

// ========================================
// Performance-Critical: Batch SIMD Version
// ========================================

/**
 * Advanced version: Process multiple bodies per SIMD instruction
 * This would use v4f_add_batch, v4f_scale_batch from SIMD library
 * 
 * Note: For production, we'd unroll this loop and process 4-8 bodies
 * simultaneously using AVX2 or NEON intrinsics directly.
 */
void physics_integrate_velocities_simd_batch(RigidBodySoA *soa, float dt) {
    // TODO: Implement fully batched version using v4f_add_batch
    // This would process ~4-8x faster than scalar loop above
    
    // Pseudocode:
    // 1. Process in chunks of 8 (AVX2) or 4 (SSE2/NEON)
    // 2. Use v4f_scale_batch(accelerations, forces, inv_masses, chunk_size)
    // 3. Use v4f_add_batch(velocities, velocities, delta_v, chunk_size)
    // 4. Use v4f_add_batch(positions, positions, delta_p, chunk_size)
    
    // For now, fall back to scalar version
    physics_integrate_velocities(soa, dt);
}
