/**
 * @file mat4.h
 * @brief 4x4 Matrix mathematics
 */

#ifndef VOXELFORGE_MATH_MAT4_H
#define VOXELFORGE_MATH_MAT4_H

#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/Math/Vector/vec4.h"
#include "Core/Public/core_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

// Column-major 4x4 matrix (OpenGL convention)
typedef struct Mat4 {
  f32 m[16]; // Column-major: m[col * 4 + row]
} Mat4;

// ============================================================================
// Access Helpers
// ============================================================================

#define MAT4_AT(mat, row, col) ((mat).m[(col) * 4 + (row)])

// ============================================================================
// Construction
// ============================================================================

VF_INLINE Mat4 mat4_identity(void) {
  Mat4 result = {0};
  result.m[0] = 1.0f;
  result.m[5] = 1.0f;
  result.m[10] = 1.0f;
  result.m[15] = 1.0f;
  return result;
}

VF_INLINE Mat4 mat4_zero(void) {
  Mat4 result = {0};
  return result;
}

VF_INLINE Mat4 mat4_diagonal(f32 d) {
  Mat4 result = {0};
  result.m[0] = d;
  result.m[5] = d;
  result.m[10] = d;
  result.m[15] = d;
  return result;
}

// ============================================================================
// Transform Construction
// ============================================================================

VF_INLINE Mat4 mat4_translate(Vec3 t) {
  Mat4 result = mat4_identity();
  result.m[12] = t.x;
  result.m[13] = t.y;
  result.m[14] = t.z;
  return result;
}

VF_INLINE Mat4 mat4_scale(Vec3 s) {
  Mat4 result = mat4_identity();
  result.m[0] = s.x;
  result.m[5] = s.y;
  result.m[10] = s.z;
  return result;
}

VF_INLINE Mat4 mat4_rotate_x(f32 radians) {
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  Mat4 result = mat4_identity();
  result.m[5] = c;
  result.m[6] = s;
  result.m[9] = -s;
  result.m[10] = c;
  return result;
}

VF_INLINE Mat4 mat4_rotate_y(f32 radians) {
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  Mat4 result = mat4_identity();
  result.m[0] = c;
  result.m[2] = -s;
  result.m[8] = s;
  result.m[10] = c;
  return result;
}

VF_INLINE Mat4 mat4_rotate_z(f32 radians) {
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  Mat4 result = mat4_identity();
  result.m[0] = c;
  result.m[1] = s;
  result.m[4] = -s;
  result.m[5] = c;
  return result;
}

// ============================================================================
// Matrix Operations
// ============================================================================

VF_INLINE Mat4 mat4_mul(Mat4 a, Mat4 b) {
  Mat4 result = {0};
  for (i32 col = 0; col < 4; col++) {
    for (i32 row = 0; row < 4; row++) {
      f32 sum = 0.0f;
      for (i32 k = 0; k < 4; k++) {
        sum += a.m[k * 4 + row] * b.m[col * 4 + k];
      }
      result.m[col * 4 + row] = sum;
    }
  }
  return result;
}

VF_INLINE Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
  return (Vec4){m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w,
                m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w,
                m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
                m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w};
}

VF_INLINE Vec3 mat4_mul_point(Mat4 m, Vec3 p) {
  Vec4 v = mat4_mul_vec4(m, vec4_from_vec3(p, 1.0f));
  return (Vec3){v.x / v.w, v.y / v.w, v.z / v.w};
}

VF_INLINE Vec3 mat4_mul_direction(Mat4 m, Vec3 d) {
  Vec4 v = mat4_mul_vec4(m, vec4_from_vec3(d, 0.0f));
  return (Vec3){v.x, v.y, v.z};
}

VF_INLINE Mat4 mat4_transpose(Mat4 m) {
  Mat4 result;
  for (i32 col = 0; col < 4; col++) {
    for (i32 row = 0; row < 4; row++) {
      result.m[col * 4 + row] = m.m[row * 4 + col];
    }
  }
  return result;
}

// ============================================================================
// View/Projection
// ============================================================================

VF_INLINE Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
  Vec3 f = vec3_normalize(vec3_sub(target, eye));
  Vec3 r = vec3_normalize(vec3_cross(f, up));
  Vec3 u = vec3_cross(r, f);

  Mat4 result = mat4_identity();
  result.m[0] = r.x;
  result.m[1] = u.x;
  result.m[2] = -f.x;
  result.m[4] = r.y;
  result.m[5] = u.y;
  result.m[6] = -f.y;
  result.m[8] = r.z;
  result.m[9] = u.z;
  result.m[10] = -f.z;
  result.m[12] = -vec3_dot(r, eye);
  result.m[13] = -vec3_dot(u, eye);
  result.m[14] = vec3_dot(f, eye);

  return result;
}

VF_INLINE Mat4 mat4_perspective(f32 fov_radians, f32 aspect, f32 near,
                                f32 far) {
  f32 tan_half_fov = tanf(fov_radians * 0.5f);

  Mat4 result = mat4_zero();
  result.m[0] = 1.0f / (aspect * tan_half_fov);
  result.m[5] = 1.0f / tan_half_fov;
  result.m[10] = -(far + near) / (far - near);
  result.m[11] = -1.0f;
  result.m[14] = -(2.0f * far * near) / (far - near);

  return result;
}

VF_INLINE Mat4 mat4_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                          f32 far) {
  Mat4 result = mat4_identity();
  result.m[0] = 2.0f / (right - left);
  result.m[5] = 2.0f / (top - bottom);
  result.m[10] = -2.0f / (far - near);
  result.m[12] = -(right + left) / (right - left);
  result.m[13] = -(top + bottom) / (top - bottom);
  result.m[14] = -(far + near) / (far - near);

  return result;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MATH_MAT4_H
