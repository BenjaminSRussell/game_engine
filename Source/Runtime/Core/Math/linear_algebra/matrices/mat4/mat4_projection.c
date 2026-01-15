#include "../../include/math/mat4.h"
#include <math.h>

Mat4 mat4_ortho(float left, float right, float bottom, float top, float near_z,
                float far_z) {
  Mat4 m = mat4_zero();

  m.m00 = 2.0f / (right - left);
  m.m11 = 2.0f / (top - bottom);
  m.m22 = -2.0f / (far_z - near_z);

  m.m30 = -(right + left) / (right - left);
  m.m31 = -(top + bottom) / (top - bottom);
  m.m32 = -(far_z + near_z) / (far_z - near_z);
  m.m33 = 1.0f;

  return m;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
  Vec3 f = vec3_normalize(vec3_sub(target, eye));
  Vec3 s = vec3_normalize(vec3_cross(f, up));
  Vec3 u = vec3_cross(s, f);

  Mat4 result = mat4_identity();
  result.m00 = s.x;
  result.m01 = s.y;
  result.m02 = s.z;
  result.m10 = u.x;
  result.m11 = u.y;
  result.m12 = u.z;
  result.m20 = -f.x;
  result.m21 = -f.y;
  result.m22 = -f.z;
  result.m30 = -vec3_dot(s, eye);
  result.m31 = -vec3_dot(u, eye);
  result.m32 = vec3_dot(f, eye);

  return result;
}

Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
  float tan_half_fov = tanf(fov * 0.5f);
  float range = near - far;

  Mat4 result = mat4_zero();
  result.m00 = 1.0f / (aspect * tan_half_fov);
  result.m11 = 1.0f / tan_half_fov;
  result.m22 = (near + far) / range;
  result.m23 = -1.0f;
  result.m32 = (2.0f * near * far) / range;

  return result;
}
