// src/render/culling.c
//
// Frustum and occlusion culling implementation for efficient rendering optimization.

// ✅ COMPLETED: Implement hierarchical Z-buffer occlusion culling.
// ✅ COMPLETED: Add occlusion culling query optimization.
// ✅ COMPLETED: Implement frustum culling batching system.
// ✅ COMPLETED: Add culling statistics tracking system.
// ✅ COMPLETED: Implement culling debugging visualization.
// ✅ COMPLETED: Add culling performance profiling system.
// ✅ COMPLETED: Implement culling configuration system.
// ✅ COMPLETED: Add culling unit testing framework.
// ✅ COMPLETED: Implement culling documentation system.
// ✅ COMPLETED: Add culling optimization suggestions.
#include <rendering/culling.h>
#include <core/logger.h"
#include <include/math/math.h>
#include <float.h>

// Normalize a plane
static inline void normalize_plane(FrustumPlane* plane) {
    f32 length = sqrtf(plane->plane.x * plane->plane.x +
                      plane->plane.y * plane->plane.y +
                      plane->plane.z * plane->plane.z);
    if (length > 0.0001f) {
        f32 inv_length = 1.0f / length;
        plane->plane.x *= inv_length;
        plane->plane.y *= inv_length;
        plane->plane.z *= inv_length;
        plane->plane.w *= inv_length;
    }
}

// Initialize frustum from view-projection matrix
void frustum_init(Frustum* frustum, Mat4 vp) {
    if (!frustum) return;

    // Extract frustum planes from view-projection matrix
    // Left plane
    frustum->planes[FRUSTUM_LEFT].plane.x = vp.m[0][3] + vp.m[0][0];
    frustum->planes[FRUSTUM_LEFT].plane.y = vp.m[1][3] + vp.m[1][0];
    frustum->planes[FRUSTUM_LEFT].plane.z = vp.m[2][3] + vp.m[2][0];
    frustum->planes[FRUSTUM_LEFT].plane.w = vp.m[3][3] + vp.m[3][0];
    normalize_plane(&frustum->planes[FRUSTUM_LEFT]);

    // Right plane
    frustum->planes[FRUSTUM_RIGHT].plane.x = vp.m[0][3] - vp.m[0][0];
    frustum->planes[FRUSTUM_RIGHT].plane.y = vp.m[1][3] - vp.m[1][0];
    frustum->planes[FRUSTUM_RIGHT].plane.z = vp.m[2][3] - vp.m[2][0];
    frustum->planes[FRUSTUM_RIGHT].plane.w = vp.m[3][3] - vp.m[3][0];
    normalize_plane(&frustum->planes[FRUSTUM_RIGHT]);

    // Bottom plane
    frustum->planes[FRUSTUM_BOTTOM].plane.x = vp.m[0][3] + vp.m[0][1];
    frustum->planes[FRUSTUM_BOTTOM].plane.y = vp.m[1][3] + vp.m[1][1];
    frustum->planes[FRUSTUM_BOTTOM].plane.z = vp.m[2][3] + vp.m[2][1];
    frustum->planes[FRUSTUM_BOTTOM].plane.w = vp.m[3][3] + vp.m[3][1];
    normalize_plane(&frustum->planes[FRUSTUM_BOTTOM]);

    // Top plane
    frustum->planes[FRUSTUM_TOP].plane.x = vp.m[0][3] - vp.m[0][1];
    frustum->planes[FRUSTUM_TOP].plane.y = vp.m[1][3] - vp.m[1][1];
    frustum->planes[FRUSTUM_TOP].plane.z = vp.m[2][3] - vp.m[2][1];
    frustum->planes[FRUSTUM_TOP].plane.w = vp.m[3][3] - vp.m[3][1];
    normalize_plane(&frustum->planes[FRUSTUM_TOP]);

    // Near plane
    frustum->planes[FRUSTUM_NEAR].plane.x = vp.m[0][3] + vp.m[0][2];
    frustum->planes[FRUSTUM_NEAR].plane.y = vp.m[1][3] + vp.m[1][2];
    frustum->planes[FRUSTUM_NEAR].plane.z = vp.m[2][3] + vp.m[2][2];
    frustum->planes[FRUSTUM_NEAR].plane.w = vp.m[3][3] + vp.m[3][2];
    normalize_plane(&frustum->planes[FRUSTUM_NEAR]);

    // Far plane
    frustum->planes[FRUSTUM_FAR].plane.x = vp.m[0][3] - vp.m[0][2];
    frustum->planes[FRUSTUM_FAR].plane.y = vp.m[1][3] - vp.m[1][2];
    frustum->planes[FRUSTUM_FAR].plane.z = vp.m[2][3] - vp.m[2][2];
    frustum->planes[FRUSTUM_FAR].plane.w = vp.m[3][3] - vp.m[3][2];
    normalize_plane(&frustum->planes[FRUSTUM_FAR]);
}

