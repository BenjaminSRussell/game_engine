#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "mat4.h"
#include "plane.h"
#include "aabb.h"

typedef struct {
    Plane planes[6];
} Frustum;

// Extract frustum planes from view-projection matrix
// Assumes OpenGL style matrix (column-major)
// Planes are normalized.
// Plane order: Left, Right, Bottom, Top, Near, Far
static inline Frustum frustum_from_mat4(Mat4 m) {
    Frustum f;
    f32* mat = m.m;

    // Left plane
    f.planes[0].normal.x = mat[3] + mat[0];
    f.planes[0].normal.y = mat[7] + mat[4];
    f.planes[0].normal.z = mat[11] + mat[8];
    f.planes[0].distance = mat[15] + mat[12];

    // Right plane
    f.planes[1].normal.x = mat[3] - mat[0];
    f.planes[1].normal.y = mat[7] - mat[4];
    f.planes[1].normal.z = mat[11] - mat[8];
    f.planes[1].distance = mat[15] - mat[12];

    // Bottom plane
    f.planes[2].normal.x = mat[3] + mat[1];
    f.planes[2].normal.y = mat[7] + mat[5];
    f.planes[2].normal.z = mat[11] + mat[9];
    f.planes[2].distance = mat[15] + mat[13];

    // Top plane
    f.planes[3].normal.x = mat[3] - mat[1];
    f.planes[3].normal.y = mat[7] - mat[5];
    f.planes[3].normal.z = mat[11] - mat[9];
    f.planes[3].distance = mat[15] - mat[13];

    // Near plane
    f.planes[4].normal.x = mat[3] + mat[2];
    f.planes[4].normal.y = mat[7] + mat[6];
    f.planes[4].normal.z = mat[11] + mat[10];
    f.planes[4].distance = mat[15] + mat[14];

    // Far plane
    f.planes[5].normal.x = mat[3] - mat[2];
    f.planes[5].normal.y = mat[7] - mat[6];
    f.planes[5].normal.z = mat[11] - mat[10];
    f.planes[5].distance = mat[15] - mat[14];

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        f.planes[i] = plane_normalize(f.planes[i]);
    }

    return f;
}

// Check if AABB is inside or intersects frustum
static inline bool frustum_intersects_aabb(Frustum f, AABB box) {
    for (int i = 0; i < 6; ++i) {
        if (plane_aabb_is_outside(f.planes[i], box)) {
            return false;
        }
    }
    return true;
}

// Check if Sphere is inside or intersects frustum
static inline bool frustum_intersects_sphere(Frustum f, Vec3 center, f32 radius) {
    for (int i = 0; i < 6; ++i) {
        if (plane_sphere_is_outside(f.planes[i], center, radius)) {
            return false;
        }
    }
    return true;
}

static inline bool frustum_contains_point(Frustum f, Vec3 p) {
    for (int i = 0; i < 6; ++i) {
        if (plane_distance_to_point(f.planes[i], p) < 0.0f) {
            return false;
        }
    }
    return true;
}

#endif // FRUSTUM_H
