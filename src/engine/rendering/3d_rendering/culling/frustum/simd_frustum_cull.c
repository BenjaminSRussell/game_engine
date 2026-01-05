/*
 * simd_frustum_cull.c
 * SIMD-optimized frustum culling (4 AABBs at once)
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#include "simd_frustum_cull.h"
#include "../../math/vec3.h"
#include "../../math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#if defined(__x86_64__) || defined(_M_X64)
    #define SIMD_SSE
    #include <immintrin.h>
#elif defined(__arm64__) || defined(__aarch64__)
    #define SIMD_NEON
    #include <arm_neon.h>
#endif

/* ============================================================================
 * SIMD FRUSTUM CULLING
 * ============================================================================ */

#ifdef SIMD_SSE

// Test 4 AABBs against one frustum plane (SSE)
static inline __m128 test_plane_sse(
    __m128 plane_x, __m128 plane_y, __m128 plane_z, __m128 plane_w,
    __m128 min_x, __m128 min_y, __m128 min_z,
    __m128 max_x, __m128 max_y, __m128 max_z
) {
    // Find positive vertex (furthest in direction of plane normal)
    __m128 px = _mm_blendv_ps(min_x, max_x, plane_x);
    __m128 py = _mm_blendv_ps(min_y, max_y, plane_y);
    __m128 pz = _mm_blendv_ps(min_z, max_z, plane_z);
    
    // Dot product: plane.xyz * p_vertex + plane.w
    __m128 dot = _mm_mul_ps(plane_x, px);
    dot = _mm_add_ps(dot, _mm_mul_ps(plane_y, py));
    dot = _mm_add_ps(dot, _mm_mul_ps(plane_z, pz));
    dot = _mm_add_ps(dot, plane_w);
    
    // Return mask: dot >= 0 means inside
    return _mm_cmpge_ps(dot, _mm_setzero_ps());
}

int culling_simd_frustum_cull_batch_sse(
    const vec4_t* frustum_planes,  // 6 planes
    const aabb_t* aabbs,           // Input AABBs
    uint32_t aabb_count,
    uint32_t* out_visible_indices,
    uint32_t max_visible
) {
    uint32_t visible_count = 0;
    
    // Process 4 AABBs at a time
    for (uint32_t i = 0; i + 3 < aabb_count; i += 4) {
        // Load 4 AABBs
        __m128 min_x = _mm_setr_ps(aabbs[i+0].min.x, aabbs[i+1].min.x, aabbs[i+2].min.x, aabbs[i+3].min.x);
        __m128 min_y = _mm_setr_ps(aabbs[i+0].min.y, aabbs[i+1].min.y, aabbs[i+2].min.y, aabbs[i+3].min.y);
        __m128 min_z = _mm_setr_ps(aabbs[i+0].min.z, aabbs[i+1].min.z, aabbs[i+2].min.z, aabbs[i+3].min.z);
        __m128 max_x = _mm_setr_ps(aabbs[i+0].max.x, aabbs[i+1].max.x, aabbs[i+2].max.x, aabbs[i+3].max.x);
        __m128 max_y = _mm_setr_ps(aabbs[i+0].max.y, aabbs[i+1].max.y, aabbs[i+2].max.y, aabbs[i+3].max.y);
        __m128 max_z = _mm_setr_ps(aabbs[i+0].max.z, aabbs[i+1].max.z, aabbs[i+2].max.z, aabbs[i+3].max.z);
        
        __m128 inside_mask = _mm_castsi128_ps(_mm_set1_epi32(0xFFFFFFFF));
        
        // Test against all 6 planes
        for (int p = 0; p < 6; p++) {
            __m128 plane_x = _mm_set1_ps(frustum_planes[p].x);
            __m128 plane_y = _mm_set1_ps(frustum_planes[p].y);
            __m128 plane_z = _mm_set1_ps(frustum_planes[p].z);
            __m128 plane_w = _mm_set1_ps(frustum_planes[p].w);
            
            __m128 plane_result = test_plane_sse(
                plane_x, plane_y, plane_z, plane_w,
                min_x, min_y, min_z,
                max_x, max_y, max_z
            );
            
            inside_mask = _mm_and_ps(inside_mask, plane_result);
        }
        
        // Extract results
        int mask = _mm_movemask_ps(inside_mask);
        for (int j = 0; j < 4 && visible_count < max_visible; j++) {
            if (mask & (1 << j)) {
                out_visible_indices[visible_count++] = i + j;
            }
        }
    }
    
    // Handle remaining AABBs (scalar fallback)
    uint32_t remainder_start = (aabb_count / 4) * 4;
    for (uint32_t i = remainder_start; i < aabb_count && visible_count < max_visible; i++) {
        bool visible = true;
        for (int p = 0; p < 6 && visible; p++) {
            vec3_t plane_n = {frustum_planes[p].x, frustum_planes[p].y, frustum_planes[p].z};
            float plane_d = frustum_planes[p].w;
            
            vec3_t p_vertex;
            p_vertex.x = (plane_n.x >= 0.0f) ? aabbs[i].max.x : aabbs[i].min.x;
            p_vertex.y = (plane_n.y >= 0.0f) ? aabbs[i].max.y : aabbs[i].min.y;
            p_vertex.z = (plane_n.z >= 0.0f) ? aabbs[i].max.z : aabbs[i].min.z;
            
            if (vec3_dot(plane_n, p_vertex) + plane_d < 0.0f) {
                visible = false;
            }
        }
        
        if (visible) {
            out_visible_indices[visible_count++] = i;
        }
    }
    
    return visible_count;
}

#elif defined(SIMD_NEON)

