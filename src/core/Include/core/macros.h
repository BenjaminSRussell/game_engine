#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include <stddef.h>

// Alignment
#if defined(_MSC_VER)
#define K_ALIGN(x) __declspec(align(x))
#else
#define K_ALIGN(x) __attribute__((aligned(x)))
#endif

#ifndef ALIGN
#define ALIGN(x) K_ALIGN(x)
#endif

// Inlining
#ifndef INLINE
#if defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE inline __attribute__((always_inline))
#endif
#endif

// Math utilities
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#endif

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

#ifndef PI
#define PI 3.14159265359f
#endif

// Array size
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Alignment utilities
#define ALIGN_UP(size, alignment)                                              \
  (((size) + (alignment) - 1) & ~((alignment) - 1))
#define ALIGN_DOWN(size, alignment) ((size) & ~((alignment) - 1))

#endif // CORE_MACROS_H
