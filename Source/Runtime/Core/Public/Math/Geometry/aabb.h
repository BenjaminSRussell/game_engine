/**
 * @file aabb.h
 * @brief Axis-Aligned Bounding Box
 */

#ifndef VOXELFORGE_GEOMETRY_AABB_H
#define VOXELFORGE_GEOMETRY_AABB_H

#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef struct AABB {
  Vec3 min;
  Vec3 max;
} AABB;

// ============================================================================
// Construction
// ============================================================================

VF_INLINE AABB aabb_create(Vec3 min, Vec3 max) { return (AABB){min, max}; }

VF_INLINE AABB aabb_from_center_extents(Vec3 center, Vec3 extents) {
  return (AABB){vec3_sub(center, extents), vec3_add(center, extents)};
}

VF_INLINE AABB aabb_empty(void) {
  return (AABB){{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
}

// ============================================================================
// Properties
// ============================================================================

VF_INLINE Vec3 aabb_center(AABB a) {
  return vec3_scale(vec3_add(a.min, a.max), 0.5f);
}

VF_INLINE Vec3 aabb_extents(AABB a) {
  return vec3_scale(vec3_sub(a.max, a.min), 0.5f);
}

VF_INLINE Vec3 aabb_size(AABB a) { return vec3_sub(a.max, a.min); }

VF_INLINE f32 aabb_volume(AABB a) {
  Vec3 size = aabb_size(a);
  return size.x * size.y * size.z;
}

VF_INLINE f32 aabb_surface_area(AABB a) {
  Vec3 size = aabb_size(a);
  return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
}

// ============================================================================
// Operations
// ============================================================================

VF_INLINE AABB aabb_expand(AABB a, Vec3 point) {
  return (AABB){vec3_min(a.min, point), vec3_max(a.max, point)};
}

VF_INLINE AABB aabb_merge(AABB a, AABB b) {
  return (AABB){vec3_min(a.min, b.min), vec3_max(a.max, b.max)};
}

VF_INLINE AABB aabb_grow(AABB a, f32 amount) {
  Vec3 expansion = vec3_splat(amount);
  return (AABB){vec3_sub(a.min, expansion), vec3_add(a.max, expansion)};
}

VF_INLINE AABB aabb_translate(AABB a, Vec3 offset) {
  return (AABB){vec3_add(a.min, offset), vec3_add(a.max, offset)};
}

// ============================================================================
// Tests
// ============================================================================

VF_INLINE b8 aabb_contains_point(AABB a, Vec3 p) {
  return p.x >= a.min.x && p.x <= a.max.x && p.y >= a.min.y && p.y <= a.max.y &&
         p.z >= a.min.z && p.z <= a.max.z;
}

VF_INLINE b8 aabb_contains_aabb(AABB a, AABB b) {
  return b.min.x >= a.min.x && b.max.x <= a.max.x && b.min.y >= a.min.y &&
         b.max.y <= a.max.y && b.min.z >= a.min.z && b.max.z <= a.max.z;
}

VF_INLINE b8 aabb_intersects(AABB a, AABB b) {
  return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y &&
         a.max.y >= b.min.y && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

VF_INLINE b8 aabb_is_valid(AABB a) {
  return a.min.x <= a.max.x && a.min.y <= a.max.y && a.min.z <= a.max.z;
}

// ============================================================================
// Ray Intersection
// ============================================================================

VF_INLINE b8 aabb_ray_intersect(AABB a, Vec3 origin, Vec3 inv_dir, f32 *t_near,
                                f32 *t_far) {
  Vec3 t1 = vec3_mul(vec3_sub(a.min, origin), inv_dir);
  Vec3 t2 = vec3_mul(vec3_sub(a.max, origin), inv_dir);

  Vec3 t_min = vec3_min(t1, t2);
  Vec3 t_max = vec3_max(t1, t2);

  f32 near = VF_MAX(VF_MAX(t_min.x, t_min.y), t_min.z);
  f32 far = VF_MIN(VF_MIN(t_max.x, t_max.y), t_max.z);

  if (t_near)
    *t_near = near;
  if (t_far)
    *t_far = far;

  return near <= far && far >= 0.0f;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_GEOMETRY_AABB_H
