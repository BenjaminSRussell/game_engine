// src/engine/physics/simd/physics_simd.h
//
// Purpose: SIMD-optimized physics operations header
// Provides high-performance physics calculations using SIMD instructions

#ifndef PHYSICS_SIMD_H
#define PHYSICS_SIMD_H

#include "core/math/types.h"
#include "physics/core/physics_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SIMD LEVEL DETECTION
// ============================================================================

// Initialize SIMD system and detect capabilities
void physics_simd_initialize(void);

// Get current SIMD level
typedef enum SIMDLevel {
    SIMD_LEVEL_NONE = 0,
    SIMD_LEVEL_SSE2,
    SIMD_LEVEL_SSE4_1,
    SIMD_LEVEL_AVX,
    SIMD_LEVEL_AVX2,
    SIMD_LEVEL_AVX512
} SIMDLevel;

SIMDLevel physics_simd_get_level(void);

// ============================================================================
// VECTOR OPERATIONS - SIMD BATCH PROCESSING
// ============================================================================

// Batch vector addition: result[i] = a[i] + b[i]
void physics_simd_vec3_add_batch(const vec3* a, const vec3* b, vec3* result, u32 count);

// Batch vector subtraction: result[i] = a[i] - b[i]
void physics_simd_vec3_subtract_batch(const vec3* a, const vec3* b, vec3* result, u32 count);

// Batch vector multiplication: result[i] = a[i] * scalar
void physics_simd_vec3_multiply_batch(const vec3* a, f32 scalar, vec3* result, u32 count);

// Batch dot product: result[i] = dot(a[i], b[i]), returns sum of all results
f32 physics_simd_vec3_dot_batch(const vec3* a, const vec3* b, f32* result, u32 count);

// Batch cross product: result[i] = cross(a[i], b[i])
void physics_simd_vec3_cross_batch(const vec3* a, const vec3* b, vec3* result, u32 count);

// ============================================================================
// RIGID BODY OPERATIONS - SIMD BATCH PROCESSING
// ============================================================================

// Batch velocity integration for rigid bodies
void physics_simd_integrate_velocity_batch(RigidBody* bodies, f32 dt, u32 count);

// Batch force application to rigid bodies
void physics_simd_apply_force_batch(RigidBody* bodies, const vec3* forces, u32 count);

// Batch AABB updates for broadphase collision detection
void physics_simd_broadphase_aabb_update_batch(BroadPhaseAABB* aabbs, const RigidBody* bodies, u32 count);

// ============================================================================
// COLLISION DETECTION - SIMD BATCH PROCESSING
// ============================================================================

// Batch sphere-sphere collision detection
void physics_simd_sphere_sphere_collision_batch(const vec3* centers_a, const f32* radii_a,
                                               const vec3* centers_b, const f32* radii_b,
                                               bool* results, u32 count);

// ============================================================================
// MATRIX OPERATIONS - SIMD BATCH PROCESSING
// ============================================================================

// Batch matrix-vector multiplication: result[i] = matrix[i] * vector[i]
void physics_simd_mat4_vec3_multiply_batch(const mat4* matrices, const vec3* vectors, 
                                          vec3* result, u32 count);

// Batch matrix multiplication: result[i] = a[i] * b[i]
void physics_simd_mat4_multiply_batch(const mat4* a, const mat4* b, mat4* result, u32 count);

// ============================================================================
// TRANSFORM OPERATIONS - SIMD BATCH PROCESSING
// ============================================================================

// Batch transform points: result[i] = transform * points[i]
void physics_simd_transform_points_batch(const mat4* transform, const vec3* points, 
                                       vec3* result, u32 count);

// Batch inverse transform points: result[i] = inverse(transform) * points[i]
void physics_simd_inverse_transform_points_batch(const mat4* transform, const vec3* points, 
                                                 vec3* result, u32 count);

// ============================================================================
// CONSTRAINT SOLVING - SIMD BATCH PROCESSING
// ============================================================================

// Batch constraint resolution
void physics_simd_resolve_constraints_batch(Constraint* constraints, RigidBody* bodies, 
                                           u32 constraint_count, u32 iterations);

// Batch impulse application
void physics_simd_apply_impulse_batch(RigidBody* bodies, const vec3* impulses, 
                                     const vec3* contact_points, u32 count);

// ============================================================================
// PERFORMANCE UTILITIES
// ============================================================================

// Get SIMD performance statistics
typedef struct PhysicsSIMDStats {
    u32 total_operations;
    u32 simd_operations;
    u32 scalar_fallbacks;
    f32 simd_speedup_factor;
} PhysicsSIMDStats;

void physics_simd_get_stats(PhysicsSIMDStats* out_stats);
void physics_simd_reset_stats(void);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

// Use SIMD operations if available, otherwise fallback
#define PHYSICS_SIMD_VEC3_ADD_BATCH(a, b, result, count) \
    do { \
        if (physics_simd_get_level() >= SIMD_LEVEL_SSE2) { \
            physics_simd_vec3_add_batch(a, b, result, count); \
        } else { \
            for (u32 i = 0; i < count; i++) { \
                result[i] = vec3_add(a[i], b[i]); \
            } \
        } \
    } while(0)

#define PHYSICS_SIMD_VEC3_MULTIPLY_BATCH(a, scalar, result, count) \
    do { \
        if (physics_simd_get_level() >= SIMD_LEVEL_SSE2) { \
            physics_simd_vec3_multiply_batch(a, scalar, result, count); \
        } else { \
            for (u32 i = 0; i < count; i++) { \
                result[i] = vec3_multiply(a[i], scalar); \
            } \
        } \
    } while(0)

// ============================================================================
// COMPATIBILITY CHECKS
// ============================================================================

// Check if SIMD is available for physics operations
static inline bool physics_simd_is_available(void) {
    return physics_simd_get_level() >= SIMD_LEVEL_SSE2;
}

// Check if AVX is available for enhanced performance
static inline bool physics_simd_avx_available(void) {
    return physics_simd_get_level() >= SIMD_LEVEL_AVX;
}

// Check if AVX2 is available for maximum performance
static inline bool physics_simd_avx2_available(void) {
    return physics_simd_get_level() >= SIMD_LEVEL_AVX2;
}

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_SIMD_H
