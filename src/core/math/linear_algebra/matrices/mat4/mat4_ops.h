#ifndef MATH_LA_MATRICES_MAT4_OPS_H
#define MATH_LA_MATRICES_MAT4_OPS_H

#include "mat4_types.h"

// Convenience macro for creating Mat4 (compound literal, column-major)
#define mat4(m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03,  \
             m13, m23, m33)                                                    \
  ((Mat4){.m = {{m00, m10, m20, m30},                                          \
                {m01, m11, m21, m31},                                          \
                {m02, m12, m22, m32},                                          \
                {m03, m13, m23, m33}}})

INLINE Mat4 mat4_identity(void) {
  Mat4 m = {0};
  m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
  return m;
}

INLINE Mat4 mat4_zero(void) {
  Mat4 m = {0};
  return m;
}

#endif // MATH_LA_MATRICES_MAT4_OPS_H
