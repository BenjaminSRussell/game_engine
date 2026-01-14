/**
 * @file unified_types.h
 * @brief Unified mathematical types for the entire engine
 * @details Provides single source of truth for all vector, quaternion, and matrix types.
 * All types are SIMD-friendly with proper alignment.
 *
 * Architecture:
 * - Vec2: 2D vector (8 bytes, no padding)
 * - Vec3: 3D vector (12 bytes, no padding)
 * - Vec4: 4D vector (16 bytes, SIMD aligned)
 * - Quat: Quaternion (16 bytes, SIMD aligned)
 * - Mat3: 3x3 matrix (36 bytes, column-major)
 * - Mat4: 4x4 matrix (64 bytes, SIMD aligned, column-major)
 *
 * Usage:
 *   #include "core/math/unified_types.h"
 *
 * Dependencies:
 *   - core/simd/simd_types.h (SIMD support)
 */

#ifndef UNIFIED_TYPES_H
#define UNIFIED_TYPES_H

#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ============================================================================
 * VECTOR TYPES
 * ============================================================================
 */

/** 2D vector (8 bytes) */
typedef struct {
    float x, y;
} Vec2;

/** 3D vector (12 bytes) */
typedef struct {
    float x, y, z;
} Vec3;

/** 4D vector (16 bytes, SIMD aligned) */
typedef struct {
    float x, y, z, w;
} __attribute__((aligned(16))) Vec4;

/** Integer 2D vector */
typedef struct {
    int x, y;
} IVec2;

/** Integer 3D vector */
typedef struct {
    int x, y, z;
} IVec3;

/** Integer 4D vector */
typedef struct {
    int x, y, z, w;
} IVec4;

/**
 * ============================================================================
 * QUATERNION TYPE
 * ============================================================================
 */

/** Quaternion: x, y, z, w order (16 bytes, SIMD aligned) */
typedef struct {
    float x, y, z, w;
} __attribute__((aligned(16))) Quat;

/**
 * ============================================================================
 * MATRIX TYPES (Column-Major Order)
 * ============================================================================
 */

/** 3x3 matrix (36 bytes, column-major) */
typedef struct {
    float m[3][3];
} Mat3;

/** 4x4 matrix (64 bytes, column-major, SIMD aligned) */
typedef struct {
    float m[4][4];
} __attribute__((aligned(16))) Mat4;

/**
 * ============================================================================
 * VECTOR OPERATIONS
 * ============================================================================
 */

// Vec2 Operations
static inline Vec2 vec2(float x, float y) {
    return (Vec2){ x, y };
}

static inline Vec2 vec2_zero(void) {
    return (Vec2){ 0.0f, 0.0f };
}

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){ a.x + b.x, a.y + b.y };
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){ a.x - b.x, a.y - b.y };
}

static inline Vec2 vec2_mul(Vec2 v, float s) {
    return (Vec2){ v.x * s, v.y * s };
}

static inline float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static inline float vec2_length_sq(Vec2 v) {
    return vec2_dot(v, v);
}

static inline float vec2_length(Vec2 v) {
    return sqrtf(vec2_length_sq(v));
}

static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    return len > 0.0001f ? vec2_mul(v, 1.0f / len) : vec2_zero();
}

// Vec3 Operations
static inline Vec3 vec3(float x, float y, float z) {
    return (Vec3){ x, y, z };
}

static inline Vec3 vec3_zero(void) {
    return (Vec3){ 0.0f, 0.0f, 0.0f };
}

static inline Vec3 vec3_one(void) {
    return (Vec3){ 1.0f, 1.0f, 1.0f };
}

static inline Vec3 vec3_up(void) {
    return (Vec3){ 0.0f, 1.0f, 0.0f };
}

static inline Vec3 vec3_forward(void) {
    return (Vec3){ 0.0f, 0.0f, 1.0f };
}

