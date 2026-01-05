/**
 * SIMD Types for Physics Engine
 * 
 * Defines 128-bit aligned vector and matrix types optimized for modern CPU SIMD.
 * These types form the foundation of the Data-Oriented Design physics architecture.
 */

#ifndef SIMD_TYPES_H
#define SIMD_TYPES_H

#include <stdint.h>

// Platform detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define SIMD_PLATFORM_X86
    #if defined(__AVX2__)
        #define SIMD_AVX2
    #elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        #define SIMD_SSE2
    #endif
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #define SIMD_PLATFORM_ARM
    #define SIMD_NEON
#endif

// Alignment macros
#if defined(_MSC_VER)
    #define SIMD_ALIGN(x) __declspec(align(x))
#else
    #define SIMD_ALIGN(x) __attribute__((aligned(x)))
#endif

/**
 * v4f: 128-bit aligned 4-component float vector
 * 
 * Used for 3D vectors (x, y, z) with w=0 for vectors or w=1 for points.
 * The 4th component ensures alignment for SIMD instructions (MOVAPS, etc.)
 */
typedef SIMD_ALIGN(16) struct {
    float x, y, z, w;
} v4f;

/**
 * m33f: 3x3 rotation matrix stored as 3 column vectors
 * 
 * Column-major storage aligns with matrix-vector multiplication:
 * v' = M * v becomes three dot products using SIMD.
 */
typedef struct {
    v4f col0;  // First column (m00, m10, m20)
    v4f col1;  // Second column (m01, m11, m21)
    v4f col2;  // Third column (m02, m12, m22)
} m33f;

/**
 * Helper macros for common vector initializations
 */
#define V4F_ZERO ((v4f){0.0f, 0.0f, 0.0f, 0.0f})
#define V4F_VECTOR(x, y, z) ((v4f){x, y, z, 0.0f})
#define V4F_POINT(x, y, z) ((v4f){x, y, z, 1.0f})

/**
 * Helper macros for identity matrix
 */
#define M33F_IDENTITY ((m33f){ \
    {1.0f, 0.0f, 0.0f, 0.0f}, \
    {0.0f, 1.0f, 0.0f, 0.0f}, \
    {0.0f, 0.0f, 1.0f, 0.0f}  \
})

#endif // SIMD_TYPES_H
