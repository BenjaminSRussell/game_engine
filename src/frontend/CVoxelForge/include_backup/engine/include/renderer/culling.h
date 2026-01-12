// include/rendering/culling.h
//
// Purpose: Defines frustum and occlusion culling systems for optimizing rendering
// by eliminating geometry that's not visible to the camera. Frustum culling removes
// objects outside the view frustum, while occlusion culling removes objects behind
// other opaque geometry.
//
// This module uses the standard math types from frustum.h and aabb.h to avoid
// duplication and header conflicts.
//
// Public APIs:
// - `frustum_test_sphere`: Test if sphere intersects frustum
// - `frustum_test_aabb`: Test if axis-aligned bounding box intersects frustum
// - `frustum_test_point`: Test if point is inside frustum
// - `occlusion_culling_init`: Initialize occlusion culling system
// - `occlusion_culling_test`: Test if object is occluded
//
// Ownership: Culling structures are temporary and recreated each frame.
//
// Invariants:
// - Frustum planes must be normalized for accurate distance tests
// - AABB min values must be <= max values
// - Occlusion queries require GPU support
//
#ifndef CULLING_H
#define CULLING_H

#include "math/frustum.h"
#include "math/aabb.h"
#include "math/plane.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bounding sphere
typedef struct {
    Vec3 center;
    f32 radius;
} BoundingSphere;

// Intersection test result
typedef enum {
    OUTSIDE_FRUSTUM = 0,   // Completely outside
    INTERSECTS_FRUSTUM = 1, // Partially inside
    INSIDE_FRUSTUM = 2      // Completely inside
} FrustumTestResult;

// Test if point is inside frustum
static inline bool frustum_test_point(Frustum f, Vec3 point) {
    return frustum_contains_point(f, point);
}

// Test if sphere intersects frustum
static inline FrustumTestResult frustum_test_sphere(Frustum f, BoundingSphere sphere) {
    if (frustum_intersects_sphere(f, sphere.center, sphere.radius)) {
        return INTERSECTS_FRUSTUM;
    }
    return OUTSIDE_FRUSTUM;
}

// Test if AABB intersects frustum
static inline FrustumTestResult frustum_test_aabb(Frustum f, AABB aabb) {
    if (frustum_intersects_aabb(f, aabb)) {
        return INTERSECTS_FRUSTUM;
    }
    return OUTSIDE_FRUSTUM;
}

// Create bounding sphere from center and radius
static inline BoundingSphere bounding_sphere_create(Vec3 center, f32 radius) {
    return (BoundingSphere){center, radius};
}

// Create bounding sphere from AABB
static inline BoundingSphere bounding_sphere_from_aabb(AABB aabb) {
    Vec3 center = aabb_center(aabb);
    Vec3 extent = aabb_extent(aabb);
    f32 radius = (vec3_length(extent) * 0.5f);
    return bounding_sphere_create(center, radius);
}

#ifdef __cplusplus
}
#endif

#endif // CULLING_H
