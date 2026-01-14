/**
 * @file frustum.h
 * @brief View frustum for culling
 */

#ifndef VOXELFORGE_GEOMETRY_FRUSTUM_H
#define VOXELFORGE_GEOMETRY_FRUSTUM_H

#include "Core/Public/Math/Geometry/aabb.h"
#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/Math/Vector/vec4.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef enum FrustumPlane {
  FRUSTUM_PLANE_LEFT = 0,
  FRUSTUM_PLANE_RIGHT = 1,
  FRUSTUM_PLANE_BOTTOM = 2,
  FRUSTUM_PLANE_TOP = 3,
  FRUSTUM_PLANE_NEAR = 4,
  FRUSTUM_PLANE_FAR = 5,
  FRUSTUM_PLANE_COUNT = 6
} FrustumPlane;

typedef struct Plane {
  Vec3 normal;
  f32 distance;
} Plane;

typedef struct Frustum {
  Plane planes[FRUSTUM_PLANE_COUNT];
} Frustum;

// ============================================================================
// Plane Operations
// ============================================================================

VF_INLINE Plane plane_create(Vec3 normal, f32 distance) {
  return (Plane){vec3_normalize(normal), distance};
}

VF_INLINE Plane plane_from_points(Vec3 a, Vec3 b, Vec3 c) {
  Vec3 ab = vec3_sub(b, a);
  Vec3 ac = vec3_sub(c, a);
  Vec3 n = vec3_normalize(vec3_cross(ab, ac));
  return (Plane){n, vec3_dot(n, a)};
}

VF_INLINE f32 plane_distance_to_point(Plane p, Vec3 point) {
  return vec3_dot(p.normal, point) - p.distance;
}

VF_INLINE b8 plane_is_point_in_front(Plane p, Vec3 point) {
  return plane_distance_to_point(p, point) > 0.0f;
}

// ============================================================================
// Frustum Construction
// ============================================================================

VF_INLINE Frustum frustum_from_view_proj(Mat4 vp) {
  Frustum f;

  // Left: row 4 + row 1
  f.planes[FRUSTUM_PLANE_LEFT].normal.x = vp.m[3] + vp.m[0];
  f.planes[FRUSTUM_PLANE_LEFT].normal.y = vp.m[7] + vp.m[4];
  f.planes[FRUSTUM_PLANE_LEFT].normal.z = vp.m[11] + vp.m[8];
  f.planes[FRUSTUM_PLANE_LEFT].distance = -(vp.m[15] + vp.m[12]);

  // Right: row 4 - row 1
  f.planes[FRUSTUM_PLANE_RIGHT].normal.x = vp.m[3] - vp.m[0];
  f.planes[FRUSTUM_PLANE_RIGHT].normal.y = vp.m[7] - vp.m[4];
  f.planes[FRUSTUM_PLANE_RIGHT].normal.z = vp.m[11] - vp.m[8];
  f.planes[FRUSTUM_PLANE_RIGHT].distance = -(vp.m[15] - vp.m[12]);

  // Bottom: row 4 + row 2
  f.planes[FRUSTUM_PLANE_BOTTOM].normal.x = vp.m[3] + vp.m[1];
  f.planes[FRUSTUM_PLANE_BOTTOM].normal.y = vp.m[7] + vp.m[5];
  f.planes[FRUSTUM_PLANE_BOTTOM].normal.z = vp.m[11] + vp.m[9];
  f.planes[FRUSTUM_PLANE_BOTTOM].distance = -(vp.m[15] + vp.m[13]);

  // Top: row 4 - row 2
  f.planes[FRUSTUM_PLANE_TOP].normal.x = vp.m[3] - vp.m[1];
  f.planes[FRUSTUM_PLANE_TOP].normal.y = vp.m[7] - vp.m[5];
  f.planes[FRUSTUM_PLANE_TOP].normal.z = vp.m[11] - vp.m[9];
  f.planes[FRUSTUM_PLANE_TOP].distance = -(vp.m[15] - vp.m[13]);

  // Near: row 4 + row 3
  f.planes[FRUSTUM_PLANE_NEAR].normal.x = vp.m[3] + vp.m[2];
  f.planes[FRUSTUM_PLANE_NEAR].normal.y = vp.m[7] + vp.m[6];
  f.planes[FRUSTUM_PLANE_NEAR].normal.z = vp.m[11] + vp.m[10];
  f.planes[FRUSTUM_PLANE_NEAR].distance = -(vp.m[15] + vp.m[14]);

  // Far: row 4 - row 3
  f.planes[FRUSTUM_PLANE_FAR].normal.x = vp.m[3] - vp.m[2];
  f.planes[FRUSTUM_PLANE_FAR].normal.y = vp.m[7] - vp.m[6];
  f.planes[FRUSTUM_PLANE_FAR].normal.z = vp.m[11] - vp.m[10];
  f.planes[FRUSTUM_PLANE_FAR].distance = -(vp.m[15] - vp.m[14]);

  // Normalize all planes
  for (i32 i = 0; i < FRUSTUM_PLANE_COUNT; i++) {
    f32 len = vec3_length(f.planes[i].normal);
    if (len > 0.0001f) {
      f32 inv_len = 1.0f / len;
      f.planes[i].normal = vec3_scale(f.planes[i].normal, inv_len);
      f.planes[i].distance *= inv_len;
    }
  }

  return f;
}

// ============================================================================
// Frustum Tests
// ============================================================================

VF_INLINE b8 frustum_contains_point(Frustum f, Vec3 point) {
  for (i32 i = 0; i < FRUSTUM_PLANE_COUNT; i++) {
    if (plane_distance_to_point(f.planes[i], point) < 0.0f) {
      return false;
    }
  }
  return true;
}

VF_INLINE b8 frustum_intersects_aabb(Frustum f, AABB box) {
  for (i32 i = 0; i < FRUSTUM_PLANE_COUNT; i++) {
    Plane p = f.planes[i];

    // Find positive vertex
    Vec3 positive = box.min;
    if (p.normal.x >= 0)
      positive.x = box.max.x;
    if (p.normal.y >= 0)
      positive.y = box.max.y;
    if (p.normal.z >= 0)
      positive.z = box.max.z;

    if (plane_distance_to_point(p, positive) < 0.0f) {
      return false;
    }
  }
  return true;
}

VF_INLINE b8 frustum_intersects_sphere(Frustum f, Vec3 center, f32 radius) {
  for (i32 i = 0; i < FRUSTUM_PLANE_COUNT; i++) {
    if (plane_distance_to_point(f.planes[i], center) < -radius) {
      return false;
    }
  }
  return true;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_GEOMETRY_FRUSTUM_H
