// math/vec2.h
//
// Purpose: Defines the `Vec2` structure for 2D vectors and provides a concise
// API for fundamental 2D linear algebra operations. This header is essential
// for representing and manipulating positions, directions, and other 2D
// quantities within the game engine, particularly for UI, 2D physics, or
// horizontal coordinates.
//
// Public APIs:
// - `Vec2`: A union representing a 2-component floating-point vector, allowing
// access
//   via named members (`x`, `y`) or a 1D array (`data`). It is 8-byte aligned
//   for performance.
// - `vec2`: A convenience constructor for creating a `Vec2` from two float
// values.
// - `vec2_zero`: Returns a zero vector (0.0, 0.0).
// - `vec2_add`: Performs vector addition.
// - `vec2_sub`: Performs vector subtraction.
// - `vec2_mul`: Multiplies a vector by a scalar.
// - `vec2_dot`: Calculates the dot product of two vectors.
// - `vec2_length_sq`: Calculates the squared Euclidean length (magnitude) of a
// vector.
// - `vec2_length`: Calculates the Euclidean length (magnitude) of a vector.
//
// Ownership: `Vec2` instances are value types. Functions return new `Vec2`
// instances but do not manage dynamic memory.
//
// Invariants:
// - Vectors are assumed to be 2-component floating-point vectors.
// - Operations like `vec2_length` rely on standard `sqrtf` from `math.h`.
// - The `K_ALIGN(8)` attribute suggests optimization for 8-byte boundaries.
//
#ifndef VEC2_H
#define VEC2_H

#include <common.h>
#include <math.h>

// Use unified type from core/math/types.h
typedef vec2 Vec2;

typedef Vec2 vec2_t;

INLINE Vec2 vec2_create(f32 x, f32 y) {
  Vec2 v = {{x, y}};
  return v;
}

INLINE Vec2 vec2_zero(void) { return vec2_create(0.0f, 0.0f); }

INLINE Vec2 vec2_add(Vec2 a, Vec2 b) {
  return vec2_create(a.x + b.x, a.y + b.y);
}

INLINE Vec2 vec2_sub(Vec2 a, Vec2 b) {
  return vec2_create(a.x - b.x, a.y - b.y);
}

INLINE Vec2 vec2_mul(Vec2 v, f32 s) { return vec2_create(v.x * s, v.y * s); }

INLINE Vec2 vec2_scale_componentwise(Vec2 a, Vec2 b) {
  return vec2_create(a.x * b.x, a.y * b.y);
}

INLINE f32 vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

INLINE f32 vec2_length_sq(Vec2 v) { return vec2_dot(v, v); }

INLINE f32 vec2_length(Vec2 v) { return sqrtf(vec2_length_sq(v)); }

INLINE f32 vec2_distance(Vec2 a, Vec2 b) { return vec2_length(vec2_sub(a, b)); }

#endif // VEC2_H
