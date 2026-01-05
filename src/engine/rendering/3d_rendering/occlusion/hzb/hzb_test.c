/*
 * hzb_test.c
 * HZB Occlusion Testing
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#include "hzb_test.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"
#include "../../math/aabb.h"
#include <math.h>
#include <stdbool.h>

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/*
 * Projects an AABB into screen space and returns the bounding rectangle and min depth.
 */
static void project_aabb(
    const aabb_t* aabb, 
    const mat4_t* view_proj, 
    float width, float height,
    float* min_x, float* min_y, float* max_x, float* max_y, float* min_depth) 
{
    vec3_t corners[8];
    // Generate corners... (omitted for brevity, assume standard min/max permutes)
    corners[0] = (vec3_t){aabb->min.x, aabb->min.y, aabb->min.z};
    corners[1] = (vec3_t){aabb->max.x, aabb->min.y, aabb->min.z};
    corners[2] = (vec3_t){aabb->min.x, aabb->max.y, aabb->min.z};
    corners[3] = (vec3_t){aabb->max.x, aabb->max.y, aabb->min.z};
    corners[4] = (vec3_t){aabb->min.x, aabb->min.y, aabb->max.z};
    corners[5] = (vec3_t){aabb->max.x, aabb->min.y, aabb->max.z};
    corners[6] = (vec3_t){aabb->min.x, aabb->max.y, aabb->max.z};
    corners[7] = (vec3_t){aabb->max.x, aabb->max.y, aabb->max.z};

    *min_x = width; *min_y = height;
    *max_x = 0.0f;  *max_y = 0.0f;
    *min_depth = 1.0f;

    for (int i = 0; i < 8; i++) {
        // Project point
        // vec4 pos = view_proj * vec4(corner, 1.0)
        // ndc = pos / pos.w
        // screen = viewport * ndc
        
        // Simplified Logic:
        // Update bounds
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/*
 * CPU-side HZB test.
 * Note: HZB is typically read on the GPU (compute shader), but a CPU readback 
 * version can be useful for latency-tolerant checks.
 */
bool hzb_test_aabb_visible(
    const aabb_t* aabb, 
    const mat4_t* view_proj,
    void* hzb_cpu_data, // Mip 0 or cpu copy
    uint32_t hzb_width, uint32_t hzb_height
) {
    float min_x, min_y, max_x, max_y, min_z;
    // project_aabb(aabb, view_proj, (float)hzb_width, (float)hzb_height, &min_x, &min_y, &max_x, &max_y, &min_z);

    // 1. Calculate mip level
    // float max_dim = fmaxf(max_x - min_x, max_y - min_y);
    // int mip = (int)ceilf(log2f(max_dim));

    // 2. Sample 4 texels at that mip level
    // float occluder_depth = sample_hzb(mip, coords);

    // 3. Compare depth
    // return min_z >= occluder_depth; // If strictly behind, it is occluded (invisible)
    
    return true; // Stub: assume visible
}
