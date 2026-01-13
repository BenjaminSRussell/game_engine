#pragma once

#include "simd_physics.h"
#include "../math/types.h"

// SIMD-Optimized Cloth Physics
// High-performance cloth simulation using vectorized operations

// Cloth Particle with SIMD support
typedef struct {
    simd_vec3 position;
    simd_vec3 old_position;
    simd_vec3 velocity;
    simd_vec3 force;
    __m256 mass;
    __m256 inv_mass;
    u32 pinned;
} simd_cloth_particle;

// Cloth Constraint with SIMD support
typedef struct {
    u32 particle_a;
    u32 particle_b;
    __m256 rest_length;
    __m256 stiffness;
} simd_cloth_constraint;

// SIMD Cloth System
typedef struct {
    simd_cloth_particle* particles;
    simd_cloth_constraint* constraints;
    u32 particle_count;
    u32 constraint_count;
    u32 width;
    u32 height;
    __m256 damping;
    __m256 gravity;
} simd_cloth_system;

// SIMD Cloth Operations
void simd_cloth_create(simd_cloth_system* cloth, u32 width, u32 height);
void simd_cloth_destroy(simd_cloth_system* cloth);
void simd_cloth_update(simd_cloth_system* cloth, f32 dt);
void simd_cloth_apply_forces(simd_cloth_system* cloth);
void simd_cloth_satisfy_constraints(simd_cloth_system* cloth, u32 iterations);
void simd_cloth_integrate(simd_cloth_system* cloth, f32 dt);

// SIMD Wind Forces
void simd_cloth_apply_wind(simd_cloth_system* cloth, const simd_vec3* wind_direction, __m256 wind_strength);

// SIMD Collision Detection
void simd_cloth_sphere_collision(simd_cloth_system* cloth, const simd_vec3* sphere_center, __m256 sphere_radius);

#endif // SIMD_CLOTH_H