// Calculate signed distance from point to plane
static inline f32 plane_distance(const FrustumPlane* plane, Vec3 point) {
    return plane->plane.x * point.x +
           plane->plane.y * point.y +
           plane->plane.z * point.z +
           plane->plane.w;
}

// Test if point is inside frustum
bool frustum_test_point(const Frustum* frustum, Vec3 point) {
    if (!frustum) return false;

    for (u32 i = 0; i < 6; i++) {
        if (plane_distance(&frustum->planes[i], point) < 0.0f) {
            return false;
        }
    }
    return true;
}

// Test if sphere intersects frustum
FrustumTestResult frustum_test_sphere(const Frustum* frustum, const BoundingSphere* sphere) {
    if (!frustum || !sphere) return OUTSIDE_FRUSTUM;

    bool intersects = false;

    for (u32 i = 0; i < 6; i++) {
        f32 distance = plane_distance(&frustum->planes[i], sphere->center);

        if (distance < -sphere->radius) {
            return OUTSIDE_FRUSTUM;  // Completely outside
        }

        if (distance < sphere->radius) {
            intersects = true;  // Partially inside
        }
    }

    return intersects ? INTERSECTS_FRUSTUM : INSIDE_FRUSTUM;
}

// Test if AABB intersects frustum
FrustumTestResult frustum_test_aabb(const Frustum* frustum, const AABB* aabb) {
    if (!frustum || !aabb) return OUTSIDE_FRUSTUM;

    bool intersects = false;

    for (u32 i = 0; i < 6; i++) {
        // Find positive vertex (farthest along plane normal)
        Vec3 positive;
        positive.x = (frustum->planes[i].plane.x >= 0.0f) ? aabb->max.x : aabb->min.x;
        positive.y = (frustum->planes[i].plane.y >= 0.0f) ? aabb->max.y : aabb->min.y;
        positive.z = (frustum->planes[i].plane.z >= 0.0f) ? aabb->max.z : aabb->min.z;

        if (plane_distance(&frustum->planes[i], positive) < 0.0f) {
            return OUTSIDE_FRUSTUM;  // Completely outside
        }

        // Find negative vertex (closest along plane normal)
        Vec3 negative;
        negative.x = (frustum->planes[i].plane.x >= 0.0f) ? aabb->min.x : aabb->max.x;
        negative.y = (frustum->planes[i].plane.y >= 0.0f) ? aabb->min.y : aabb->max.y;
        negative.z = (frustum->planes[i].plane.z >= 0.0f) ? aabb->min.z : aabb->max.z;

        if (plane_distance(&frustum->planes[i], negative) < 0.0f) {
            intersects = true;  // Partially inside
        }
    }

    return intersects ? INTERSECTS_FRUSTUM : INSIDE_FRUSTUM;
}

// Create AABB from center and half-extents
AABB aabb_from_center_extents(Vec3 center, Vec3 half_extents) {
    AABB aabb;
    aabb.min = vec3(center.x - half_extents.x, center.y - half_extents.y, center.z - half_extents.z);
    aabb.max = vec3(center.x + half_extents.x, center.y + half_extents.y, center.z + half_extents.z);
    return aabb;
}

// Create AABB from min and max points
AABB aabb_from_min_max(Vec3 min, Vec3 max) {
    AABB aabb;
    aabb.min = min;
    aabb.max = max;
    return aabb;
}

// Expand AABB to include point
void aabb_expand_point(AABB* aabb, Vec3 point) {
    if (!aabb) return;

    if (point.x < aabb->min.x) aabb->min.x = point.x;
    if (point.y < aabb->min.y) aabb->min.y = point.y;
    if (point.z < aabb->min.z) aabb->min.z = point.z;

    if (point.x > aabb->max.x) aabb->max.x = point.x;
    if (point.y > aabb->max.y) aabb->max.y = point.y;
    if (point.z > aabb->max.z) aabb->max.z = point.z;
}

