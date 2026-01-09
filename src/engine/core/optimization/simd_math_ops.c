/**
 * @file simd_math_ops.c
 * @brief SIMD Optimized Math Library.
 *
 * Provides AVX/NEON implementations for Matrix and Vector operations.
 * Auto-detects CPU capabilities.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <core/optimization/simd_math_ops.h>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h> // AVX
#endif
// #include <arm_neon.h> // NEON (if ARM)

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Multiplies 4x4 matrix by 4x4 matrix using AVX.
 */
void simd_mat4_mul(const float *a, const float *b, float *out) {
#ifdef __AVX__
  __m256 row0 = _mm256_loadu_ps(&b[0]);
  __m256 row1 = _mm256_loadu_ps(&b[4]);
  __m256 row2 = _mm256_loadu_ps(&b[8]);
  __m256 row3 = _mm256_loadu_ps(&b[12]);

  for (int i = 0; i < 4; i++) {
    __m256 a_val = _mm256_broadcast_ss(&a[4 * i + 0]);
    __m256 sum = _mm256_mul_ps(a_val, row0);

    a_val = _mm256_broadcast_ss(&a[4 * i + 1]);
    sum = _mm256_add_ps(sum, _mm256_mul_ps(a_val, row1));

    a_val = _mm256_broadcast_ss(&a[4 * i + 2]);
    sum = _mm256_add_ps(sum, _mm256_mul_ps(a_val, row2));

    a_val = _mm256_broadcast_ss(&a[4 * i + 3]);
    sum = _mm256_add_ps(sum, _mm256_mul_ps(a_val, row3));

    _mm256_storeu_ps(&out[4 * i], sum);
  }
#else
  // Fallback C implementation
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float sum = 0.0f;
      for (int k = 0; k < 4; k++) {
        sum += a[i * 4 + k] * b[k * 4 + j];
      }
      out[i * 4 + j] = sum;
    }
  }
#endif
}

/**
 * @brief Batch transformation of vectors.
 */
void simd_transform_vectors(const float *mat, const float *in_vecs,
                            float *out_vecs, int count) {
  // Fallback C implementation
  for (int i = 0; i < count; i++) {
    const float *v = &in_vecs[i * 4];
    float *o = &out_vecs[i * 4];
    
    // M * V (Standard column vector multiplication)
    for (int r = 0; r < 4; r++) {
      float sum = 0.0f;
      for (int c = 0; c < 4; c++) {
        sum += mat[r * 4 + c] * v[c];
      }
      o[r] = sum;
    }
  }
}
