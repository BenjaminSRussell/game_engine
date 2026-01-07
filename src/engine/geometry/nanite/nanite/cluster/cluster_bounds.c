/*
 * cluster_bounds.c
 * Implementation of cluster bounding volume generation
 */

#include "geometry/nanite/nanite/cluster/cluster_bounds.h"
#include <include/math/math.h>

static float dist_sq(vec3_t a, vec3_t b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}

void compute_cluster_bounds(cluster_t* c, const cluster_t* all_clusters) {
    if (!c || !all_clusters) return;

    // If leaf (no children), bounds already based on meshlets
    if (c->child_clusters[0] == 0xFFFFFFFF) return;

    // Compound bounds from children
    // Simple average center initially, then find max radius
    vec3_t center = {0, 0, 0};
    uint32_t valid_children = 0;
    for (int i = 0; i < 8; ++i) {
        uint32_t idx = c->child_clusters[i];
        if (idx != 0xFFFFFFFF) {
            center.x += all_clusters[idx].bounds.center.x;
            center.y += all_clusters[idx].bounds.center.y;
            center.z += all_clusters[idx].bounds.center.z;
            valid_children++;
        }
    }

    if (valid_children > 0) {
        center.x /= valid_children;
        center.y /= valid_children;
        center.z /= valid_children;
        c->bounds.center = center;

        float max_radius = 0;
        for (int i = 0; i < 8; ++i) {
            uint32_t idx = c->child_clusters[i];
            if (idx != 0xFFFFFFFF) {
                float d = sqrtf(dist_sq(center, all_clusters[idx].bounds.center));
                float r = d + all_clusters[idx].bounds.radius;
                if (r > max_radius) max_radius = r;
            }
        }
        c->bounds.radius = max_radius;
    }
}
