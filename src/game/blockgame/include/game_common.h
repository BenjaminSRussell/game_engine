// include/common.h
//
// Purpose: This header file centralizes core type definitions, common macros,
// and global constants used across the entire game engine. It serves as a
// foundational include for nearly all other C/C++ source files, providing
// essential utilities and configuration parameters.
//
// Public APIs:
// - Standard C library includes (`stdint.h`, `stdbool.h`, `math.h`, etc.).
// - Platform detection macros (`PLATFORM_WEB`).
// - Compiler optimization hints (`INLINE`, `LIKELY`, `UNLIKELY`, `ALIGN`,
// `PACKED`).
// - Global constants such as `CHUNK_SIZE`, `MAX_CHUNKS_LOADED`, `BLOCK_SIZE`,
// and various math constants.
// - Type aliases for fixed-width integers (`u8`, `u16`, `i32`, etc.) and
// floating-point numbers (`f32`, `f64`).
// - Custom type definitions for `BlockID`, `EntityID`, and `ComponentTypeID`.
// - Utility macros for common operations: `MIN`, `MAX`, `CLAMP`, `ABS`, `SIGN`,
// `LERP`, `SWAP`, `ARRAY_SIZE`.
// - Debugging macros: `ASSERT` for conditional assertions and basic `LOG_INFO`,
// `LOG_WARN`, `LOG_ERROR` stubs.
//
// Ownership: This file defines global constants and macros; it does not "own"
// data in the traditional sense. Its contents are globally available to any
// file that includes it.
//
// Invariants:
// - `CHUNK_SIZE` must be a power of 2 for efficient bitwise operations in
// `chunk_block_index`.
// - All type aliases are standard-compliant and should maintain their size
// across platforms.
// - The `DEBUG` macro controls the behavior of assertions and logging, ensuring
// they are stripped
//   in release builds.
// - Math constants like `PI` are defined with high precision.
//
#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Platform detection
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define PLATFORM_WEB 1
#else
#define PLATFORM_WEB 0
#endif

// Compiler hints for optimization
#define INLINE inline __attribute__((always_inline))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#ifndef ALIGN
#define ALIGN(n) __attribute__((aligned(n)))
#endif
#define PACKED __attribute__((packed))

// Memory alignment
#define ALIGN_16 16
#define ALIGN_32 32
#define ALIGN_64 64

// Constants
#define CHUNK_SIZE 32
#define CHUNK_SIZE_SQ (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_SIZE_CUBE (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
#define BLOCK_SIZE 1.0f
#define MAX_CHUNKS_LOADED 1024
#ifndef MAX_ENTITIES
#define MAX_ENTITIES 65536
#endif
#define MAX_COMPONENTS 256

// Type definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

// Block ID type (16-bit allows 65536 block types)
typedef u16 BlockID;

// Entity ID type
typedef u32 EntityID;
#define NULL_ENTITY 0

// Component type ID
typedef u32 ComponentTypeID;

// Math constants
#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifdef PI_2
#undef PI_2
#endif
#define PI_2 (3.14159265358979323846f * 2.0f)

#ifdef PI_HALF
#undef PI_HALF
#endif
#define PI_HALF (3.14159265358979323846f * 0.5f)
#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0f)
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0f / PI)
#endif
#ifndef EPSILON
#define EPSILON 1e-6f
#endif

// Cardinal directions and faces
typedef enum {
  DIR_DOWN = 0,
  DIR_UP,
  DIR_NORTH,
  DIR_SOUTH,
  DIR_WEST,
  DIR_EAST,
  DIR_COUNT
} Direction;

// RGBA Color structure (used for potions, dyes, map colors)
typedef struct {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
} Color;

// Utility macros
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) (MIN(MAX(x, min), max))
#endif
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef SIGN
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#endif
#ifndef LERP
#define LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#endif
#define SWAP(a, b)                                                             \
  do {                                                                         \
    typeof(a) _tmp = (a);                                                      \
    (a) = (b);                                                                 \
    (b) = _tmp;                                                                \
  } while (0)

// Array size
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Assertions
#ifndef ASSERT
#ifdef DEBUG
#define ASSERT(cond, msg) assert(cond &&msg)
#else
#define ASSERT(cond, msg) ((void)0)
#endif
#endif

// Logging (stub for now, can be implemented with Emscripten console)
#ifndef LOG_INFO
#endif
#ifndef LOG_WARN
#endif
#ifndef LOG_ERROR
#endif
#ifndef LOG_DEBUG
#endif

#endif // GAME_COMMON_H
