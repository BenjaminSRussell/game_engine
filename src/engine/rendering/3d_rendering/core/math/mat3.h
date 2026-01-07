#ifndef RENDERING_MAT3_H
#define RENDERING_MAT3_H

#include "vec3.h"

typedef struct mat3 {
    float m[9];
    float e[3][3]; // [col][row]
} mat3_t;

mat3_t mat3_identity(void);
mat3_t mat3_zero(void);
mat3_t mat3_mul(mat3_t a, mat3_t b);
mat3_t mat3_transpose(mat3_t m);
mat3_t mat3_inverse(mat3_t m);

vec3_t mat3_mul_vec3(mat3_t m, vec3_t v);

#endif // RENDERING_MAT3_H
