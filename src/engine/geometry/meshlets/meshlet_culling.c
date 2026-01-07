/*
 * meshlet_culling.c
 * Implementation of meshlet visibility testing
 */

#include "geometry/meshlets/meshlet_culling.h"
#include <include/math/math.h>

// Helper: vec3 normalize
static vec3_t normalize_vec3(vec3_t v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 0) {
        float inv = 1.0f / len;
        return (vec3_t){ v.x * inv, v.y * inv, v.z * inv };
    }
    return v;
}

// Helper: vec3 dot
static float dot_vec3(vec3_t a, vec3_t b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

bool cull_meshlet_cone(meshlet_t m, vec3_t view_pos) {
    // Backface cone culling
    vec3_t to_apex = {
        m.cone_apex.x - view_pos.x,
        m.cone_apex.y - view_pos.y,
        m.cone_apex.z - view_pos.z
    };
    to_apex = normalize_vec3(to_apex);
    
    // If the dot product is less than the cutoff, it's backfacing
    if (dot_vec3(to_apex, m.cone_axis) < m.cone_cutoff) {
        return true; // Culled
    }
    return false; // Visible
}

bool cull_meshlet_frustum(meshlet_t m, const float frustum_planes[6][4]) {
    // Sphere vs Frustum planes
    for (int i = 0; i < 6; ++i) {
        float distance = frustum_planes[i][0] * m.bounding_sphere.center.x +
                         frustum_planes[i][1] * m.bounding_sphere.center.y +
                         frustum_planes[i][2] * m.bounding_sphere.center.z +
                         frustum_planes[i][3];
        
        if (distance < -m.bounding_sphere.radius) {
            return true; // Outside this plane
        }
    }
    return false; // Partially or fully inside
}
