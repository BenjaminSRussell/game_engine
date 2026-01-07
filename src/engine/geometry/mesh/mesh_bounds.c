/*
 * mesh_bounds.c
 * AABB and bounding sphere calculation utilities
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 *
 * Provides utilities for:
 * - AABB computation and manipulation
 * - Bounding sphere computation (Ritter's algorithm)
 * - Intersection tests
 * - Bounds merging for hierarchical structures
 */

#include "mesh_bounds.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

/* ============================================================================
 * AABB OPERATIONS
 * ============================================================================ */

int geometry_bounds_compute_aabb(
    const void* vertex_data,
    uint32_t vertex_count,
    uint32_t vertex_stride,
    uint32_t position_offset,
    aabb_t* out_aabb)
{
    if (!vertex_data || vertex_count == 0 || !out_aabb) return -1;

    const uint8_t* vert_ptr = (const uint8_t*)vertex_data + position_offset;
    const float* first_pos = (const float*)vert_ptr;

    float min_x = first_pos[0];
    float min_y = first_pos[1];
    float min_z = first_pos[2];
    float max_x = min_x;
    float max_y = min_y;
    float max_z = min_z;

    // Iterate through all vertices
    for (uint32_t i = 1; i < vertex_count; i++) {
        vert_ptr += vertex_stride;
        const float* pos = (const float*)vert_ptr;

        if (pos[0] < min_x) min_x = pos[0];
        if (pos[0] > max_x) max_x = pos[0];
        if (pos[1] < min_y) min_y = pos[1];
        if (pos[1] > max_y) max_y = pos[1];
        if (pos[2] < min_z) min_z = pos[2];
        if (pos[2] > max_z) max_z = pos[2];
    }

    out_aabb->min_x = min_x;
    out_aabb->min_y = min_y;
    out_aabb->min_z = min_z;
    out_aabb->max_x = max_x;
    out_aabb->max_y = max_y;
    out_aabb->max_z = max_z;

    return 0;
}

int geometry_bounds_merge_aabbs(const aabb_t* aabbs, uint32_t count, aabb_t* out_merged)
{
    if (!aabbs || count == 0 || !out_merged) return -1;

    out_merged->min_x = aabbs[0].min_x;
    out_merged->min_y = aabbs[0].min_y;
    out_merged->min_z = aabbs[0].min_z;
    out_merged->max_x = aabbs[0].max_x;
    out_merged->max_y = aabbs[0].max_y;
    out_merged->max_z = aabbs[0].max_z;

    for (uint32_t i = 1; i < count; i++) {
        if (aabbs[i].min_x < out_merged->min_x) out_merged->min_x = aabbs[i].min_x;
        if (aabbs[i].min_y < out_merged->min_y) out_merged->min_y = aabbs[i].min_y;
        if (aabbs[i].min_z < out_merged->min_z) out_merged->min_z = aabbs[i].min_z;
        if (aabbs[i].max_x > out_merged->max_x) out_merged->max_x = aabbs[i].max_x;
        if (aabbs[i].max_y > out_merged->max_y) out_merged->max_y = aabbs[i].max_y;
        if (aabbs[i].max_z > out_merged->max_z) out_merged->max_z = aabbs[i].max_z;
    }

    return 0;
}

bool geometry_bounds_point_in_aabb(const aabb_t* aabb, float x, float y, float z)
{
    if (!aabb) return false;
    return x >= aabb->min_x && x <= aabb->max_x &&
           y >= aabb->min_y && y <= aabb->max_y &&
           z >= aabb->min_z && z <= aabb->max_z;
}

bool geometry_bounds_aabb_intersect(const aabb_t* aabb1, const aabb_t* aabb2)
{
    if (!aabb1 || !aabb2) return false;
    return aabb1->min_x <= aabb2->max_x && aabb1->max_x >= aabb2->min_x &&
           aabb1->min_y <= aabb2->max_y && aabb1->max_y >= aabb2->min_y &&
           aabb1->min_z <= aabb2->max_z && aabb1->max_z >= aabb2->min_z;
}

