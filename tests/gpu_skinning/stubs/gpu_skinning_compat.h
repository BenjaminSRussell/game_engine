// Test-only compatibility header for test_gpu_skinning. CMake force-includes it
// (-include) into every translation unit of that target. It declares what
// gpu_skinning.c calls without declaring, and supplies the standard headers the
// unit relies on transitively. It is not production code.
#ifndef GPU_SKINNING_TEST_COMPAT_H
#define GPU_SKINNING_TEST_COMPAT_H

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/math/types.h"

// gpu_skinning.c calls these shader functions; no header in the tree declares them.
u32 shader_load_compute(const char *path);
void shader_destroy(u32 shader);
void shader_bind_compute(u32 shader);
void shader_set_uniform_compute(u32 shader, const char *name, const void *data,
                                usize size);
void shader_dispatch_compute(u32 shader, u32 groups_x, u32 groups_y,
                             u32 groups_z);
void shader_memory_barrier_compute(void);

// gpu_skinning.c calls these without declaring them.
static inline mat4 mat4_identity(void) {
  mat4 result;
  memset(&result, 0, sizeof(result));
  result.m[0][0] = 1.0f;
  result.m[1][1] = 1.0f;
  result.m[2][2] = 1.0f;
  result.m[3][3] = 1.0f;
  return result;
}

static inline f32 clamp(f32 value, f32 min_value, f32 max_value) {
  if (value < min_value)
    return min_value;
  if (value > max_value)
    return max_value;
  return value;
}

#endif
