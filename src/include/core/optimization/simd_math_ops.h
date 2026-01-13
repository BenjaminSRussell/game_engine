#ifndef SIMD_MATH_OPS_H
#define SIMD_MATH_OPS_H

void simd_mat4_mul(const float *a, const float *b, float *out);
void simd_transform_vectors(const float *mat, const float *in_vecs, float *out_vecs, int count);

#endif // SIMD_MATH_OPS_H
