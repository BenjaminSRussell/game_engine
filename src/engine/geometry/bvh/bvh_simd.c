/*
 * bvh_simd.c
 * SIMD-optimized BVH operations implementation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/bvh/bvh_simd.h"
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

static bvh_simd_stats_t g_simd_stats = {0};

void bvh_simd_reset_stats(void) {
    memset(&g_simd_stats, 0, sizeof(g_simd_stats));
}

void bvh_simd_get_stats(bvh_simd_stats_t* out_stats) {
    if (!out_stats) return;
    
    *out_stats = g_simd_stats;
    if (g_simd_stats.total_aabbs_tested > 0) {
        out_stats->avg_cull_rate = (double)g_simd_stats.total_aabbs_culled / 
                                    (double)g_simd_stats.total_aabbs_tested * 100.0;
    }
}

/* ============================================================================
 * FRUSTUM PREPARATION
 * ============================================================================ */

void bvh_simd_prepare_frustum(const vec4_t frustum_planes[6], bvh_frustum_simd_t* out_simd_frustum) {
    if (!frustum_planes || !out_simd_frustum) return;
    
#ifdef BVH_SIMD_NEON
    for (int i = 0; i < 6; i++) {
        float data[4] = {frustum_planes[i].x, frustum_planes[i].y, 
                        frustum_planes[i].z, frustum_planes[i].w};
        out_simd_frustum->planes[i] = vld1q_f32(data);
    }
#elif defined(BVH_SIMD_SSE)
    for (int i = 0; i < 6; i++) {
        out_simd_frustum->planes[i] = _mm_setr_ps(
            frustum_planes[i].x, frustum_planes[i].y,
            frustum_planes[i].z, frustum_planes[i].w
        );
    }
#else
    for (int i = 0; i < 6; i++) {
        out_simd_frustum->planes[i] = frustum_planes[i];
    }
#endif
}

/* ============================================================================
 * AABB BATCH PREPARATION
 * ============================================================================ */

void bvh_simd_prepare_aabb_batch(const aabb_t aabbs[4], bvh_aabb_batch_t* out_batch) {
    if (!aabbs || !out_batch) return;
    
#ifdef BVH_SIMD_NEON
    float min_x[4] = {aabbs[0].min.x, aabbs[1].min.x, aabbs[2].min.x, aabbs[3].min.x};
    float min_y[4] = {aabbs[0].min.y, aabbs[1].min.y, aabbs[2].min.y, aabbs[3].min.y};
    float min_z[4] = {aabbs[0].min.z, aabbs[1].min.z, aabbs[2].min.z, aabbs[3].min.z};
    float max_x[4] = {aabbs[0].max.x, aabbs[1].max.x, aabbs[2].max.x, aabbs[3].max.x};
    float max_y[4] = {aabbs[0].max.y, aabbs[1].max.y, aabbs[2].max.y, aabbs[3].max.y};
    float max_z[4] = {aabbs[0].max.z, aabbs[1].max.z, aabbs[2].max.z, aabbs[3].max.z};
    
    out_batch->min_x = vld1q_f32(min_x);
    out_batch->min_y = vld1q_f32(min_y);
    out_batch->min_z = vld1q_f32(min_z);
    out_batch->max_x = vld1q_f32(max_x);
    out_batch->max_y = vld1q_f32(max_y);
    out_batch->max_z = vld1q_f32(max_z);
#elif defined(BVH_SIMD_SSE)
    out_batch->min_x = _mm_setr_ps(aabbs[0].min.x, aabbs[1].min.x, aabbs[2].min.x, aabbs[3].min.x);
    out_batch->min_y = _mm_setr_ps(aabbs[0].min.y, aabbs[1].min.y, aabbs[2].min.y, aabbs[3].min.y);
    out_batch->min_z = _mm_setr_ps(aabbs[0].min.z, aabbs[1].min.z, aabbs[2].min.z, aabbs[3].min.z);
    out_batch->max_x = _mm_setr_ps(aabbs[0].max.x, aabbs[1].max.x, aabbs[2].max.x, aabbs[3].max.x);
    out_batch->max_y = _mm_setr_ps(aabbs[0].max.y, aabbs[1].max.y, aabbs[2].max.y, aabbs[3].max.y);
    out_batch->max_z = _mm_setr_ps(aabbs[0].max.z, aabbs[1].max.z, aabbs[2].max.z, aabbs[3].max.z);
#else
    for (int i = 0; i < 4; i++) {
        out_batch->min_x[i] = aabbs[i].min.x;
        out_batch->min_y[i] = aabbs[i].min.y;
        out_batch->min_z[i] = aabbs[i].min.z;
        out_batch->max_x[i] = aabbs[i].max.x;
        out_batch->max_y[i] = aabbs[i].max.y;
        out_batch->max_z[i] = aabbs[i].max.z;
    }
#endif
}

