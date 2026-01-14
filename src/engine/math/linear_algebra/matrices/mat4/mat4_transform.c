#include "../../include/math/mat4.h"
#include <math.h>

Mat4 mat4_translate(Vec3 v) {
  Mat4 m = mat4_identity();
  m.m30 = v.x;
  m.m31 = v.y;
  m.m32 = v.z;
  return m;
}

Mat4 mat4_scale(Vec3 v) {
  Mat4 m = mat4_identity();
  m.m00 = v.x;
  m.m11 = v.y;
  m.m22 = v.z;
  return m;
}

Mat4 mat4_rotate(Vec3 axis, float angle) {
  Mat4 m = mat4_identity();

  float c = cosf(angle);
  float s = sinf(angle);
  float omc = 1.0f - c;

  float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
  if (len < 0.0001f)
    return m;

  float x = axis.x / len;
  float y = axis.y / len;
  float z = axis.z / len;

  m.m00 = x * x * omc + c;
  m.m01 = y * x * omc + z * s;
  m.m02 = z * x * omc - y * s;

  m.m10 = x * y * omc - z * s;
  m.m11 = y * y * omc + c;
  m.m12 = z * y * omc + x * s;

  m.m20 = x * z * omc + y * s;
  m.m21 = y * z * omc - x * s;
  m.m22 = z * z * omc + c;

  return m;
}

Mat4 mat4_rotate_x(float angle) {
  float c = cosf(angle);
  float s = sinf(angle);

  Mat4 result = mat4_identity();
  result.m11 = c;
  result.m12 = -s;
  result.m21 = s;
  result.m22 = c;

  return result;
}

Mat4 mat4_rotate_y(float angle) {
  float c = cosf(angle);
  float s = sinf(angle);

  Mat4 result = mat4_identity();
  result.m00 = c;
  result.m02 = s;
  result.m20 = -s;
  result.m22 = c;

  return result;
}

Mat4 mat4_rotate_z(float angle) {
  float c = cosf(angle);
  float s = sinf(angle);

  Mat4 result = mat4_identity();
  result.m00 = c;
  result.m01 = -s;
  result.m10 = s;
  result.m11 = c;

  return result;
}
