#ifndef MATH_LA_QUATERNIONS_QUAT_OPS_H
#define MATH_LA_QUATERNIONS_QUAT_OPS_H

#include "quat_types.h"

INLINE Quat quat_create(f32 w, f32 x, f32 y, f32 z) {
  Quat q = {{w, x, y, z}};
  return q;
}

INLINE Quat quat_identity(void) { return quat_create(1.0f, 0.0f, 0.0f, 0.0f); }

#endif // MATH_LA_QUATERNIONS_QUAT_OPS_H
