#pragma once

#include <stdbool.h>
#include <stdint.h>

// Unsigned integers
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed integers
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Floating point
typedef float f32;
typedef double f64;

// Entity handle ID
typedef u32 EntityID;

// Boolean
#ifndef __cplusplus
#ifndef bool
// typedef _Bool bool; // Standard stdbool.h already handles this
#endif
#endif

// Utility macros
#define BIT(x) (1 << (x))

#if defined(_MSC_VER)
#define INLINE __forceinline
#define NOINLINE __declspec(noinline)
#else
#define INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#endif
