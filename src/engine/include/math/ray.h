#ifndef RAY_H
#define RAY_H

#include <float.h>
#include "../core/utils.h"

// Basic Vec3 structure to avoid conflicts
typedef struct {
    f32 x, y, z;
} Vec3Ray;

// Local AABB structure to avoid conflicts
typedef struct {
    Vec3Ray min;
    Vec3Ray max;
} AABBRay;

// Local Plane structure to avoid conflicts  
typedef struct {
    Vec3Ray normal;
    f32 distance;
} PlaneRay;

// Ray structure
typedef struct {
    Vec3Ray origin;
    Vec3Ray direction; // Should be normalized
} Ray;

// Basic Vec3 operations
static inline Vec3Ray vec3ray_create(f32 x, f32 y, f32 z) {
    Vec3Ray v = {x, y, z};
    return v;
}

static inline Vec3Ray vec3ray_add(Vec3Ray a, Vec3Ray b) {
    return vec3ray_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3Ray vec3ray_sub(Vec3Ray a, Vec3Ray b) {
    return vec3ray_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3Ray vec3ray_mul(Vec3Ray v, f32 s) {
    return vec3ray_create(v.x * s, v.y * s, v.z * s);
}

static inline f32 vec3ray_dot(Vec3Ray a, Vec3Ray b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline f32 vec3ray_length(Vec3Ray v) {
    return sqrtf(vec3ray_dot(v, v));
}

static inline Vec3Ray vec3ray_cross(Vec3Ray a, Vec3Ray b) {
    return vec3ray_create(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                          a.x * b.y - a.y * b.x);
}

static inline Vec3Ray vec3ray_normalize(Vec3Ray v) {
    f32 len = vec3ray_length(v);
    if (len < 1e-6f) {
        return vec3ray_create(0.0f, 0.0f, 0.0f);
    }
    return vec3ray_mul(v, 1.0f / len);
}

static inline Ray ray_create(Vec3Ray origin, Vec3Ray direction) {
    Ray r;
    r.origin = origin;
    r.direction = vec3ray_normalize(direction);
    return r;
}

static inline Ray ray_from_points(Vec3Ray start, Vec3Ray end) {
    Ray r;
    r.origin = start;
    r.direction = vec3ray_normalize(vec3ray_sub(end, start));
    return r;
}

static inline Vec3Ray ray_at(Ray r, f32 t) {
    return vec3ray_add(r.origin, vec3ray_mul(r.direction, t));
}

// Ray-Sphere intersection
// Returns true if intersection occurs, stores distance in *t
static inline bool ray_intersects_sphere(Ray r, Vec3Ray center, f32 radius, f32 *t) {
    Vec3Ray oc = vec3ray_sub(r.origin, center);
    f32 b = vec3ray_dot(oc, r.direction);
    f32 c = vec3ray_dot(oc, oc) - radius * radius;
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
static inline bool ray_intersects_aabb(Ray r, AABBRay box, f32 *t) {
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
        tmin = (tmin > t0) ? tmin : t0;
        tmax = (tmax < t1) ? tmax : t1;
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
        tmin = (tmin > t0) ? tmin : t0;
        tmax = (tmax < t1) ? tmax : t1;
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
        tmin = (tmin > t0) ? tmin : t0;
        tmax = (tmax < t1) ? tmax : t1;
        if (tmax <= tmin) return false;
    }

    if (t) *t = (tmin > 0.0f) ? tmin : tmax;
    return true;
}

// Ray-Plane intersection
static inline bool ray_intersects_plane(Ray r, PlaneRay p, f32 *t) {
    f32 denom = vec3ray_dot(p.normal, r.direction);
    if (fabsf(denom) > EPSILON) {
        f32 t_hit = -(vec3ray_dot(p.normal, r.origin) + p.distance) / denom;
        if (t_hit >= 0.0f) {
            if (t) *t = t_hit;
            return true;
        }
    }
    return false;
}

// Ray-Triangle intersection (Mller-Trumbore)
static inline bool ray_intersects_triangle(Ray r, Vec3Ray v0, Vec3Ray v1, Vec3Ray v2, f32 *t) {
    Vec3Ray v0v1 = vec3ray_sub(v1, v0);
    Vec3Ray v0v2 = vec3ray_sub(v2, v0);
    Vec3Ray pvec = vec3ray_cross(r.direction, v0v2);
    f32 det = vec3ray_dot(v0v1, pvec);

    // If determinant is near zero, ray lies in plane of triangle
    if (fabsf(det) < EPSILON) return false;

    f32 inv_det = 1.0f / det;
    Vec3Ray tvec = vec3ray_sub(r.origin, v0);
    f32 u = vec3ray_dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f) return false;

    Vec3Ray qvec = vec3ray_cross(tvec, v0v1);
    f32 v = vec3ray_dot(r.direction, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f) return false;

    f32 t_hit = vec3ray_dot(v0v2, qvec) * inv_det;

    if (t_hit > EPSILON) { // Ray intersection
        if (t) *t = t_hit;
        return true;
    }

    return false;
}

#endif // RAY_H