void geometry_bounds_aabb_scale(aabb_t* aabb, float scale)
{
    if (!aabb) return;

    float center_x = (aabb->min_x + aabb->max_x) * 0.5f;
    float center_y = (aabb->min_y + aabb->max_y) * 0.5f;
    float center_z = (aabb->min_z + aabb->max_z) * 0.5f;

    float half_x = (aabb->max_x - aabb->min_x) * 0.5f * scale;
    float half_y = (aabb->max_y - aabb->min_y) * 0.5f * scale;
    float half_z = (aabb->max_z - aabb->min_z) * 0.5f * scale;

    aabb->min_x = center_x - half_x;
    aabb->max_x = center_x + half_x;
    aabb->min_y = center_y - half_y;
    aabb->max_y = center_y + half_y;
    aabb->min_z = center_z - half_z;
    aabb->max_z = center_z + half_z;
}

void geometry_bounds_aabb_half_extents(const aabb_t* aabb, float* out_hx, float* out_hy, float* out_hz)
{
    if (!aabb) return;
    if (out_hx) *out_hx = (aabb->max_x - aabb->min_x) * 0.5f;
    if (out_hy) *out_hy = (aabb->max_y - aabb->min_y) * 0.5f;
    if (out_hz) *out_hz = (aabb->max_z - aabb->min_z) * 0.5f;
}

/* ============================================================================
 * BOUNDING SPHERE OPERATIONS
 * ============================================================================ */

void geometry_bounds_sphere_from_aabb(const aabb_t* aabb, bounding_sphere_t* out_sphere)
{
    if (!aabb || !out_sphere) return;

    out_sphere->center_x = (aabb->min_x + aabb->max_x) * 0.5f;
    out_sphere->center_y = (aabb->min_y + aabb->max_y) * 0.5f;
    out_sphere->center_z = (aabb->min_z + aabb->max_z) * 0.5f;

    float dx = aabb->max_x - aabb->min_x;
    float dy = aabb->max_y - aabb->min_y;
    float dz = aabb->max_z - aabb->min_z;
    out_sphere->radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
}

// Ritter's bounding sphere algorithm: fast and reasonably tight
int geometry_bounds_compute_sphere(
    const void* vertex_data,
    uint32_t vertex_count,
    uint32_t vertex_stride,
    uint32_t position_offset,
    bounding_sphere_t* out_sphere)
{
    if (!vertex_data || vertex_count == 0 || !out_sphere) return -1;

    // First, find two extreme points along the longest axis
    const uint8_t* vert_ptr = (const uint8_t*)vertex_data + position_offset;

    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    float min_z = FLT_MAX, max_z = -FLT_MAX;

    for (uint32_t i = 0; i < vertex_count; i++) {
        const float* pos = (const float*)vert_ptr;
        if (pos[0] < min_x) min_x = pos[0];
        if (pos[0] > max_x) max_x = pos[0];
        if (pos[1] < min_y) min_y = pos[1];
        if (pos[1] > max_y) max_y = pos[1];
        if (pos[2] < min_z) min_z = pos[2];
        if (pos[2] > max_z) max_z = pos[2];
        vert_ptr += vertex_stride;
    }

    // Find the axis with largest extent
    float ext_x = max_x - min_x;
    float ext_y = max_y - min_y;
    float ext_z = max_z - min_z;

    float p1_x, p1_y, p1_z, p2_x, p2_y, p2_z;

    if (ext_x >= ext_y && ext_x >= ext_z) {
        p1_x = min_x; p1_y = min_y; p1_z = min_z;
        p2_x = max_x; p2_y = max_y; p2_z = max_z;
    } else if (ext_y >= ext_x && ext_y >= ext_z) {
        p1_x = min_x; p1_y = min_y; p1_z = min_z;
        p2_x = max_x; p2_y = max_y; p2_z = max_z;
    } else {
        p1_x = min_x; p1_y = min_y; p1_z = min_z;
        p2_x = max_x; p2_y = max_y; p2_z = max_z;
    }

    // Sphere center at midpoint
    out_sphere->center_x = (p1_x + p2_x) * 0.5f;
    out_sphere->center_y = (p1_y + p2_y) * 0.5f;
    out_sphere->center_z = (p1_z + p2_z) * 0.5f;

    float dx = p2_x - p1_x;
    float dy = p2_y - p1_y;
    float dz = p2_z - p1_z;
    out_sphere->radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;

    // Expand to include all points
    vert_ptr = (const uint8_t*)vertex_data + position_offset;
    for (uint32_t i = 0; i < vertex_count; i++) {
        const float* pos = (const float*)vert_ptr;
        float cx = pos[0] - out_sphere->center_x;
        float cy = pos[1] - out_sphere->center_y;
        float cz = pos[2] - out_sphere->center_z;
        float dist = sqrtf(cx*cx + cy*cy + cz*cz);

        if (dist > out_sphere->radius) {
            // Expand sphere to include this point
            float old_radius = out_sphere->radius;
            out_sphere->radius = (old_radius + dist) * 0.5f;

            // Adjust center
            float move = (dist - old_radius) * 0.5f;
            if (dist > 0.0001f) {
                out_sphere->center_x += cx * (move / dist);
                out_sphere->center_y += cy * (move / dist);
                out_sphere->center_z += cz * (move / dist);
            }
        }
        vert_ptr += vertex_stride;
    }

    return 0;
}

