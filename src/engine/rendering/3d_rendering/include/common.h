/*
 * common.h
 * Common definitions for 3D rendering engine
 */

#ifndef RENDERING_COMMON_H
#define RENDERING_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * COMPILER ATTRIBUTES
 * ============================================================================ */

#if defined(__GNUC__) || defined(__clang__)
    #define ALIGN(x) __attribute__((aligned(x)))
    #define PACKED __attribute__((packed))
    #define INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define ALIGN(x) __declspec(align(x))
    #define PACKED
    #define INLINE __forceinline
#else
    #define ALIGN(x)
    #define PACKED
    #define INLINE inline
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define EPSILON 1e-6f
#define PI 3.14159265358979323846f
#define TWO_PI (2.0f * PI)
#define HALF_PI (PI / 2.0f)

/* ============================================================================
 * UTILITY MACROS
 * ============================================================================ */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX(x, min), max))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* RENDERING_COMMON_H */