// NEON implementation (ARM)
int culling_simd_frustum_cull_batch_neon(
    const vec4_t* frustum_planes,
    const aabb_t* aabbs,
    uint32_t aabb_count,
    uint32_t* out_visible_indices,
    uint32_t max_visible
) {
    uint32_t visible_count = 0;
    
    // Process 4 AABBs at a time with NEON
    for (uint32_t i = 0; i + 3 < aabb_count; i += 4) {
        float32x4_t min_x = {aabbs[i+0].min.x, aabbs[i+1].min.x, aabbs[i+2].min.x, aabbs[i+3].min.x};
        float32x4_t min_y = {aabbs[i+0].min.y, aabbs[i+1].min.y, aabbs[i+2].min.y, aabbs[i+3].min.y};
        float32x4_t min_z = {aabbs[i+0].min.z, aabbs[i+1].min.z, aabbs[i+2].min.z, aabbs[i+3].min.z};
        float32x4_t max_x = {aabbs[i+0].max.x, aabbs[i+1].max.x, aabbs[i+2].max.x, aabbs[i+3].max.x};
        float32x4_t max_y = {aabbs[i+0].max.y, aabbs[i+1].max.y, aabbs[i+2].max.y, aabbs[i+3].max.y};
        float32x4_t max_z = {aabbs[i+0].max.z, aabbs[i+1].max.z, aabbs[i+2].max.z, aabbs[i+3].max.z};
        
        uint32x4_t inside_mask = vdupq_n_u32(0xFFFFFFFF);
        
        for (int p = 0; p < 6; p++) {
            float32x4_t plane_x = vdupq_n_f32(frustum_planes[p].x);
            float32x4_t plane_y = vdupq_n_f32(frustum_planes[p].y);
            float32x4_t plane_z = vdupq_n_f32(frustum_planes[p].z);
            float32x4_t plane_w = vdupq_n_f32(frustum_planes[p].w);
            
            // Select positive vertex
            float32x4_t px = vbslq_f32(vcgeq_f32(plane_x, vdupq_n_f32(0)), max_x, min_x);
            float32x4_t py = vbslq_f32(vcgeq_f32(plane_y, vdupq_n_f32(0)), max_y, min_y);
            float32x4_t pz = vbslq_f32(vcgeq_f32(plane_z, vdupq_n_f32(0)), max_z, min_z);
            
            // Dot product
            float32x4_t dot = vmulq_f32(plane_x, px);
            dot = vmlaq_f32(dot, plane_y, py);
            dot = vmlaq_f32(dot, plane_z, pz);
            dot = vaddq_f32(dot, plane_w);
            
            uint32x4_t plane_result = vcgeq_f32(dot, vdupq_n_f32(0));
            inside_mask = vandq_u32(inside_mask, plane_result);
        }
        
        // Extract results
        uint32_t results[4];
        vst1q_u32(results, inside_mask);
        for (int j = 0; j < 4 && visible_count < max_visible; j++) {
            if (results[j]) {
                out_visible_indices[visible_count++] = i + j;
            }
        }
    }
    
    // Scalar fallback for remainder
    uint32_t remainder_start = (aabb_count / 4) * 4;
    for (uint32_t i = remainder_start; i < aabb_count && visible_count < max_visible; i++) {
        bool visible = true;
        for (int p = 0; p < 6 && visible; p++) {
            vec3_t plane_n = {frustum_planes[p].x, frustum_planes[p].y, frustum_planes[p].z};
            float plane_d = frustum_planes[p].w;
            
            vec3_t p_vertex;
            p_vertex.x = (plane_n.x >= 0.0f) ? aabbs[i].max.x : aabbs[i].min.x;
            p_vertex.y = (plane_n.y >= 0.0f) ? aabbs[i].max.y : aabbs[i].min.y;
            p_vertex.z = (plane_n.z >= 0.0f) ? aabbs[i].max.z : aabbs[i].min.z;
            
            if (vec3_dot(plane_n, p_vertex) + plane_d < 0.0f) {
                visible = false;
            }
        }
        
        if (visible) {
            out_visible_indices[visible_count++] = i;
        }
    }
    
    return visible_count;
}

#endif

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int culling_simd_frustum_cull_batch(
    const vec4_t* frustum_planes,
    const aabb_t* aabbs,
    uint32_t aabb_count,
    uint32_t* out_visible_indices,
    uint32_t max_visible
) {
    if (!frustum_planes || !aabbs || !out_visible_indices || aabb_count == 0) {
        return 0;
    }
    
#ifdef SIMD_SSE
    return culling_simd_frustum_cull_batch_sse(frustum_planes, aabbs, aabb_count,
                                                out_visible_indices, max_visible);
#elif defined(SIMD_NEON)
    return culling_simd_frustum_cull_batch_neon(frustum_planes, aabbs, aabb_count,
                                                 out_visible_indices, max_visible);
#else
    // Scalar fallback
    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < aabb_count && visible_count < max_visible; i++) {
        bool visible = true;
        for (int p = 0; p < 6 && visible; p++) {
            vec3_t plane_n = {frustum_planes[p].x, frustum_planes[p].y, frustum_planes[p].z};
            float plane_d = frustum_planes[p].w;
            
            vec3_t p_vertex;
            p_vertex.x = (plane_n.x >= 0.0f) ? aabbs[i].max.x : aabbs[i].min.x;
            p_vertex.y = (plane_n.y >= 0.0f) ? aabbs[i].max.y : aabbs[i].min.y;
            p_vertex.z = (plane_n.z >= 0.0f) ? aabbs[i].max.z : aabbs[i].min.z;
            
            if (vec3_dot(plane_n, p_vertex) + plane_d < 0.0f) {
                visible = false;
            }
        }
        
        if (visible) {
            out_visible_indices[visible_count++] = i;
        }
    }
    return visible_count;
#endif
}

/* End of simd_frustum_cull.c */
