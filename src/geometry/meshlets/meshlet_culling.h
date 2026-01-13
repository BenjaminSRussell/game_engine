/*
 * meshlet_culling.h
 * Visibility testing (frustum and cone culling) for meshlets
 */

#ifndef MESHLET_CULLING_H
#define MESHLET_CULLING_H

#include "geometry/meshlets/meshlet_builder.h"

// Culling result
typedef enum culling_result {
    CULL_VISIBLE,
    CULL_INVISIBLE
} culling_result_t;

// Cone culling (as requested in key patterns)
bool cull_meshlet_cone(meshlet_t m, vec3_t view_pos);

// Frustum culling (sphere vs planes)
bool cull_meshlet_frustum(meshlet_t m, const float frustum_planes[6][4]);

#endif // MESHLET_CULLING_H
