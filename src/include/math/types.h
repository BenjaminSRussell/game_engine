#ifndef MATH_TYPES_H
#define MATH_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Basic type definitions
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

// Vector types
typedef struct {
    f32 x, y;
} Vec2;

typedef struct {
    f32 x, y, z;
} Vec3;

typedef struct {
    f32 x, y, z, w;
} Vec4;

// Matrix types
typedef struct {
    f32 m[16];
} Mat4;

// Quaternion type
typedef struct {
    f32 w, x, y, z;
} Quat;

// AABB type
typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

// Plane type
typedef struct {
    Vec3 normal;
    f32 d;
} Plane;

// Ray type
typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

// Frustum type
typedef struct {
    Plane planes[6];
} Frustum;

// Common constants
#define PI_F 3.14159265358979323846f
#define TWO_PI_F (2.0f * PI_F)
#define HALF_PI_F (PI_F * 0.5f)
#define EPSILON_F 1e-6f

#endif // MATH_TYPES_H
