/*
 * cluster_lod.c
 * Implementation of cluster LOD selection
 */

#include "geometry/nanite/nanite/cluster/cluster_lod.h"
#include <include/math/math.h>

float compute_cluster_projected_error(const cluster_t* c, vec3_t view_pos, float view_fov, float screen_height) {
    if (!c) return 0.0f;

    // Projected error = (world_error * screen_height) / (2 * tan(fov/2) * distance)
    float dx = c->bounds.center.x - view_pos.x;
    float dy = c->bounds.center.y - view_pos.y;
    float dz = c->bounds.center.z - view_pos.z;
    float distance = sqrtf(dx*dx + dy*dy + dz*dz) - c->bounds.radius;
    
    if (distance <= 0.001f) distance = 0.001f;

    float projected_error = (c->error * screen_height) / (2.0f * tanf(view_fov * 0.5f) * distance);
    return projected_error;
}

cluster_lod_status_t evaluate_cluster_lod(const cluster_t* c, float threshold, vec3_t view_pos, float view_fov, float screen_height) {
    float projected_error = compute_cluster_projected_error(c, view_pos, view_fov, screen_height);

    if (projected_error <= threshold) {
        return CLUSTER_LOD_SELECT;
    } else {
        return CLUSTER_LOD_REFINE;
    }
}
