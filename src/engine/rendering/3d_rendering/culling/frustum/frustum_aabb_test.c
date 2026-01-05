/*
 * frustum_aabb_test.c
 * AABB vs Frustum intersection tests
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#include "frustum_aabb_test.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * INTERSECTION TESTS
 * ============================================================================ */

/*
 * Tests if an AABB is visible within a frustum using the p-vertex/n-vertex approach.
 * Returns true if the AABB is inside or intersects the frustum.
 * Returns false if the AABB is completely outside.
 */
bool frustum_aabb_intersect(const vec4_t* planes, const aabb_t* aabb) {
    // Iterate over all 6 planes
    for (int i = 0; i < 6; i++) {
        vec3_t plane_normal = {planes[i].x, planes[i].y, planes[i].z};
        float plane_dist = planes[i].w;

        // Find the "positive" vertex (p-vertex)
        // This is the vertex of the AABB furthest in the direction of the plane normal
        vec3_t p_vertex;
        p_vertex.x = (plane_normal.x >= 0.0f) ? aabb->max.x : aabb->min.x;
        p_vertex.y = (plane_normal.y >= 0.0f) ? aabb->max.y : aabb->min.y;
        p_vertex.z = (plane_normal.z >= 0.0f) ? aabb->max.z : aabb->min.z;

        // If the p-vertex is behind the plane, the completely box is outside
        // Plane equation: dot(n, p) + d = distance
        // If distance < 0, point is behind plane
        if (vec3_dot(plane_normal, p_vertex) + plane_dist < 0.0f) {
            return false;
        }
    }

    return true;
}

/*
 * Intersection test returning the state: INSIDE, INTERSECT, OUTSIDE
 * Useful for hierarchical culling optimizations.
 */
int frustum_aabb_intersect_state(const vec4_t* planes, const aabb_t* aabb) {
    int result = 2; // Start assuming INSIDE

    for (int i = 0; i < 6; i++) {
        vec3_t plane_normal = {planes[i].x, planes[i].y, planes[i].z};
        float plane_dist = planes[i].w;

        vec3_t p_vertex; // Positive vertex
        vec3_t n_vertex; // Negative vertex

        p_vertex.x = (plane_normal.x >= 0.0f) ? aabb->max.x : aabb->min.x;
        p_vertex.y = (plane_normal.y >= 0.0f) ? aabb->max.y : aabb->min.y;
        p_vertex.z = (plane_normal.z >= 0.0f) ? aabb->max.z : aabb->min.z;

        n_vertex.x = (plane_normal.x >= 0.0f) ? aabb->min.x : aabb->max.x;
        n_vertex.y = (plane_normal.y >= 0.0f) ? aabb->min.y : aabb->max.y;
        n_vertex.z = (plane_normal.z >= 0.0f) ? aabb->min.z : aabb->max.z;

        if (vec3_dot(plane_normal, p_vertex) + plane_dist < 0.0f) {
            return 0; // OUTSIDE
        }

        if (vec3_dot(plane_normal, n_vertex) + plane_dist < 0.0f) {
            result = 1; // INTERSECT
        }
    }

    return result; // 2=INSIDE, 1=INTERSECT
}

/*
 * Updates the visibility buffer for a list of AABBs
 */
void frustum_aabb_cull_list(
    const vec4_t* planes, 
    const aabb_t* aabbs, 
    uint8_t* visibility, 
    uint32_t count) 
{
    // Scalar implementation loop
    // Note: SIMD version exists in simd_frustum_cull.c
    for (uint32_t i = 0; i < count; i++) {
        visibility[i] = frustum_aabb_intersect(planes, &aabbs[i]) ? 1 : 0;
    }
}