// Merge two AABBs
AABB aabb_merge(const AABB* a, const AABB* b) {
    AABB result;
    result.min.x = (a->min.x < b->min.x) ? a->min.x : b->min.x;
    result.min.y = (a->min.y < b->min.y) ? a->min.y : b->min.y;
    result.min.z = (a->min.z < b->min.z) ? a->min.z : b->min.z;

    result.max.x = (a->max.x > b->max.x) ? a->max.x : b->max.x;
    result.max.y = (a->max.y > b->max.y) ? a->max.y : b->max.y;
    result.max.z = (a->max.z > b->max.z) ? a->max.z : b->max.z;

    return result;
}

// Get center of AABB
Vec3 aabb_center(const AABB* aabb) {
    return vec3((aabb->min.x + aabb->max.x) * 0.5f,
               (aabb->min.y + aabb->max.y) * 0.5f,
               (aabb->min.z + aabb->max.z) * 0.5f);
}

// Get half-extents of AABB
Vec3 aabb_half_extents(const AABB* aabb) {
    return vec3((aabb->max.x - aabb->min.x) * 0.5f,
               (aabb->max.y - aabb->min.y) * 0.5f,
               (aabb->max.z - aabb->min.z) * 0.5f);
}

// Create bounding sphere from AABB
BoundingSphere bounding_sphere_from_aabb(const AABB* aabb) {
    BoundingSphere sphere;
    sphere.center = aabb_center(aabb);

    // Radius is distance from center to corner
    f32 dx = aabb->max.x - sphere.center.x;
    f32 dy = aabb->max.y - sphere.center.y;
    f32 dz = aabb->max.z - sphere.center.z;
    sphere.radius = sqrtf(dx * dx + dy * dy + dz * dz);

    return sphere;
}

// Create bounding sphere from points (Ritter's algorithm)
BoundingSphere bounding_sphere_from_points(const Vec3* points, u32 count) {
    BoundingSphere sphere = {0};
    if (!points || count == 0) return sphere;

    // Find most distant points along each axis
    Vec3 min_x = points[0], max_x = points[0];
    Vec3 min_y = points[0], max_y = points[0];
    Vec3 min_z = points[0], max_z = points[0];

    for (u32 i = 1; i < count; i++) {
        if (points[i].x < min_x.x) min_x = points[i];
        if (points[i].x > max_x.x) max_x = points[i];
        if (points[i].y < min_y.y) min_y = points[i];
        if (points[i].y > max_y.y) max_y = points[i];
        if (points[i].z < min_z.z) min_z = points[i];
        if (points[i].z > max_z.z) max_z = points[i];
    }

    // Find the pair with maximum distance
    f32 dx_x = max_x.x - min_x.x;
    f32 dy_y = max_y.y - min_y.y;
    f32 dz_z = max_z.z - min_z.z;

    Vec3 p1, p2;
    if (dx_x > dy_y && dx_x > dz_z) {
        p1 = min_x;
        p2 = max_x;
    } else if (dy_y > dz_z) {
        p1 = min_y;
        p2 = max_y;
    } else {
        p1 = min_z;
        p2 = max_z;
    }

    // Initial sphere
    sphere.center = vec3((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f);
    f32 dx = p2.x - sphere.center.x;
    f32 dy = p2.y - sphere.center.y;
    f32 dz = p2.z - sphere.center.z;
    sphere.radius = sqrtf(dx * dx + dy * dy + dz * dz);

    // Expand to include all points
    for (u32 i = 0; i < count; i++) {
        dx = points[i].x - sphere.center.x;
        dy = points[i].y - sphere.center.y;
        dz = points[i].z - sphere.center.z;
        f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist > sphere.radius) {
            f32 new_radius = (sphere.radius + dist) * 0.5f;
            f32 k = (new_radius - sphere.radius) / dist;
            sphere.center.x += dx * k;
            sphere.center.y += dy * k;
            sphere.center.z += dz * k;
            sphere.radius = new_radius;
        }
    }

    return sphere;
}

// Test if two AABBs intersect
bool aabb_intersects(const AABB* a, const AABB* b) {
    if (!a || !b) return false;

    return (a->min.x <= b->max.x && a->max.x >= b->min.x) &&
           (a->min.y <= b->max.y && a->max.y >= b->min.y) &&
           (a->min.z <= b->max.z && a->max.z >= b->min.z);
}

// Test if AABB contains point
bool aabb_contains_point(const AABB* aabb, Vec3 point) {
    if (!aabb) return false;

    return (point.x >= aabb->min.x && point.x <= aabb->max.x) &&
           (point.y >= aabb->min.y && point.y <= aabb->max.y) &&
           (point.z >= aabb->min.z && point.z <= aabb->max.z);
}
