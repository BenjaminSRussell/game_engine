#ifndef RAY_H
#define RAY_H

#include "vec3.h"
#include "plane.h"
#include "aabb.h"
#include <float.h>

typedef struct {
    Vec3 origin;
    Vec3 direction; // Should be normalized
} Ray;

static inline Ray ray_create(Vec3 origin, Vec3 direction) {
    Ray r;
    r.origin = origin;
    r.direction = vec3_normalize(direction);
    return r;
}

static inline Ray ray_from_points(Vec3 start, Vec3 end) {
    Ray r;
    r.origin = start;
    r.direction = vec3_normalize(vec3_sub(end, start));
    return r;
}

static inline Vec3 ray_at(Ray r, f32 t) {
    return vec3_add(r.origin, vec3_mul(r.direction, t));
}

// Ray-Sphere intersection
// Returns true if intersection occurs, stores distance in *t
static inline bool ray_intersects_sphere(Ray r, Vec3 center, f32 radius, f32 *t) {
    Vec3 oc = vec3_sub(r.origin, center);
    f32 b = vec3_dot(oc, r.direction);
    f32 c = vec3_dot(oc, oc) - radius * radius;
    f32 h = b * b - c;

    if (h < 0.0f) return false;

    h = sqrtf(h);
    f32 t0 = -b - h;
    f32 t1 = -b + h;

    if (t0 > 0.001f) {
        if (t) *t = t0;
        return true;
    }
    if (t1 > 0.001f) {
        if (t) *t = t1;
        return true;
    }

    return false;
}

// Ray-AABB intersection (slab method)
static inline bool ray_intersects_aabb(Ray r, AABB box, f32 *t) {
    f32 tmin = -FLT_MAX;
    f32 tmax = FLT_MAX;

    // For each axis
    // X
    if (fabsf(r.direction.x) < EPSILON) {
        if (r.origin.x < box.min.x || r.origin.x > box.max.x) return false;
    } else {
        f32 inv_d = 1.0f / r.direction.x;
        f32 t0 = (box.min.x - r.origin.x) * inv_d;
        f32 t1 = (box.max.x - r.origin.x) * inv_d;
        if (inv_d < 0.0f) { f32 temp = t0; t0 = t1; t1 = temp; }
        tmin = MAX(tmin, t0);
        tmax = MIN(tmax, t1);
        if (tmax <= tmin) return false;
    }

    // Y
    if (fabsf(r.direction.y) < EPSILON) {
        if (r.origin.y < box.min.y || r.origin.y > box.max.y) return false;
    } else {
        f32 inv_d = 1.0f / r.direction.y;
        f32 t0 = (box.min.y - r.origin.y) * inv_d;
        f32 t1 = (box.max.y - r.origin.y) * inv_d;
        if (inv_d < 0.0f) { f32 temp = t0; t0 = t1; t1 = temp; }
        tmin = MAX(tmin, t0);
        tmax = MIN(tmax, t1);
        if (tmax <= tmin) return false;
    }

    // Z
    if (fabsf(r.direction.z) < EPSILON) {
        if (r.origin.z < box.min.z || r.origin.z > box.max.z) return false;
    } else {
        f32 inv_d = 1.0f / r.direction.z;
        f32 t0 = (box.min.z - r.origin.z) * inv_d;
        f32 t1 = (box.max.z - r.origin.z) * inv_d;
        if (inv_d < 0.0f) { f32 temp = t0; t0 = t1; t1 = temp; }
        tmin = MAX(tmin, t0);
        tmax = MIN(tmax, t1);
        if (tmax <= tmin) return false;
    }

    if (t) *t = tmin > 0.0f ? tmin : tmax;
    return true;
}

// Ray-Plane intersection
static inline bool ray_intersects_plane(Ray r, Plane p, f32 *t) {
    f32 denom = vec3_dot(p.normal, r.direction);
    if (fabsf(denom) > EPSILON) {
        f32 t_hit = -(vec3_dot(p.normal, r.origin) + p.distance) / denom;
        if (t_hit >= 0.0f) {
            if (t) *t = t_hit;
            return true;
        }
    }
    return false;
}

// Ray-Triangle intersection (Mller-Trumbore)
static inline bool ray_intersects_triangle(Ray r, Vec3 v0, Vec3 v1, Vec3 v2, f32 *t) {
    Vec3 v0v1 = vec3_sub(v1, v0);
    Vec3 v0v2 = vec3_sub(v2, v0);
    Vec3 pvec = vec3_cross(r.direction, v0v2);
    f32 det = vec3_dot(v0v1, pvec);

    // If determinant is near zero, ray lies in plane of triangle
    if (fabsf(det) < EPSILON) return false;

    f32 inv_det = 1.0f / det;
    Vec3 tvec = vec3_sub(r.origin, v0);
    f32 u = vec3_dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f) return false;

    Vec3 qvec = vec3_cross(tvec, v0v1);
    f32 v = vec3_dot(r.direction, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f) return false;

    f32 t_hit = vec3_dot(v0v2, qvec) * inv_det;

    if (t_hit > EPSILON) { // Ray intersection
        if (t) *t = t_hit;
        return true;
    }

    return false;
}

#endif // RAY_H
