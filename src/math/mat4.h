#ifndef RENDERING_MAT4_H
#define RENDERING_MAT4_H

#include "common.h"
#include "math/vec3.h"
#include "math/vec4.h"

// Column-major 4x4 matrix
typedef union ALIGN(16) mat4 {
    float m[16];
    float e[4][4]; // [col][row]
#ifdef __SSE__
    __m128 columns[4];
#endif
} mat4_t;

mat4_t mat4_identity(void);
mat4_t mat4_zero(void);
mat4_t mat4_mul(mat4_t a, mat4_t b);
mat4_t mat4_transpose(mat4_t m);
mat4_t mat4_inverse(mat4_t m);

// Transformations
mat4_t mat4_translate(float x, float y, float z);
mat4_t mat4_scale(float x, float y, float z);
mat4_t mat4_rotate(float angle, float x, float y, float z);

// Projections
mat4_t mat4_perspective(float fov_y, float aspect, float near_z, float far_z);
mat4_t mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z);
mat4_t mat4_look_at(vec3_t eye, vec3_t center, vec3_t up);

// Vector transformations
vec4_t mat4_mul_vec4(mat4_t m, vec4_t v);
vec3_t mat4_mul_vec3(mat4_t m, vec3_t v, float w);

#endif // RENDERING_MAT4_H