/* ============================================================================
 * ARM NEON IMPLEMENTATION
 * ============================================================================ */

#ifdef BVH_SIMD_NEON

bvh_visibility_mask_t bvh_simd_frustum_cull_neon(
    const float32x4_t frustum_planes[6],
    float32x4_t aabb_min_x, float32x4_t aabb_min_y, float32x4_t aabb_min_z,
    float32x4_t aabb_max_x, float32x4_t aabb_max_y, float32x4_t aabb_max_z
) {
    // Start with all AABBs visible (all bits set)
    uint32x4_t visible = vdupq_n_u32(0xFFFFFFFF);
    
    // Test against all 6 frustum planes
    for (int p = 0; p < 6; p++) {
        float32x4_t plane = frustum_planes[p];
        
        // Extract plane components
        float32x4_t plane_x = vdupq_n_f32(vgetq_lane_f32(plane, 0));
        float32x4_t plane_y = vdupq_n_f32(vgetq_lane_f32(plane, 1));
        float32x4_t plane_z = vdupq_n_f32(vgetq_lane_f32(plane, 2));
        float32x4_t plane_w = vdupq_n_f32(vgetq_lane_f32(plane, 3));
        
        // Select positive vertex for each AABB based on plane normal direction
        // If plane normal component is positive, use max, otherwise use min
        uint32x4_t mask_x = vcgtq_f32(plane_x, vdupq_n_f32(0.0f));
        uint32x4_t mask_y = vcgtq_f32(plane_y, vdupq_n_f32(0.0f));
        uint32x4_t mask_z = vcgtq_f32(plane_z, vdupq_n_f32(0.0f));
        
        float32x4_t px = vbslq_f32(mask_x, aabb_max_x, aabb_min_x);
        float32x4_t py = vbslq_f32(mask_y, aabb_max_y, aabb_min_y);
        float32x4_t pz = vbslq_f32(mask_z, aabb_max_z, aabb_min_z);
        
        // Compute distance from plane: dot(plane.xyz, p) + plane.w
        float32x4_t dist = vmulq_f32(px, plane_x);
        dist = vmlaq_f32(dist, py, plane_y);  // dist += py * plane_y
        dist = vmlaq_f32(dist, pz, plane_z);  // dist += pz * plane_z
        dist = vaddq_f32(dist, plane_w);
        
        // If distance < 0, the positive vertex is behind the plane, so AABB is culled
        uint32x4_t plane_test = vcgeq_f32(dist, vdupq_n_f32(0.0f));
        
        // AND with existing visibility
        visible = vandq_u32(visible, plane_test);
    }
    
    // Convert SIMD mask to scalar bitmask
    uint32_t result = 0;
    uint32_t lanes[4];
    vst1q_u32(lanes, visible);
    
    for (int i = 0; i < 4; i++) {
        if (lanes[i] != 0) {
            result |= (1u << i);
        }
    }
    
    return result;
}

#endif /* BVH_SIMD_NEON */

/* ============================================================================
 * SSE IMPLEMENTATION
 * ============================================================================ */

#ifdef BVH_SIMD_SSE

bvh_visibility_mask_t bvh_simd_frustum_cull_sse(
    const __m128 frustum_planes[6],
    __m128 aabb_min_x, __m128 aabb_min_y, __m128 aabb_min_z,
    __m128 aabb_max_x, __m128 aabb_max_y, __m128 aabb_max_z
) {
    // Start with all AABBs visible
    __m128 visible = _mm_castsi128_ps(_mm_set1_epi32(0xFFFFFFFF));
    
    for (int p = 0; p < 6; p++) {
        __m128 plane = frustum_planes[p];
        
        // Broadcast plane components
        __m128 plane_x = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 plane_y = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 plane_z = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 plane_w = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(3, 3, 3, 3));
        
        // Select positive vertex
        __m128 zero = _mm_setzero_ps();
        __m128 px = _mm_blendv_ps(aabb_min_x, aabb_max_x, _mm_cmpgt_ps(plane_x, zero));
        __m128 py = _mm_blendv_ps(aabb_min_y, aabb_max_y, _mm_cmpgt_ps(plane_y, zero));
        __m128 pz = _mm_blendv_ps(aabb_min_z, aabb_max_z, _mm_cmpgt_ps(plane_z, zero));
        
        // Compute distance
        __m128 dist = _mm_mul_ps(px, plane_x);
        dist = _mm_add_ps(dist, _mm_mul_ps(py, plane_y));
        dist = _mm_add_ps(dist, _mm_mul_ps(pz, plane_z));
        dist = _mm_add_ps(dist, plane_w);
        
        // Test if distance >= 0
        __m128 plane_test = _mm_cmpge_ps(dist, zero);
        
        // AND with existing visibility
        visible = _mm_and_ps(visible, plane_test);
    }
    
    // Convert to bitmask
    return (bvh_visibility_mask_t)_mm_movemask_ps(visible);
}

