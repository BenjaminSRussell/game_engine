/*
 * mesh_bounds.h
 * AABB/bounding sphere calculation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_BOUNDS_H
#define GEOMETRY_MESH_BOUNDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

#include "geometry/mesh/mesh_data.h"

/* Bounding volume utilities for mesh optimization */

/* ============================================================================
 * API - AABB OPERATIONS
 * ============================================================================ */

// Compute AABB from vertex data
// vertex_stride: bytes between each vertex
// position_offset: offset of position in each vertex (usually 0)
int geometry_bounds_compute_aabb(
    const void* vertex_data,
    uint32_t vertex_count,
    uint32_t vertex_stride,
    uint32_t position_offset,  // Offset to position data within vertex
    aabb_t* out_aabb
);

// Compute AABB from multiple AABBs (union of all bounds)
int geometry_bounds_merge_aabbs(
    const aabb_t* aabbs,
    uint32_t count,
    aabb_t* out_merged
);

// Check if point is inside AABB
bool geometry_bounds_point_in_aabb(const aabb_t* aabb, float x, float y, float z);

// Check if two AABBs intersect
bool geometry_bounds_aabb_intersect(const aabb_t* aabb1, const aabb_t* aabb2);

// Transform AABB by a uniform scale
void geometry_bounds_aabb_scale(aabb_t* aabb, float scale);

// Get AABB size (half-extents)
void geometry_bounds_aabb_half_extents(const aabb_t* aabb, float* out_hx, float* out_hy, float* out_hz);

/* ============================================================================
 * API - BOUNDING SPHERE OPERATIONS
 * ============================================================================ */

// Compute bounding sphere from AABB (fast, axis-aligned)
void geometry_bounds_sphere_from_aabb(const aabb_t* aabb, bounding_sphere_t* out_sphere);

// Compute optimal bounding sphere from vertex data (Ritter's algorithm)
int geometry_bounds_compute_sphere(
    const void* vertex_data,
    uint32_t vertex_count,
    uint32_t vertex_stride,
    uint32_t position_offset,
    bounding_sphere_t* out_sphere
);

// Merge two bounding spheres
void geometry_bounds_sphere_merge(
    const bounding_sphere_t* sphere1,
    const bounding_sphere_t* sphere2,
    bounding_sphere_t* out_merged
);

// Check if point is inside bounding sphere
bool geometry_bounds_point_in_sphere(const bounding_sphere_t* sphere, float x, float y, float z);

// Check if two spheres intersect
bool geometry_bounds_sphere_intersect(const bounding_sphere_t* sphere1, const bounding_sphere_t* sphere2);

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t geometry_mesh_bounds_get_count(void);
void geometry_mesh_bounds_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_BOUNDS_H */
