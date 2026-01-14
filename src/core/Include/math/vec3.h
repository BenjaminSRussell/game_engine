// math/vec3.h
// Facade header referencing new granular implementation
#ifndef VEC3_H
#define VEC3_H

#include <common.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Include granular definitions
// Note: We are using relative paths from the new location mapping or we rely on
// the build system adding src/engine/math/linear_algebra/vectors/vec3 to
// include path. For now, let's assume we include them directly relative to
// source root or use correct relative linking if possible. Since this is the
// Public API file in include/, we will include the files using their path
// relative to source root if they are headers.

#include "../../math/linear_algebra/vectors/vec3/vec3_ops.h"
#include "../../math/linear_algebra/vectors/vec3/vec3_types.h"

// Forward declarations for non-inline functions (implemented in
// vec3_geometric.c etc)
f32 vec3_angle_between(Vec3 a, Vec3 b);
Vec3 vec3_reflect(Vec3 v, Vec3 normal);
Vec3 vec3_slerp(Vec3 a, Vec3 b, f32 t);
// ... add others as needed from original header ...

#endif // VEC3_H
