// src/engine/physics/simd/physics_simd.c
//
// Purpose: SIMD-optimized physics operations
// Provides high-performance physics calculations using SIMD instructions

#include "physics/simd/physics_simd.h"
#include "core/memory/unified_allocator.h"
#include "core/logger/unified_logger.h"
#include <immintrin.h>  // For SSE/AVX intrinsics
#include <string.h>

// ============================================================================
// SIMD DETECTION AND UTILITIES
// ============================================================================

typedef enum SIMDLevel {
    SIMD_LEVEL_NONE = 0,
    SIMD_LEVEL_SSE2,
    SIMD_LEVEL_SSE4_1,
    SIMD_LEVEL_AVX,
    SIMD_LEVEL_AVX2,
    SIMD_LEVEL_AVX512
} SIMDLevel;

static SIMDLevel g_simd_level = SIMD_LEVEL_NONE;

static SIMDLevel detect_simd_level(void) {
    // CPUID detection for SIMD capabilities
    u32 eax, ebx, ecx, edx;
    
    // Check for SSE2
    __cpuid(1, eax, ebx, ecx, edx);
    if (!(edx & (1 << 26))) return SIMD_LEVEL_NONE;  // No SSE2
    
    // Check for SSE4.1
    if (ecx & (1 << 19)) return SIMD_LEVEL_SSE4_1;
    
    // Check for AVX
    if (ecx & (1 << 28)) {
        // Check for AVX2
        __cpuid(7, eax, ebx, ecx, edx);
        if (ebx & (1 << 5)) return SIMD_LEVEL_AVX2;
        return SIMD_LEVEL_AVX;
    }
    
    return SIMD_LEVEL_SSE2;
}

void physics_simd_initialize(void) {
    g_simd_level = detect_simd_level();
    
    const char* level_names[] = {
        "None", "SSE2", "SSE4.1", "AVX", "AVX2", "AVX512"
    };
    
    LOG_INFO_CAT(LOG_CAT_PHYSICS, "Physics SIMD initialized: %s", level_names[g_simd_level]);
}

SIMDLevel physics_simd_get_level(void) {
    return g_simd_level;
}

// ============================================================================
// VECTOR OPERATIONS - SIMD IMPLEMENTATIONS
// ============================================================================

void physics_simd_vec3_add_batch(const vec3* a, const vec3* b, vec3* result, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation - 8 floats at a time
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        f32* r_ptr = (f32*)result;
        
        u32 avx_count = (count * 3) / 8;
        u32 remainder = (count * 3) % 8;
        
        for (u32 i = 0; i < avx_count; i++) {
            __m256 va = _mm256_load_ps(a_ptr + i * 8);
            __m256 vb = _mm256_load_ps(b_ptr + i * 8);
            __m256 vr = _mm256_add_ps(va, vb);
            _mm256_store_ps(r_ptr + i * 8, vr);
        }
        
        // Handle remainder
        for (u32 i = avx_count * 8; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] + b_ptr[i];
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation - 4 floats at a time
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        f32* r_ptr = (f32*)result;
        
        u32 sse_count = (count * 3) / 4;
        u32 remainder = (count * 3) % 4;
        
        for (u32 i = 0; i < sse_count; i++) {
            __m128 va = _mm_load_ps(a_ptr + i * 4);
            __m128 vb = _mm_load_ps(b_ptr + i * 4);
            __m128 vr = _mm_add_ps(va, vb);
            _mm_store_ps(r_ptr + i * 4, vr);
        }
        
        // Handle remainder
        for (u32 i = sse_count * 4; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] + b_ptr[i];
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            result[i].x = a[i].x + b[i].x;
            result[i].y = a[i].y + b[i].y;
            result[i].z = a[i].z + b[i].z;
        }
    }
}

void physics_simd_vec3_subtract_batch(const vec3* a, const vec3* b, vec3* result, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        f32* r_ptr = (f32*)result;
        
        u32 avx_count = (count * 3) / 8;
        u32 remainder = (count * 3) % 8;
        
        for (u32 i = 0; i < avx_count; i++) {
            __m256 va = _mm256_load_ps(a_ptr + i * 8);
            __m256 vb = _mm256_load_ps(b_ptr + i * 8);
            __m256 vr = _mm256_sub_ps(va, vb);
            _mm256_store_ps(r_ptr + i * 8, vr);
        }
        
        // Handle remainder
        for (u32 i = avx_count * 8; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] - b_ptr[i];
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        f32* r_ptr = (f32*)result;
        
        u32 sse_count = (count * 3) / 4;
        u32 remainder = (count * 3) % 4;
        
        for (u32 i = 0; i < sse_count; i++) {
            __m128 va = _mm_load_ps(a_ptr + i * 4);
            __m128 vb = _mm_load_ps(b_ptr + i * 4);
            __m128 vr = _mm_sub_ps(va, vb);
            _mm_store_ps(r_ptr + i * 4, vr);
        }
        
        // Handle remainder
        for (u32 i = sse_count * 4; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] - b_ptr[i];
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            result[i].x = a[i].x - b[i].x;
            result[i].y = a[i].y - b[i].y;
            result[i].z = a[i].z - b[i].z;
        }
    }
}

