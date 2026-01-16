/**
 * @file core_types.h
 * @brief Fundamental type definitions for VoxelForge Engine
 *
 * This header defines the core primitive types used throughout the engine.
 * All engine code should use these types for consistency and portability.
 */

#ifndef VOXELFORGE_CORE_TYPES_H
#define VOXELFORGE_CORE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Integer Types
// ============================================================================

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// ============================================================================
// Floating Point Types
// ============================================================================

typedef float f32;
typedef double f64;

// ============================================================================
// Size Types
// ============================================================================

typedef size_t usize;
typedef ptrdiff_t isize;

// ============================================================================
// Boolean Type
// ============================================================================

typedef bool b8;
typedef uint32_t b32; // 32-bit bool for alignment

// ============================================================================
// Handle Types
// ============================================================================

typedef u32 EntityHandle;
typedef u32 ComponentHandle;
typedef u32 ResourceHandle;
typedef u32 AssetHandle;

#define INVALID_HANDLE ((u32)0xFFFFFFFF)

// ============================================================================
// Math Types
// ============================================================================

typedef struct vec2 {
  f32 x, y;
} vec2;
typedef struct vec3 {
  f32 x, y, z;
} vec3;
typedef struct vec4 {
  f32 x, y, z, w;
} vec4;
typedef struct quat {
  f32 x, y, z, w;
} quat;
typedef struct mat4 {
  f32 m[4][4];
} mat4;

// ============================================================================
// Result Type
// ============================================================================

typedef enum VF_Result {
  VF_SUCCESS = 0,
  VF_ERROR_UNKNOWN = -1,
  VF_ERROR_OUT_OF_MEMORY = -2,
  VF_ERROR_INVALID_ARGUMENT = -3,
  VF_ERROR_NOT_FOUND = -4,
  VF_ERROR_ALREADY_EXISTS = -5,
  VF_ERROR_NOT_INITIALIZED = -6,
  VF_ERROR_IO = -7,
  VF_ERROR_TIMEOUT = -8,
  VF_ERROR_NOT_SUPPORTED = -9,
} VF_Result;

// ============================================================================
// Utility Macros
// ============================================================================

#define VF_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define VF_KB(x) ((x) * 1024ULL)
#define VF_MB(x) (VF_KB(x) * 1024ULL)
#define VF_GB(x) (VF_MB(x) * 1024ULL)

#define VF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define VF_MAX(a, b) ((a) > (b) ? (a) : (b))
#define VF_CLAMP(x, lo, hi) VF_MIN(VF_MAX(x, lo), hi)

#define VF_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define VF_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#define VF_UNUSED(x) (void)(x)

// ============================================================================
// Platform Detection
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
#define VF_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define VF_PLATFORM_MACOS 1
#elif TARGET_OS_IOS
#define VF_PLATFORM_IOS 1
#endif
#elif defined(__linux__)
#define VF_PLATFORM_LINUX 1
#elif defined(__ANDROID__)
#define VF_PLATFORM_ANDROID 1
#endif

// ============================================================================
// Compiler Detection
// ============================================================================

#if defined(__clang__)
#define VF_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define VF_COMPILER_GCC 1
#elif defined(_MSC_VER)
#define VF_COMPILER_MSVC 1
#endif

// ============================================================================
// Export/Import Macros
// ============================================================================

#if defined(VF_PLATFORM_WINDOWS)
#if defined(VF_BUILD_DLL)
#define VF_API __declspec(dllexport)
#elif defined(VF_USE_DLL)
#define VF_API __declspec(dllimport)
#else
#define VF_API
#endif
#else
#define VF_API __attribute__((visibility("default")))
#endif

#define VF_INLINE static inline

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_CORE_TYPES_H
