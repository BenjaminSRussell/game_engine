/*
 * aabb.h
 * Axis Aligned Bounding Box structure and operations
 *
 * Part of the Math Core
 */

#ifndef MATH_AABB_H
#define MATH_AABB_H

#include "include/common.h"
#include "include/math/vec3.h"
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
        vec3(FLT_MAX, FLT_MAX, FLT_MAX),
        vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX)
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

#ifdef __cplusplus
}
#endif

#endif /* MATH_AABB_H */