void geometry_bounds_sphere_merge(
    const bounding_sphere_t* sphere1,
    const bounding_sphere_t* sphere2,
    bounding_sphere_t* out_merged)
{
    if (!sphere1 || !sphere2 || !out_merged) return;

    // Distance between centers
    float dx = sphere2->center_x - sphere1->center_x;
    float dy = sphere2->center_y - sphere1->center_y;
    float dz = sphere2->center_z - sphere1->center_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    // Check if one sphere contains the other
    if (dist + sphere2->radius <= sphere1->radius) {
        memcpy(out_merged, sphere1, sizeof(bounding_sphere_t));
        return;
    }
    if (dist + sphere1->radius <= sphere2->radius) {
        memcpy(out_merged, sphere2, sizeof(bounding_sphere_t));
        return;
    }

    // Merge: find new sphere enclosing both
    float new_radius = (dist + sphere1->radius + sphere2->radius) * 0.5f;
    float t = (new_radius - sphere1->radius) / (dist + 0.0001f);

    out_merged->center_x = sphere1->center_x + dx * t;
    out_merged->center_y = sphere1->center_y + dy * t;
    out_merged->center_z = sphere1->center_z + dz * t;
    out_merged->radius = new_radius;
}

bool geometry_bounds_point_in_sphere(const bounding_sphere_t* sphere, float x, float y, float z)
{
    if (!sphere) return false;

    float dx = x - sphere->center_x;
    float dy = y - sphere->center_y;
    float dz = z - sphere->center_z;
    float dist_sq = dx*dx + dy*dy + dz*dz;
    return dist_sq <= sphere->radius * sphere->radius;
}

bool geometry_bounds_sphere_intersect(const bounding_sphere_t* sphere1, const bounding_sphere_t* sphere2)
{
    if (!sphere1 || !sphere2) return false;

    float dx = sphere2->center_x - sphere1->center_x;
    float dy = sphere2->center_y - sphere1->center_y;
    float dz = sphere2->center_z - sphere1->center_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    return dist <= (sphere1->radius + sphere2->radius);
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t geometry_mesh_bounds_get_count(void)
{
    // This is a utility module, not a resource pool
    return 0;
}

void geometry_mesh_bounds_debug_print(void)
{
    printf("=== Mesh Bounds Utility Module ===\n");
    printf("Provides AABB and bounding sphere computation utilities\n");
    printf("No global state to report\n");
}
