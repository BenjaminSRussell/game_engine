#ifndef RENDERING_QUAT_H
#define RENDERING_QUAT_H

#include "math/vec3.h"
#include "math/mat4.h"

typedef struct quat {
    float x, y, z, w;
} quat_t;

quat_t quat_identity(void);
quat_t quat_from_axis_angle(vec3_t axis, float angle);
quat_t quat_mul(quat_t a, quat_t b);
quat_t quat_normalize(quat_t q);
quat_t quat_lerp(quat_t a, quat_t b, float t);
quat_t quat_slerp(quat_t a, quat_t b, float t);

mat4_t quat_to_mat4(quat_t q);

#endif // RENDERING_QUAT_H