static inline Vec3 vec3_right(void) {
    return (Vec3){ 1.0f, 0.0f, 0.0f };
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3 vec3_mul(Vec3 v, float s) {
    return (Vec3){ v.x * s, v.y * s, v.z * s };
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length_sq(Vec3 v) {
    return vec3_dot(v, v);
}

static inline float vec3_length(Vec3 v) {
    return sqrtf(vec3_length_sq(v));
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    return len > 0.0001f ? vec3_mul(v, 1.0f / len) : vec3_zero();
}

static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return vec3_add(vec3_mul(a, 1.0f - t), vec3_mul(b, t));
}

// Vec4 Operations
static inline Vec4 vec4(float x, float y, float z, float w) {
    return (Vec4){ x, y, z, w };
}

static inline Vec4 vec4_zero(void) {
    return (Vec4){ 0.0f, 0.0f, 0.0f, 0.0f };
}

static inline Vec4 vec4_add(Vec4 a, Vec4 b) {
    return (Vec4){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

static inline Vec4 vec4_sub(Vec4 a, Vec4 b) {
    return (Vec4){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

static inline Vec4 vec4_mul(Vec4 v, float s) {
    return (Vec4){ v.x * s, v.y * s, v.z * s, v.w * s };
}

static inline float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/**
 * ============================================================================
 * QUATERNION OPERATIONS
 * ============================================================================
 */

static inline Quat quat(float x, float y, float z, float w) {
    return (Quat){ x, y, z, w };
}

static inline Quat quat_identity(void) {
    return (Quat){ 0.0f, 0.0f, 0.0f, 1.0f };
}

static inline Quat quat_normalize(Quat q) {
    float len_sq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (len_sq < 0.0001f) return quat_identity();
    float len = sqrtf(len_sq);
    return (Quat){ q.x / len, q.y / len, q.z / len, q.w / len };
}

static inline Quat quat_mul(Quat a, Quat b) {
    return (Quat){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

static inline Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    // v' = q * (0, v) * q^-1
    Quat v_quat = (Quat){ v.x, v.y, v.z, 0.0f };
    Quat q_inv = (Quat){ -q.x, -q.y, -q.z, q.w };
    Quat result = quat_mul(quat_mul(q, v_quat), q_inv);
    return (Vec3){ result.x, result.y, result.z };
}

static inline Quat quat_from_axis_angle(Vec3 axis, float angle_rad) {
    float half_angle = angle_rad * 0.5f;
    float sin_half = sinf(half_angle);
    float cos_half = cosf(half_angle);
    Vec3 normalized_axis = vec3_normalize(axis);
    return (Quat){
        normalized_axis.x * sin_half,
        normalized_axis.y * sin_half,
        normalized_axis.z * sin_half,
        cos_half
    };
}

static inline Quat quat_lerp(Quat a, Quat b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }
    float k0 = 1.0f - t;
    float k1 = t;
    Quat result = {
        k0 * a.x + k1 * b.x,
        k0 * a.y + k1 * b.y,
        k0 * a.z + k1 * b.z,
        k0 * a.w + k1 * b.w
    };
    return quat_normalize(result);
}

/**
 * ============================================================================
 * MATRIX OPERATIONS
 * ============================================================================
 */

static inline Mat4 mat4_identity(void) {
    Mat4 m;
    memset(&m, 0, sizeof(Mat4));
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

static inline Mat4 mat4_translation(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[3][0] = t.x;
    m.m[3][1] = t.y;
    m.m[3][2] = t.z;
    return m;
}

static inline Mat4 mat4_scale(Vec3 s) {
    Mat4 m = mat4_identity();
    m.m[0][0] = s.x;
    m.m[1][1] = s.y;
    m.m[2][2] = s.z;
    return m;
}

static inline Mat4 mat4_from_quat(Quat q) {
    Mat4 m = mat4_identity();
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float zw = q.z * q.w;
    float xz = q.x * q.z;
    float yw = q.y * q.w;
    float yz = q.y * q.z;
    float xw = q.x * q.w;

    m.m[0][0] = 1.0f - 2.0f * (yy + zz);
    m.m[1][0] = 2.0f * (xy + zw);
    m.m[2][0] = 2.0f * (xz - yw);

    m.m[0][1] = 2.0f * (xy - zw);
    m.m[1][1] = 1.0f - 2.0f * (xx + zz);
    m.m[2][1] = 2.0f * (yz + xw);

    m.m[0][2] = 2.0f * (xz + yw);
    m.m[1][2] = 2.0f * (yz - xw);
    m.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return m;
}

static inline Mat4 mat4_perspective(float fov_rad, float aspect, float near, float far) {
    Mat4 m;
    memset(&m, 0, sizeof(Mat4));

    float f = 1.0f / tanf(fov_rad * 0.5f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far + near) / (near - far);
    m.m[2][3] = -1.0f;
    m.m[3][2] = (2.0f * far * near) / (near - far);

    return m;
}

static inline Mat4 mat4_orthogonal(float left, float right, float bottom, float top, float near, float far) {
    Mat4 m = mat4_identity();
    m.m[0][0] = 2.0f / (right - left);
    m.m[1][1] = 2.0f / (top - bottom);
    m.m[2][2] = -2.0f / (far - near);
    m.m[3][0] = -(right + left) / (right - left);
    m.m[3][1] = -(top + bottom) / (top - bottom);
    m.m[3][2] = -(far + near) / (far - near);
    return m;
}

static inline Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){
        m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
        m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
        m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
        m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w
    };
}

/**
 * ============================================================================
 * TYPE CONVERSIONS
 * ============================================================================
 */

static inline Vec2 vec3_to_vec2(Vec3 v) {
    return (Vec2){ v.x, v.y };
}

static inline Vec3 vec2_to_vec3(Vec2 v, float z) {
    return (Vec3){ v.x, v.y, z };
}

static inline Vec4 vec3_to_vec4(Vec3 v, float w) {
    return (Vec4){ v.x, v.y, v.z, w };
}

static inline Vec3 vec4_to_vec3(Vec4 v) {
    return (Vec3){ v.x, v.y, v.z };
}

#ifdef __cplusplus
}
#endif

#endif // UNIFIED_TYPES_H
