#ifndef PLANE_H
#define PLANE_H

#include "../common.h"
#include "vec3.h"
#include "aabb.h"

// Plane equation: Ax + By + Cz + D = 0
// normal = (A, B, C), distance = D
typedef struct {
    Vec3 normal;
    f32 distance;
} Plane;

static inline Plane plane_create(Vec3 normal, f32 distance) {
    Plane p;
    p.normal = normal;
    p.distance = distance;
    return p;
}

static inline Plane plane_normalize(Plane p) {
    f32 len = vec3_length(p.normal);
    if (len > EPSILON) {
        f32 inv_len = 1.0f / len;
        return plane_create(vec3_mul(p.normal, inv_len), p.distance * inv_len);
    }
    return p;
}

static inline Plane plane_from_points(Vec3 a, Vec3 b, Vec3 c) {
    Vec3 ab = vec3_sub(b, a);
    Vec3 ac = vec3_sub(c, a);
    Vec3 normal = vec3_normalize(vec3_cross(ab, ac));
    f32 distance = -vec3_dot(normal, a);
    return plane_create(normal, distance);
}

static inline Plane plane_from_point_normal(Vec3 point, Vec3 normal) {
    Vec3 n = vec3_normalize(normal);
    return plane_create(n, -vec3_dot(n, point));
}

static inline f32 plane_distance_to_point(Plane p, Vec3 point) {
    return vec3_dot(p.normal, point) + p.distance;
}

// Check if AABB intersects plane (or is on positive side)
// Useful for frustum culling.
// Returns:
// > 0 if AABB is completely on the positive side of the plane
// < 0 if AABB is completely on the negative side of the plane
// = 0 if AABB intersects the plane
// Wait, for culling we usually want to know if it's "outside" (negative side).
static inline bool plane_aabb_is_outside(Plane p, AABB box) {
    // To check if box is completely on negative side:
    // We find the point of the AABB furthest in the direction of the normal.
    // If that point is on the negative side, the whole box is.

    // Support point in direction of normal
    Vec3 max_point;
    max_point.x = (p.normal.x > 0.0f) ? box.max.x : box.min.x;
    max_point.y = (p.normal.y > 0.0f) ? box.max.y : box.min.y;
    max_point.z = (p.normal.z > 0.0f) ? box.max.z : box.min.z;

    return plane_distance_to_point(p, max_point) < 0.0f;
}

static inline bool plane_sphere_is_outside(Plane p, Vec3 center, f32 radius) {
    return plane_distance_to_point(p, center) < -radius;
}

#endif // PLANE_H
