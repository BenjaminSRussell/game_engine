/*
 * bvh_simd.h
 * SIMD-optimized BVH operations for frustum culling
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_BVH_SIMD_H
#define GEOMETRY_BVH_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/math/math/vec3.h"
#include "../../core/math/math/vec4.h"
#include "../../core/math/math/aabb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SIMD ARCHITECTURE DETECTION
 * ============================================================================ */

#if defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    #define BVH_SIMD_NEON
    #include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64)
    #define BVH_SIMD_SSE
    #include <immintrin.h>
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Frustum planes in SIMD-friendly format
// Each plane stored as (nx, ny, nz, d) where n is normal and d is distance
typedef struct bvh_frustum_simd {
#ifdef BVH_SIMD_NEON
    float32x4_t planes[6];  // 6 frustum planes
#elif defined(BVH_SIMD_SSE)
    __m128 planes[6];
#else
    vec4_t planes[6];
#endif
} bvh_frustum_simd_t;

// AABB data in SoA (Structure of Arrays) layout for SIMD processing
// Stores 4 AABBs at once for batch processing
typedef struct bvh_aabb_batch {
#ifdef BVH_SIMD_NEON
    float32x4_t min_x, min_y, min_z;  // 4 AABBs' min points
    float32x4_t max_x, max_y, max_z;  // 4 AABBs' max points
#elif defined(BVH_SIMD_SSE)
    __m128 min_x, min_y, min_z;
    __m128 max_x, max_y, max_z;
#else
    float min_x[4], min_y[4], min_z[4];
    float max_x[4], max_y[4], max_z[4];
#endif
} bvh_aabb_batch_t;

// Result of batch frustum culling - bitmask where bit i indicates if AABB i is visible
typedef uint32_t bvh_visibility_mask_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Convert standard frustum planes to SIMD-optimized format
 * @param frustum_planes Array of 6 vec4_t planes (left, right, top, bottom, near, far)
 * @param out_simd_frustum Output SIMD frustum structure
 */
void bvh_simd_prepare_frustum(const vec4_t frustum_planes[6], bvh_frustum_simd_t* out_simd_frustum);

/**
 * Convert 4 AABBs from AoS to SoA layout for SIMD processing
 * @param aabbs Array of 4 AABBs
 * @param out_batch Output batch structure
 */
void bvh_simd_prepare_aabb_batch(const aabb_t aabbs[4], bvh_aabb_batch_t* out_batch);

/**
 * Test 4 AABBs against frustum simultaneously using SIMD
 * @param frustum SIMD-optimized frustum
 * @param aabb_batch Batch of 4 AABBs in SoA layout
 * @return Visibility mask (bit 0-3 for AABBs 0-3, 1=visible, 0=culled)
 */
bvh_visibility_mask_t bvh_simd_frustum_cull_batch(
    const bvh_frustum_simd_t* frustum,
    const bvh_aabb_batch_t* aabb_batch
);

/**
 * Test single AABB against frustum (convenience wrapper)
 * @param frustum_planes Standard frustum planes
 * @param aabb AABB to test
 * @return true if visible, false if culled
 */
bool bvh_simd_frustum_cull_single(const vec4_t frustum_planes[6], const aabb_t* aabb);

/**
 * Compute union of 4 AABBs using SIMD
 * @param aabb_batch Batch of 4 AABBs
 * @param out_union Output union AABB
 */
void bvh_simd_aabb_union_batch(const bvh_aabb_batch_t* aabb_batch, aabb_t* out_union);

/**
 * Test if 4 AABBs intersect with a reference AABB using SIMD
 * @param aabb_batch Batch of 4 AABBs
 * @param reference Reference AABB to test against
 * @return Intersection mask (bit i = 1 if AABB i intersects reference)
 */
bvh_visibility_mask_t bvh_simd_aabb_intersect_batch(
    const bvh_aabb_batch_t* aabb_batch,
    const aabb_t* reference
);

/* ============================================================================
 * PLATFORM-SPECIFIC IMPLEMENTATIONS
 * ============================================================================ */

#ifdef BVH_SIMD_NEON

/**
 * ARM NEON implementation: Test 4 AABBs against frustum
 * Optimized for Apple Silicon (M1/M2/M3)
 */
bvh_visibility_mask_t bvh_simd_frustum_cull_neon(
    const float32x4_t frustum_planes[6],
    float32x4_t aabb_min_x, float32x4_t aabb_min_y, float32x4_t aabb_min_z,
    float32x4_t aabb_max_x, float32x4_t aabb_max_y, float32x4_t aabb_max_z
);

#endif

#ifdef BVH_SIMD_SSE

/**
 * SSE implementation: Test 4 AABBs against frustum
 * Optimized for x86_64 processors
 */
bvh_visibility_mask_t bvh_simd_frustum_cull_sse(
    const __m128 frustum_planes[6],
    __m128 aabb_min_x, __m128 aabb_min_y, __m128 aabb_min_z,
    __m128 aabb_max_x, __m128 aabb_max_y, __m128 aabb_max_z
);

#endif

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

typedef struct bvh_simd_stats {
    uint64_t total_batches_tested;
    uint64_t total_aabbs_tested;
    uint64_t total_aabbs_culled;
    uint64_t total_aabbs_visible;
    double avg_cull_rate;  // Percentage of AABBs culled
} bvh_simd_stats_t;

void bvh_simd_reset_stats(void);
void bvh_simd_get_stats(bvh_simd_stats_t* out_stats);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_BVH_SIMD_H */