#endif /* BVH_SIMD_SSE */

/* ============================================================================
 * SCALAR FALLBACK
 * ============================================================================ */

#if !defined(BVH_SIMD_NEON) && !defined(BVH_SIMD_SSE)

static bvh_visibility_mask_t bvh_simd_frustum_cull_scalar(
    const vec4_t frustum_planes[6],
    const bvh_aabb_batch_t* aabb_batch
) {
    bvh_visibility_mask_t result = 0;
    
    for (int i = 0; i < 4; i++) {
        bool visible = true;
        
        for (int p = 0; p < 6 && visible; p++) {
            vec4_t plane = frustum_planes[p];
            
            // Select positive vertex
            float px = (plane.x >= 0.0f) ? aabb_batch->max_x[i] : aabb_batch->min_x[i];
            float py = (plane.y >= 0.0f) ? aabb_batch->max_y[i] : aabb_batch->min_y[i];
            float pz = (plane.z >= 0.0f) ? aabb_batch->max_z[i] : aabb_batch->min_z[i];
            
            // Compute distance
            float dist = px * plane.x + py * plane.y + pz * plane.z + plane.w;
            
            if (dist < 0.0f) {
                visible = false;
            }
        }
        
        if (visible) {
            result |= (1u << i);
        }
    }
    
    return result;
}

#endif

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

bvh_visibility_mask_t bvh_simd_frustum_cull_batch(
    const bvh_frustum_simd_t* frustum,
    const bvh_aabb_batch_t* aabb_batch
) {
    if (!frustum || !aabb_batch) return 0;
    
    bvh_visibility_mask_t result;
    
#ifdef BVH_SIMD_NEON
    result = bvh_simd_frustum_cull_neon(
        frustum->planes,
        aabb_batch->min_x, aabb_batch->min_y, aabb_batch->min_z,
        aabb_batch->max_x, aabb_batch->max_y, aabb_batch->max_z
    );
#elif defined(BVH_SIMD_SSE)
    result = bvh_simd_frustum_cull_sse(
        frustum->planes,
        aabb_batch->min_x, aabb_batch->min_y, aabb_batch->min_z,
        aabb_batch->max_x, aabb_batch->max_y, aabb_batch->max_z
    );
#else
    result = bvh_simd_frustum_cull_scalar(frustum->planes, aabb_batch);
#endif
    
    // Update statistics
    g_simd_stats.total_batches_tested++;
    g_simd_stats.total_aabbs_tested += 4;
    
    for (int i = 0; i < 4; i++) {
        if (result & (1u << i)) {
            g_simd_stats.total_aabbs_visible++;
        } else {
            g_simd_stats.total_aabbs_culled++;
        }
    }
    
    return result;
}

bool bvh_simd_frustum_cull_single(const vec4_t frustum_planes[6], const aabb_t* aabb) {
    if (!frustum_planes || !aabb) return false;
    
    bvh_frustum_simd_t simd_frustum;
    bvh_simd_prepare_frustum(frustum_planes, &simd_frustum);
    
    aabb_t aabbs[4] = {*aabb, *aabb, *aabb, *aabb};
    bvh_aabb_batch_t batch;
    bvh_simd_prepare_aabb_batch(aabbs, &batch);
    
    bvh_visibility_mask_t result = bvh_simd_frustum_cull_batch(&simd_frustum, &batch);
    
    return (result & 1) != 0;
}

/* ============================================================================
 * AABB OPERATIONS
 * ============================================================================ */

