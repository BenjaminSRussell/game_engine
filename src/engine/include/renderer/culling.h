// include/render/culling.h
//
// Purpose: Defines frustum and occlusion culling systems for optimizing rendering
// by eliminating geometry that's not visible to the camera. Frustum culling removes
// objects outside the view frustum, while occlusion culling removes objects behind
// other opaque geometry.
//
// Public APIs:
// - `frustum_init`: Initialize frustum from view-projection matrix
// - `frustum_test_sphere`: Test if sphere intersects frustum
// - `frustum_test_aabb`: Test if axis-aligned bounding box intersects frustum
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

#include <common.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Frustum plane
typedef struct {
    Vec4 plane;  // Plane equation: ax + by + cz + d = 0
} FrustumPlane;

// View frustum (6 planes)
typedef struct {
    FrustumPlane planes[6];  // Left, Right, Bottom, Top, Near, Far
} Frustum;

// Axis-aligned bounding box
typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

// Bounding sphere
typedef struct {
    Vec3 center;
    f32 radius;
} BoundingSphere;

// Frustum plane indices
typedef enum {
    FRUSTUM_LEFT = 0,
    FRUSTUM_RIGHT = 1,
    FRUSTUM_BOTTOM = 2,
    FRUSTUM_TOP = 3,
    FRUSTUM_NEAR = 4,
    FRUSTUM_FAR = 5
} FrustumPlaneIndex;

// Intersection test result
typedef enum {
    OUTSIDE_FRUSTUM = 0,   // Completely outside
    INTERSECTS_FRUSTUM = 1, // Partially inside
    INSIDE_FRUSTUM = 2      // Completely inside
} FrustumTestResult;

// Initialize frustum from view-projection matrix
void frustum_init(Frustum* frustum, Mat4 view_proj);

// Test if point is inside frustum
bool frustum_test_point(const Frustum* frustum, Vec3 point);

// Test if sphere intersects frustum
FrustumTestResult frustum_test_sphere(const Frustum* frustum, const BoundingSphere* sphere);

// Test if AABB intersects frustum
FrustumTestResult frustum_test_aabb(const Frustum* frustum, const AABB* aabb);

// Create AABB from center and half-extents
AABB aabb_from_center_extents(Vec3 center, Vec3 half_extents);

// Create AABB from min and max points
AABB aabb_from_min_max(Vec3 min, Vec3 max);

// Expand AABB to include point
void aabb_expand_point(AABB* aabb, Vec3 point);

// Merge two AABBs
AABB aabb_merge(const AABB* a, const AABB* b);

// Get center of AABB
Vec3 aabb_center(const AABB* aabb);

// Get half-extents of AABB
Vec3 aabb_half_extents(const AABB* aabb);

// Create bounding sphere from AABB
BoundingSphere bounding_sphere_from_aabb(const AABB* aabb);

// Create bounding sphere from points
BoundingSphere bounding_sphere_from_points(const Vec3* points, u32 count);

// Test if two AABBs intersect
bool aabb_intersects(const AABB* a, const AABB* b);

// Test if AABB contains point
bool aabb_contains_point(const AABB* aabb, Vec3 point);

#ifdef __cplusplus
}
#endif

#endif // CULLING_H
