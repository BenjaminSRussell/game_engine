/**
 * @file quat.h
 * @brief Quaternion mathematics for rotations
 */

#ifndef VOXELFORGE_MATH_QUAT_H
#define VOXELFORGE_MATH_QUAT_H

#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef struct Quat {
  f32 x, y, z, w;
} Quat;

// ============================================================================
// Construction
// ============================================================================

VF_INLINE Quat quat_identity(void) { return (Quat){0.0f, 0.0f, 0.0f, 1.0f}; }

VF_INLINE Quat quat_create(f32 x, f32 y, f32 z, f32 w) {
  return (Quat){x, y, z, w};
}

VF_INLINE Quat quat_from_axis_angle(Vec3 axis, f32 radians) {
  f32 half_angle = radians * 0.5f;
  f32 s = sinf(half_angle);
  Vec3 n = vec3_normalize(axis);
  return (Quat){n.x * s, n.y * s, n.z * s, cosf(half_angle)};
}

VF_INLINE Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll) {
  f32 cy = cosf(yaw * 0.5f);
  f32 sy = sinf(yaw * 0.5f);
  f32 cp = cosf(pitch * 0.5f);
  f32 sp = sinf(pitch * 0.5f);
  f32 cr = cosf(roll * 0.5f);
  f32 sr = sinf(roll * 0.5f);

  return (Quat){sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
                cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy};
}

// ============================================================================
// Operations
// ============================================================================

VF_INLINE f32 quat_dot(Quat a, Quat b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

VF_INLINE f32 quat_length_sq(Quat q) { return quat_dot(q, q); }

VF_INLINE f32 quat_length(Quat q) { return sqrtf(quat_length_sq(q)); }

VF_INLINE Quat quat_normalize(Quat q) {
  f32 len = quat_length(q);
  if (len > 0.0001f) {
    f32 inv_len = 1.0f / len;
    return (Quat){q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len};
  }
  return quat_identity();
}

VF_INLINE Quat quat_conjugate(Quat q) { return (Quat){-q.x, -q.y, -q.z, q.w}; }

VF_INLINE Quat quat_inverse(Quat q) {
  f32 len_sq = quat_length_sq(q);
  if (len_sq > 0.0001f) {
    f32 inv = 1.0f / len_sq;
    return (Quat){-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv};
  }
  return quat_identity();
}

VF_INLINE Quat quat_mul(Quat a, Quat b) {
  return (Quat){a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

VF_INLINE Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
  Vec3 qv = {q.x, q.y, q.z};
  Vec3 uv = vec3_cross(qv, v);
  Vec3 uuv = vec3_cross(qv, uv);
  uv = vec3_scale(uv, 2.0f * q.w);
  uuv = vec3_scale(uuv, 2.0f);
  return vec3_add(vec3_add(v, uv), uuv);
}

// ============================================================================
// Interpolation
// ============================================================================

VF_INLINE Quat quat_lerp(Quat a, Quat b, f32 t) {
  return quat_normalize((Quat){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                               a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t});
}

VF_INLINE Quat quat_slerp(Quat a, Quat b, f32 t) {
  f32 dot = quat_dot(a, b);

  // Take shorter path
  if (dot < 0.0f) {
    b = (Quat){-b.x, -b.y, -b.z, -b.w};
    dot = -dot;
  }

  // Use lerp for nearly parallel quaternions
  if (dot > 0.9995f) {
    return quat_lerp(a, b, t);
  }

  f32 theta = acosf(dot);
  f32 sin_theta = sinf(theta);
  f32 wa = sinf((1.0f - t) * theta) / sin_theta;
  f32 wb = sinf(t * theta) / sin_theta;

  return (Quat){a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb,
                a.w * wa + b.w * wb};
}

// ============================================================================
// Conversion
// ============================================================================

VF_INLINE Mat4 quat_to_mat4(Quat q) {
  f32 xx = q.x * q.x;
  f32 yy = q.y * q.y;
  f32 zz = q.z * q.z;
  f32 xy = q.x * q.y;
  f32 xz = q.x * q.z;
  f32 yz = q.y * q.z;
  f32 wx = q.w * q.x;
  f32 wy = q.w * q.y;
  f32 wz = q.w * q.z;

  Mat4 result = mat4_identity();
  result.m[0] = 1.0f - 2.0f * (yy + zz);
  result.m[1] = 2.0f * (xy + wz);
  result.m[2] = 2.0f * (xz - wy);
  result.m[4] = 2.0f * (xy - wz);
  result.m[5] = 1.0f - 2.0f * (xx + zz);
  result.m[6] = 2.0f * (yz + wx);
  result.m[8] = 2.0f * (xz + wy);
  result.m[9] = 2.0f * (yz - wx);
  result.m[10] = 1.0f - 2.0f * (xx + yy);

  return result;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MATH_QUAT_H
