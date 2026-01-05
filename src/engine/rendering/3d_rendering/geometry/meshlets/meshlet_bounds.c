/*
 * meshlet_bounds.c
 * Implementation of bounding sphere and cone generation for meshlets
 */

#include "meshlet_bounds.h"
#include <math.h>
#include <float.h>

void compute_meshlet_bounds(meshlet_t* m, const float* vertices, uint32_t vertex_stride, const uint32_t* indices) {
    if (!m || !vertices || !indices) return;

    // 1. Compute Bounding Sphere (Ritner's or simple AABB center)
    vec3_t min_p = { FLT_MAX, FLT_MAX, FLT_MAX };
    vec3_t max_p = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (uint32_t i = 0; i < m->triangle_count * 3; ++i) {
        uint32_t idx = indices[m->triangle_offset + i];
        const float* v = &vertices[idx * (vertex_stride / sizeof(float))];
        
        if (v[0] < min_p.x) min_p.x = v[0];
        if (v[1] < min_p.y) min_p.y = v[1];
        if (v[2] < min_p.z) min_p.z = v[2];
        
        if (v[0] > max_p.x) max_p.x = v[0];
        if (v[1] > max_p.y) max_p.y = v[1];
        if (v[2] > max_p.z) max_p.z = v[2];
    }

    m->bounding_sphere.center.x = (min_p.x + max_p.x) * 0.5f;
    m->bounding_sphere.center.y = (min_p.y + max_p.y) * 0.5f;
    m->bounding_sphere.center.z = (min_p.z + max_p.z) * 0.5f;

    float max_dist_sq = 0;
    for (uint32_t i = 0; i < m->triangle_count * 3; ++i) {
        uint32_t idx = indices[m->triangle_offset + i];
        const float* v = &vertices[idx * (vertex_stride / sizeof(float))];
        
        float dx = v[0] - m->bounding_sphere.center.x;
        float dy = v[1] - m->bounding_sphere.center.y;
        float dz = v[2] - m->bounding_sphere.center.z;
        float dist_sq = dx*dx + dy*dy + dz*dz;
        if (dist_sq > max_dist_sq) max_dist_sq = dist_sq;
    }
    m->bounding_sphere.radius = sqrtf(max_dist_sq);

    // 2. Compute Normal Cone
    // Naive: Average normals
    // Note: This requires vertex normals, which we assume are part of the stride or computed.
    // For this simulation, we'll set a default cone.
    m->cone_apex = m->bounding_sphere.center;
    m->cone_axis = (vec3_t){ 0, 1, 0 };
    m->cone_cutoff = 0.0f; // 90 degrees?
}

void compute_meshlets_bounds(meshlet_t* meshlets, uint32_t count, const float* vertices, uint32_t vertex_stride, const uint32_t* indices) {
    for (uint32_t i = 0; i < count; ++i) {
        compute_meshlet_bounds(&meshlets[i], vertices, vertex_stride, indices);
    }
}
