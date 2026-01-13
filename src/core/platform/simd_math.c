#include "core/simd/simd_math.h"
#include <include/math/math.h>

void simd_vec4_add(const float a[4], const float b[4], float result[4]) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
    result[3] = a[3] + b[3];
}

void simd_vec4_mul(const float a[4], const float b[4], float result[4]) {
    result[0] = a[0] * b[0];
    result[1] = a[1] * b[1];
    result[2] = a[2] * b[2];
    result[3] = a[3] * b[3];
}

float simd_vec4_dot(const float a[4], const float b[4]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}