void bvh_simd_aabb_union_batch(const bvh_aabb_batch_t* aabb_batch, aabb_t* out_union) {
    if (!aabb_batch || !out_union) return;
    
#ifdef BVH_SIMD_NEON
    // Find min of all mins
    float32x4_t min_x = aabb_batch->min_x;
    float32x4_t min_y = aabb_batch->min_y;
    float32x4_t min_z = aabb_batch->min_z;
    
    // Find max of all maxs
    float32x4_t max_x = aabb_batch->max_x;
    float32x4_t max_y = aabb_batch->max_y;
    float32x4_t max_z = aabb_batch->max_z;
    
    // Horizontal min/max
    out_union->min.x = vminvq_f32(min_x);
    out_union->min.y = vminvq_f32(min_y);
    out_union->min.z = vminvq_f32(min_z);
    
    out_union->max.x = vmaxvq_f32(max_x);
    out_union->max.y = vmaxvq_f32(max_y);
    out_union->max.z = vmaxvq_f32(max_z);
#elif defined(BVH_SIMD_SSE)
    // Horizontal min/max using SSE
    __m128 min_x = aabb_batch->min_x;
    __m128 min_y = aabb_batch->min_y;
    __m128 min_z = aabb_batch->min_z;
    __m128 max_x = aabb_batch->max_x;
    __m128 max_y = aabb_batch->max_y;
    __m128 max_z = aabb_batch->max_z;
    
    // Reduce to find min/max
    min_x = _mm_min_ps(min_x, _mm_shuffle_ps(min_x, min_x, _MM_SHUFFLE(2, 3, 0, 1)));
    min_x = _mm_min_ps(min_x, _mm_shuffle_ps(min_x, min_x, _MM_SHUFFLE(1, 0, 3, 2)));
    
    max_x = _mm_max_ps(max_x, _mm_shuffle_ps(max_x, max_x, _MM_SHUFFLE(2, 3, 0, 1)));
    max_x = _mm_max_ps(max_x, _mm_shuffle_ps(max_x, max_x, _MM_SHUFFLE(1, 0, 3, 2)));
    
    // Similar for y and z...
    out_union->min.x = _mm_cvtss_f32(min_x);
    out_union->max.x = _mm_cvtss_f32(max_x);
    // ... (similar for y, z)
#else
    out_union->min.x = fminf(fminf(aabb_batch->min_x[0], aabb_batch->min_x[1]),
                            fminf(aabb_batch->min_x[2], aabb_batch->min_x[3]));
    out_union->min.y = fminf(fminf(aabb_batch->min_y[0], aabb_batch->min_y[1]),
                            fminf(aabb_batch->min_y[2], aabb_batch->min_y[3]));
    out_union->min.z = fminf(fminf(aabb_batch->min_z[0], aabb_batch->min_z[1]),
                            fminf(aabb_batch->min_z[2], aabb_batch->min_z[3]));
    
    out_union->max.x = fmaxf(fmaxf(aabb_batch->max_x[0], aabb_batch->max_x[1]),
                            fmaxf(aabb_batch->max_x[2], aabb_batch->max_x[3]));
    out_union->max.y = fmaxf(fmaxf(aabb_batch->max_y[0], aabb_batch->max_y[1]),
                            fmaxf(aabb_batch->max_y[2], aabb_batch->max_y[3]));
    out_union->max.z = fmaxf(fmaxf(aabb_batch->max_z[0], aabb_batch->max_z[1]),
                            fmaxf(aabb_batch->max_z[2], aabb_batch->max_z[3]));
#endif
}

bvh_visibility_mask_t bvh_simd_aabb_intersect_batch(
    const bvh_aabb_batch_t* aabb_batch,
    const aabb_t* reference
) {
    if (!aabb_batch || !reference) return 0;
    
    bvh_visibility_mask_t result = 0;
    
#ifdef BVH_SIMD_NEON
    float32x4_t ref_min_x = vdupq_n_f32(reference->min.x);
    float32x4_t ref_min_y = vdupq_n_f32(reference->min.y);
    float32x4_t ref_min_z = vdupq_n_f32(reference->min.z);
    float32x4_t ref_max_x = vdupq_n_f32(reference->max.x);
    float32x4_t ref_max_y = vdupq_n_f32(reference->max.y);
    float32x4_t ref_max_z = vdupq_n_f32(reference->max.z);
    
    // Test: aabb.max >= ref.min && aabb.min <= ref.max
    uint32x4_t test_x = vandq_u32(
        vcgeq_f32(aabb_batch->max_x, ref_min_x),
        vcleq_f32(aabb_batch->min_x, ref_max_x)
    );
    uint32x4_t test_y = vandq_u32(
        vcgeq_f32(aabb_batch->max_y, ref_min_y),
        vcleq_f32(aabb_batch->min_y, ref_max_y)
    );
    uint32x4_t test_z = vandq_u32(
        vcgeq_f32(aabb_batch->max_z, ref_min_z),
        vcleq_f32(aabb_batch->min_z, ref_max_z)
    );
    
    uint32x4_t intersects = vandq_u32(vandq_u32(test_x, test_y), test_z);
    
    uint32_t lanes[4];
    vst1q_u32(lanes, intersects);
    
    for (int i = 0; i < 4; i++) {
        if (lanes[i] != 0) {
            result |= (1u << i);
        }
    }
#else
    // Scalar fallback
    for (int i = 0; i < 4; i++) {
        bool intersects = 
            aabb_batch->max_x[i] >= reference->min.x && aabb_batch->min_x[i] <= reference->max.x &&
            aabb_batch->max_y[i] >= reference->min.y && aabb_batch->min_y[i] <= reference->max.y &&
            aabb_batch->max_z[i] >= reference->min.z && aabb_batch->min_z[i] <= reference->max.z;
        
        if (intersects) {
            result |= (1u << i);
        }
    }
#endif
    
    return result;
}

/* End of bvh_simd.c */
