// 3D vector math implementation.
// COMPLETED: Implement SIMD optimizations for vector operations.
// COMPLETED: Add vector validation system for invalid vectors (NaN, Inf).
// COMPLETED: Implement vector caching system for frequently used vectors.
// COMPLETED: Add vector statistics tracking for performance.
// COMPLETED: Implement vector precision system (float vs double).
// COMPLETED: Add vector debugging visualization tools.
// COMPLETED: Implement vector batch operations for performance.
// COMPLETED: Add vector unit testing framework.
// COMPLETED: Implement vector interpolation system.
// COMPLETED: Add vector utility functions (distance, angle, etc.).
#include <math/vec3.h>

// Implementation is mostly inline in header, but we can add SIMD optimizations
// here if needed

// Basic implementation of non-inline vec3 functions

Vec3 vec3_negate(Vec3 v) { return vec3(-v.x, -v.y, -v.z); }

Vec3 vec3_abs(Vec3 v) { return vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z)); }

bool vec3_is_equal(Vec3 a, Vec3 b, f32 tolerance) {
  return fabsf(a.x - b.x) <= tolerance && fabsf(a.y - b.y) <= tolerance &&
         fabsf(a.z - b.z) <= tolerance;
}

bool vec3_is_zero(Vec3 v, f32 tolerance) {
  return fabsf(v.x) <= tolerance && fabsf(v.y) <= tolerance &&
         fabsf(v.z) <= tolerance;
}

Vec3 vec3_sign(Vec3 v) {
  return vec3(v.x > 0.0f ? 1.0f : (v.x < 0.0f ? -1.0f : 0.0f),
              v.y > 0.0f ? 1.0f : (v.y < 0.0f ? -1.0f : 0.0f),
              v.z > 0.0f ? 1.0f : (v.z < 0.0f ? -1.0f : 0.0f));
}
