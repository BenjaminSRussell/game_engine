#ifndef MATH_LA_MATRICES_MAT4_TYPES_H
#define MATH_LA_MATRICES_MAT4_TYPES_H

#include <common.h>
#include <core/math/types.h> // Ensure we use the core types if available
#include <stdbool.h>

// Matrix validation structure
typedef struct {
  bool has_nan;
  bool has_inf;
  bool is_valid;
  bool is_orthogonal;
  bool is_identity;
} Mat4Validation;

// Matrix statistics structure
typedef struct {
  u64 operations_count;
  u64 simd_operations_count;
  u64 cache_hits;
  u64 cache_misses;
  f64 total_operation_time;
} Mat4Statistics;

// Depending on if types.h is used, we might alias it.
// Assuming types.h defines mat4, we typedef it.
typedef mat4 Mat4;

#endif // MATH_LA_MATRICES_MAT4_TYPES_H
