// math/bounds.h
// Bounding box and sphere for spatial queries
#ifndef BOUNDS_H
#define BOUNDS_H

#include "include/math/vec3.h"

typedef struct {
  Vec3 min;
  Vec3 max;
} BoundingBox;

typedef struct {
  Vec3 center;
  f32 radius;
} BoundingSphere;

#ifdef __cplusplus
extern "C" {
#endif

// Bounding Box
static inline BoundingBox bounds_box_create(Vec3 min, Vec3 max) {
  BoundingBox box;
  box.min = min;
  box.max = max;
  return box;
}

static inline Vec3 bounds_box_center(const BoundingBox *box) {
  return vec3((box->min.x + box->max.x) * 0.5f,
              (box->min.y + box->max.y) * 0.5f,
              (box->min.z + box->max.z) * 0.5f);
}

static inline Vec3 bounds_box_extents(const BoundingBox *box) {
  return vec3((box->max.x - box->min.x) * 0.5f,
              (box->max.y - box->min.y) * 0.5f,
              (box->max.z - box->min.z) * 0.5f);
}

// Bounding Sphere
static inline BoundingSphere bounds_sphere_create(Vec3 center, f32 radius) {
  BoundingSphere sphere;
  sphere.center = center;
  sphere.radius = radius;
  return sphere;
}

static inline BoundingSphere bounds_sphere_from_box(const BoundingBox *box) {
  BoundingSphere sphere;
  sphere.center = bounds_box_center(box);
  Vec3 extents = bounds_box_extents(box);
  sphere.radius = vec3_length(extents);
  return sphere;
}

#ifdef __cplusplus
}
#endif

#endif // BOUNDS_H