void physics_simd_vec3_multiply_batch(const vec3* a, f32 scalar, vec3* result, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation
        const f32* a_ptr = (const f32*)a;
        f32* r_ptr = (f32*)result;
        
        __m256 vscalar = _mm256_set1_ps(scalar);
        
        u32 avx_count = (count * 3) / 8;
        u32 remainder = (count * 3) % 8;
        
        for (u32 i = 0; i < avx_count; i++) {
            __m256 va = _mm256_load_ps(a_ptr + i * 8);
            __m256 vr = _mm256_mul_ps(va, vscalar);
            _mm256_store_ps(r_ptr + i * 8, vr);
        }
        
        // Handle remainder
        for (u32 i = avx_count * 8; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] * scalar;
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        const f32* a_ptr = (const f32*)a;
        f32* r_ptr = (f32*)result;
        
        __m128 vscalar = _mm_set1_ps(scalar);
        
        u32 sse_count = (count * 3) / 4;
        u32 remainder = (count * 3) % 4;
        
        for (u32 i = 0; i < sse_count; i++) {
            __m128 va = _mm_load_ps(a_ptr + i * 4);
            __m128 vr = _mm_mul_ps(va, vscalar);
            _mm_store_ps(r_ptr + i * 4, vr);
        }
        
        // Handle remainder
        for (u32 i = sse_count * 4; i < count * 3; i++) {
            r_ptr[i] = a_ptr[i] * scalar;
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            result[i].x = a[i].x * scalar;
            result[i].y = a[i].y * scalar;
            result[i].z = a[i].z * scalar;
        }
    }
}

f32 physics_simd_vec3_dot_batch(const vec3* a, const vec3* b, f32* result, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        
        __m256 sum = _mm256_setzero_ps();
        
        u32 avx_count = (count * 3) / 8;
        u32 remainder = (count * 3) % 8;
        
        for (u32 i = 0; i < avx_count; i++) {
            __m256 va = _mm256_load_ps(a_ptr + i * 8);
            __m256 vb = _mm256_load_ps(b_ptr + i * 8);
            __m256 product = _mm256_mul_ps(va, vb);
            sum = _mm256_add_ps(sum, product);
        }
        
        // Horizontal sum
        f32 sum_array[8];
        _mm256_store_ps(sum_array, sum);
        
        f32 total = 0.0f;
        for (int i = 0; i < 8; i++) {
            total += sum_array[i];
        }
        
        // Handle remainder
        for (u32 i = avx_count * 8; i < count * 3; i++) {
            total += a_ptr[i] * b_ptr[i];
        }
        
        // Store results per vector
        for (u32 i = 0; i < count; i++) {
            result[i] = a[i].x * b[i].x + a[i].y * b[i].y + a[i].z * b[i].z;
        }
        
        return total;
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        const f32* a_ptr = (const f32*)a;
        const f32* b_ptr = (const f32*)b;
        
        __m128 sum = _mm_setzero_ps();
        
        u32 sse_count = (count * 3) / 4;
        u32 remainder = (count * 3) % 4;
        
        for (u32 i = 0; i < sse_count; i++) {
            __m128 va = _mm_load_ps(a_ptr + i * 4);
            __m128 vb = _mm_load_ps(b_ptr + i * 4);
            __m128 product = _mm_mul_ps(va, vb);
            sum = _mm_add_ps(sum, product);
        }
        
        // Horizontal sum
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        f32 total = _mm_cvtss_f32(sum);
        
        // Handle remainder
        for (u32 i = sse_count * 4; i < count * 3; i++) {
            total += a_ptr[i] * b_ptr[i];
        }
        
        // Store results per vector
        for (u32 i = 0; i < count; i++) {
            result[i] = a[i].x * b[i].x + a[i].y * b[i].y + a[i].z * b[i].z;
        }
        
        return total;
    } else {
        // Scalar fallback
        f32 total = 0.0f;
        for (u32 i = 0; i < count; i++) {
            result[i] = a[i].x * b[i].x + a[i].y * b[i].y + a[i].z * b[i].z;
            total += result[i];
        }
        return total;
    }
}

