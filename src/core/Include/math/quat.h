// math/quat.h
// Facade header referencing new granular implementation
#ifndef QUAT_H
#define QUAT_H

#include <common.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../../math/linear_algebra/quaternions/quat/quat_ops.h"
#include "../../math/linear_algebra/quaternions/quat/quat_types.h"

Quat quat_from_axis_angle(Vec3 axis, f32 angle);
Quat quat_look_rotation(Vec3 direction, Vec3 up);
Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll);
Quat quat_mul(Quat a, Quat b);
Quat quat_normalize(Quat q);
Quat quat_conjugate(Quat q);
Quat quat_inverse(Quat q);
Vec3 quat_rotate_vec3(Quat q, Vec3 v);
Mat4 quat_to_mat4(Quat q);
Quat quat_slerp(Quat a, Quat b, f32 t);
Quat quat_nlerp(Quat a, Quat b, f32 t);
f32 quat_dot(Quat a, Quat b);
f32 quat_length_sq(Quat q);
f32 quat_length(Quat q);

// Cache/Stats/Validation omitted for brevity in facade if not implemented

#endif // QUAT_H
