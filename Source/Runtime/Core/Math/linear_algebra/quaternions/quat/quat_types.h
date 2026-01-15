#ifndef MATH_LA_QUATERNIONS_QUAT_TYPES_H
#define MATH_LA_QUATERNIONS_QUAT_TYPES_H

#include <common.h>
#include <stdbool.h>

// Quaternion validation structure
typedef struct {
  bool has_nan;
  bool has_inf;
  bool is_valid;
  bool is_normalized;
  f32 length;
} QuatValidation;

// Quaternion statistics structure
typedef struct {
  u64 operations_count;
  u64 simd_operations_count;
  u64 cache_hits;
  u64 cache_misses;
  u64 normalizations_count;
  f64 total_operation_time;
} QuatStatistics;

// Quaternion for rotations (w, x, y, z)
typedef union {
  struct {
    f32 w, x, y, z;
  };
  f32 data[4];
} K_ALIGN(16) Quat;

// Convenience macro for creating Quat (compound literal)
#define quat(w, x, y, z) ((Quat){{w, x, y, z}})

#endif // MATH_LA_QUATERNIONS_QUAT_TYPES_H