void physics_simd_vec3_cross_batch(const vec3* a, const vec3* b, vec3* result, u32 count) {
    // Cross product is more complex, so we'll use a hybrid approach
    if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation - process 4 vectors at a time
        for (u32 i = 0; i < count; i++) {
            const __m128 va = _mm_set_ps(0, a[i].z, a[i].y, a[i].x);
            const __m128 vb = _mm_set_ps(0, b[i].z, b[i].y, b[i].x);
            
            // Shuffle and multiply
            __m128 va_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
            __m128 vb_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));
            __m128 va_zxy = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2));
            __m128 vb_zxy = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2));
            
            __m128 cross1 = _mm_mul_ps(va_yzx, vb_zxy);
            __m128 cross2 = _mm_mul_ps(va_zxy, vb_yzx);
            __m128 cross = _mm_sub_ps(cross1, cross2);
            
            // Store result
            f32 temp[4];
            _mm_store_ps(temp, cross);
            result[i].x = temp[0];
            result[i].y = temp[1];
            result[i].z = temp[2];
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            result[i].x = a[i].y * b[i].z - a[i].z * b[i].y;
            result[i].y = a[i].z * b[i].x - a[i].x * b[i].z;
            result[i].z = a[i].x * b[i].y - a[i].y * b[i].x;
        }
    }
}

// ============================================================================
// RIGID BODY OPERATIONS - SIMD IMPLEMENTATIONS
// ============================================================================

void physics_simd_integrate_velocity_batch(RigidBody* bodies, f32 dt, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation - integrate 8 bodies at a time
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) continue;
            
            // Load position and velocity
            __m256 pos = _mm256_load_ps(body->position);
            __m256 vel = _mm256_load_ps(body->velocity);
            __m256 dt_vec = _mm256_set1_ps(dt);
            
            // position += velocity * dt
            __m256 new_pos = _mm256_add_ps(pos, _mm256_mul_ps(vel, dt_vec));
            _mm256_store_ps(body->position, new_pos);
            
            // Load angular velocity and rotation
            __m256 ang_vel = _mm256_load_ps(body->angular_velocity);
            __m256 rot = _mm256_load_ps(body->rotation);
            
            // Simple angular integration (would need quaternion multiplication for accuracy)
            __m256 ang_delta = _mm256_mul_ps(ang_vel, dt_vec);
            __m256 new_rot = _mm256_add_ps(rot, ang_delta);
            _mm256_store_ps(body->rotation, new_rot);
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) continue;
            
            // Load position and velocity
            __m128 pos = _mm_load_ps(body->position);
            __m128 vel = _mm_load_ps(body->velocity);
            __m128 dt_vec = _mm_set1_ps(dt);
            
            // position += velocity * dt
            __m128 new_pos = _mm_add_ps(pos, _mm_mul_ps(vel, dt_vec));
            _mm_store_ps(body->position, new_pos);
            
            // Load angular velocity and rotation
            __m128 ang_vel = _mm_load_ps(body->angular_velocity);
            __m128 rot = _mm_load_ps(body->rotation);
            
            // Simple angular integration
            __m128 ang_delta = _mm_mul_ps(ang_vel, dt_vec);
            __m128 new_rot = _mm_add_ps(rot, ang_delta);
            _mm_store_ps(body->rotation, new_rot);
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) continue;
            
            body->position[0] += body->velocity[0] * dt;
            body->position[1] += body->velocity[1] * dt;
            body->position[2] += body->velocity[2] * dt;
            
            body->rotation[0] += body->angular_velocity[0] * dt;
            body->rotation[1] += body->angular_velocity[1] * dt;
            body->rotation[2] += body->angular_velocity[2] * dt;
        }
    }
}

void physics_simd_apply_force_batch(RigidBody* bodies, const vec3* forces, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC) continue;
            
            // Load accumulated force and new force
            __m256 acc_force = _mm256_load_ps(body->accumulated_force);
            __m256 new_force = _mm256_load_ps((const f32*)&forces[i]);
            
            // accumulated_force += force
            __m256 total_force = _mm256_add_ps(acc_force, new_force);
            _mm256_store_ps(body->accumulated_force, total_force);
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC) continue;
            
            // Load accumulated force and new force
            __m128 acc_force = _mm_load_ps(body->accumulated_force);
            __m128 new_force = _mm_load_ps((const f32*)&forces[i]);
            
            // accumulated_force += force
            __m128 total_force = _mm_add_ps(acc_force, new_force);
            _mm_store_ps(body->accumulated_force, total_force);
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            RigidBody* body = &bodies[i];
            if (body->type != RIGID_BODY_DYNAMIC) continue;
            
            body->accumulated_force[0] += forces[i].x;
            body->accumulated_force[1] += forces[i].y;
            body->accumulated_force[2] += forces[i].z;
        }
    }
}

