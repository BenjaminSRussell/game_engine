#ifndef MATH_LA_VECTORS_VEC3_TYPES_H
#define MATH_LA_VECTORS_VEC3_TYPES_H

#include <common.h>
#include <core/macros.h>
#include <stdbool.h>

// Vector validation structure
typedef struct {
  bool has_nan;
  bool has_inf;
  bool is_valid;
} Vec3Validation;

// Vector statistics structure
typedef struct {
  u64 operations_count;
  u64 simd_operations_count;
  u64 cache_hits;
  u64 cache_misses;
  f64 total_operation_time;
} Vec3Statistics;

// Vector precision enum
typedef enum { VEC3_PRECISION_SINGLE, VEC3_PRECISION_DOUBLE } Vec3Precision;

#include <core/math/types.h>

typedef vec3 Vec3;
typedef vec3 vec3_t;

#endif // MATH_LA_VECTORS_VEC3_TYPES_H
