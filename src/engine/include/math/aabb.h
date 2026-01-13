/*
 * aabb.h
 * Axis Aligned Bounding Box structure and operations
 *
 * Part of the Math Core
 */

#ifndef MATH_AABB_H
#define MATH_AABB_H

#include "common.h"
#include "vec3.h"
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

/* ============================================================================
 * INLINE FUNCTIONS
 * ============================================================================ */

static inline AABB aabb_create(Vec3 min, Vec3 max) {
    AABB box;
    box.min = min;
    box.max = max;
    return box;
}

static inline AABB aabb_create_empty(void) {
    return aabb_create(
        vec3_create(FLT_MAX, FLT_MAX, FLT_MAX),
        vec3_create(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    );
}

static inline AABB aabb_union(AABB a, AABB b) {
    return aabb_create(
        vec3_min(a.min, b.min),
        vec3_max(a.max, b.max)
    );
}

static inline f32 aabb_surface_area(AABB box) {
    Vec3 extent = vec3_sub(box.max, box.min);
    return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

static inline AABB aabb_expand(AABB box, Vec3 point) {
    return aabb_create(
        vec3_min(box.min, point),
        vec3_max(box.max, point)
    );
}

static inline Vec3 aabb_center(AABB box) {
    return vec3_mul(vec3_add(box.min, box.max), 0.5f);
}

static inline Vec3 aabb_extent(AABB box) {
    return vec3_sub(box.max, box.min);
}

static inline bool aabb_intersects_aabb(AABB a, AABB b) {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

static inline bool aabb_contains_point(AABB box, Vec3 point) {
    return (point.x >= box.min.x && point.x <= box.max.x &&
            point.y >= box.min.y && point.y <= box.max.y &&
            point.z >= box.min.z && point.z <= box.max.z);
}

#ifdef __cplusplus
}
#endif

#endif /* MATH_AABB_H */