void physics_simd_broadphase_aabb_update_batch(BroadPhaseAABB* aabbs, const RigidBody* bodies, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation - update 8 AABBs at a time
        for (u32 i = 0; i < count; i++) {
            const RigidBody* body = &bodies[i];
            BroadPhaseAABB* aabb = &aabbs[i];
            
            // Simple AABB update based on body position and shape bounds
            // This is a simplified implementation - real implementation would transform shape bounds
            
            __m256 pos = _mm256_load_ps(body->position);
            __m256 margin = _mm256_set1_ps(0.1f); // Simple margin
            
            __m256 min_bounds = _mm256_sub_ps(pos, margin);
            __m256 max_bounds = _mm256_add_ps(pos, margin);
            
            _mm256_store_ps(aabb->min, min_bounds);
            _mm256_store_ps(aabb->max, max_bounds);
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        for (u32 i = 0; i < count; i++) {
            const RigidBody* body = &bodies[i];
            BroadPhaseAABB* aabb = &aabbs[i];
            
            __m128 pos = _mm_load_ps(body->position);
            __m128 margin = _mm_set1_ps(0.1f);
            
            __m128 min_bounds = _mm_sub_ps(pos, margin);
            __m128 max_bounds = _mm_add_ps(pos, margin);
            
            _mm_store_ps(aabb->min, min_bounds);
            _mm_store_ps(aabb->max, max_bounds);
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            const RigidBody* body = &bodies[i];
            BroadPhaseAABB* aabb = &aabbs[i];
            
            aabb->min[0] = body->position[0] - 0.1f;
            aabb->min[1] = body->position[1] - 0.1f;
            aabb->min[2] = body->position[2] - 0.1f;
            
            aabb->max[0] = body->position[0] + 0.1f;
            aabb->max[1] = body->position[1] + 0.1f;
            aabb->max[2] = body->position[2] + 0.1f;
        }
    }
}

// ============================================================================
// COLLISION DETECTION - SIMD IMPLEMENTATIONS
// ============================================================================

void physics_simd_sphere_sphere_collision_batch(const vec3* centers_a, const f32* radii_a,
                                               const vec3* centers_b, const f32* radii_b,
                                               bool* results, u32 count) {
    if (g_simd_level >= SIMD_LEVEL_AVX) {
        // AVX implementation - check 8 sphere pairs at a time
        for (u32 i = 0; i < count; i++) {
            // Load centers and radii
            __m256 center_a = _mm256_load_ps((const f32*)&centers_a[i]);
            __m256 center_b = _mm256_load_ps((const f32*)&centers_b[i]);
            __m256 radius_a = _mm256_set1_ps(radii_a[i]);
            __m256 radius_b = _mm256_set1_ps(radii_b[i]);
            
            // Calculate distance squared
            __m256 diff = _mm256_sub_ps(center_a, center_b);
            __m256 dist_sq = _mm256_mul_ps(diff, diff);
            
            // Horizontal sum for distance squared
            f32 dist_components[8];
            _mm256_store_ps(dist_components, dist_sq);
            f32 distance_sq = dist_components[0] + dist_components[1] + dist_components[2];
            
            // Calculate combined radius squared
            f32 combined_radius = radii_a[i] + radii_b[i];
            f32 radius_sq = combined_radius * combined_radius;
            
            results[i] = distance_sq <= radius_sq;
        }
    } else if (g_simd_level >= SIMD_LEVEL_SSE2) {
        // SSE2 implementation
        for (u32 i = 0; i < count; i++) {
            __m128 center_a = _mm_load_ps((const f32*)&centers_a[i]);
            __m128 center_b = _mm_load_ps((const f32*)&centers_b[i]);
            __m128 radius_a = _mm_set1_ps(radii_a[i]);
            __m128 radius_b = _mm_set1_ps(radii_b[i]);
            
            __m128 diff = _mm_sub_ps(center_a, center_b);
            __m128 dist_sq = _mm_mul_ps(diff, diff);
            
            f32 dist_components[4];
            _mm_store_ps(dist_components, dist_sq);
            f32 distance_sq = dist_components[0] + dist_components[1] + dist_components[2];
            
            f32 combined_radius = radii_a[i] + radii_b[i];
            f32 radius_sq = combined_radius * combined_radius;
            
            results[i] = distance_sq <= radius_sq;
        }
    } else {
        // Scalar fallback
        for (u32 i = 0; i < count; i++) {
            f32 dx = centers_a[i].x - centers_b[i].x;
            f32 dy = centers_a[i].y - centers_b[i].y;
            f32 dz = centers_a[i].z - centers_b[i].z;
            f32 distance_sq = dx * dx + dy * dy + dz * dz;
            f32 combined_radius = radii_a[i] + radii_b[i];
            results[i] = distance_sq <= (combined_radius * combined_radius);
        }
    }
}
